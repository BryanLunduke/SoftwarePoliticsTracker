
#include "ServerSideNetworkHandler.h"
#include "../world/level/Level.h"
#include "../world/entity/player/Player.h"
#include "../world/entity/player/Inventory.h"
#include "../world/Container.h"
#include "../world/inventory/BaseContainerMenu.h"
#include "packet/PacketInclude.h"

#include "RakNetInstance.h"
#include "../client/Minecraft.h"
#include "../client/player/LocalPlayer.h"
#include "../client/gamemode/GameMode.h"
#include "../raknet/RakPeerInterface.h"
#include "../raknet/PacketPriority.h"
#ifndef STANDALONE_SERVER
#include "../client/sound/SoundEngine.h"
#endif
#include "../server/ServerPlayer.h"
#include "../world/entity/item/FallingTile.h"

#define TIMES(x) for(int itc ## __LINE__ = 0; itc ## __LINE__ < x; ++ itc ## __LINE__)

ServerSideNetworkHandler::ServerSideNetworkHandler(Minecraft* minecraft, IRakNetInstance* raknetInstance)
:	minecraft(minecraft),
	raknetInstance(raknetInstance),
	level(NULL)
{
	allowIncomingConnections(false);
	rakPeer = raknetInstance->getPeer();
}

ServerSideNetworkHandler::~ServerSideNetworkHandler()
{
	if (level) {
		level->removeListener(this);
	}

	for (unsigned int i = 0; i < _pendingPlayers.size(); ++i)
		delete _pendingPlayers[i];
}

void ServerSideNetworkHandler::tileChanged(int x, int y, int z)
{
	//LOGI("tileChanged(%d, %d, %d)\n", x, y, z);

	// broadcast change event
	UpdateBlockPacket packet(x, y, z, level->getTile(x, y, z), level->getData(x, y, z));
	RakNet::BitStream bitStream;
	packet.write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

Packet* ServerSideNetworkHandler::getAddPacketFromEntity( Entity* entity ) {
	if (entity->isMob() && !entity->isPlayer()) { //@fix: This code is duplicated. See if it can be unified.
		if (minecraft->player) {
			// I guess this should always be true, but it crashed somewhere in this
			// function once and I only see this one as a potential problem
			return new AddMobPacket((Mob*)entity);
		}
	}
	else if (entity->isPlayer()) {

	} else if (entity->isItemEntity()) {
		AddItemEntityPacket* packet = new AddItemEntityPacket((ItemEntity*)entity);
		entity->xd = packet->xa();
		entity->yd = packet->ya();
		entity->zd = packet->za();
		//LOGI("item-entity @ server: %f, %f, %f\n", e->xd, e->yd, e->zd);
		return packet;
	} else if(entity->isHangingEntity()) {
		return new AddPaintingPacket((Painting*) entity);
	} else {
        int type = entity->getEntityTypeId();
        int data = entity->getAuxData();

        if (EntityTypes::IdFallingTile == type) {
            FallingTile* ft = (FallingTile*) entity;
            data = -(ft->tile | (ft->data << 16));
        }

		//LOGI("Server: adding entity with %f, %f, %f\n", e->xd, e->yd, e->zd);
		AddEntityPacket* packet = new AddEntityPacket(entity, data);
		/*
		entity->xd = packet->xd;
		entity->yd = packet->yd;
		entity->zd = packet->zd;
		*/
		return packet;
	}
	return NULL;
}
void ServerSideNetworkHandler::entityAdded(Entity* e) {
	Packet* packet = getAddPacketFromEntity(e);
	if(packet != NULL) {
		if (e->isMob() && !e->isPlayer()) {
			redistributePacket(packet, rakPeer->GetMyGUID());
			delete packet;
		} else {
			raknetInstance->send(packet);
			// raknetInstance->send always deletes package
		}
		
	}
}

void ServerSideNetworkHandler::entityRemoved(Entity* e)
{
	if (!e->isPlayer()) { //@fix: This code MIGHT be duplicated. See if it can be unified.
		RemoveEntityPacket packet(e->entityId);
		redistributePacket(&packet, rakPeer->GetMyGUID());
	} else { // Is a player
		RemovePlayerPacket packet((Player*) e);
		redistributePacket(&packet, rakPeer->GetMyGUID());
	}
}

void ServerSideNetworkHandler::redistributePacket(Packet* packet, const RakNet::RakNetGUID& fromPlayer)
{
	// broadcast the new player to all other players
	RakNet::BitStream bitStream;
	packet->write(&bitStream);
	rakPeer->Send(&bitStream, packet->priority, packet->reliability, 0, fromPlayer, true);
}

void ServerSideNetworkHandler::displayGameMessage(const std::string& message)
{
#ifndef STANDALONE_SERVER
	minecraft->gui.addMessage(message);
#else
	LOGI("%s\n", message.c_str());
#endif
	MessagePacket packet(message.c_str());
	raknetInstance->send(packet);
}

void ServerSideNetworkHandler::onNewClient(const RakNet::RakNetGUID& clientGuid)
{
	LOGI("onNewClient, client guid: %s\n", clientGuid.ToString());
}

void ServerSideNetworkHandler::onDisconnect(const RakNet::RakNetGUID& guid)
{
	if (!level) return;
	LOGI("onDisconnect\n");

	const PlayerList& players = level->players;
	for (unsigned int i = 0; i < players.size(); i++)
	{
		Player* player = players[i];

		if (player->owner == guid)
		{
			std::string message = player->name;
			message += " disconnected from the game";
			displayGameMessage(message);

			//RemoveEntityPacket packet(player->entityId);
			//raknetInstance->send(packet);
			player->reallyRemoveIfPlayer = true;
			level->removeEntity(player);
			//level->removePlayer(player);
			
			LOGI("&e@disc: %p", player);

			break;
		}
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, LoginPacket* packet)
{
	if (!level) return;
	if (!_allowIncoming) return;

	LOGI("LoginPacket\n");

	int loginStatus = LoginStatus::Success;
	//
	// Bad/incompatible client version
	//
	bool oldClient = packet->clientNetworkVersion < SharedConstants::NetworkProtocolLowestSupportedVersion;
	bool oldServer = packet->clientNetworkLowestSupportedVersion > SharedConstants::NetworkProtocolVersion;
	if (oldClient || oldServer)
		loginStatus = oldClient? LoginStatus::Failed_ClientOld : LoginStatus::Failed_ServerOld;

	RakNet::BitStream bitStream;
	LoginStatusPacket(loginStatus).write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);

	if (LoginStatus::Success != loginStatus)
		return;

	//
	// Valid client version
	//
	Player* newPlayer = new ServerPlayer(minecraft, level);

	minecraft->gameMode->initAbilities(newPlayer->abilities);
	newPlayer->owner = source;
	newPlayer->name = packet->clientName.C_String();
	_pendingPlayers.push_back(newPlayer);

	// Reset the player so he doesn't spawn inside blocks
	while (newPlayer->y > 0) {
		newPlayer->setPos(newPlayer->x, newPlayer->y, newPlayer->z);
		if (level->getCubes(newPlayer, newPlayer->bb).size() == 0) break;
		newPlayer->y += 1;
	}
	newPlayer->moveTo(newPlayer->x, newPlayer->y - newPlayer->heightOffset, newPlayer->z, newPlayer->yRot, newPlayer->xRot);

	// send world seed
	{
        RakNet::BitStream bitStream;

        // @todo: Read from LevelData?
		int gameType = minecraft->isCreativeMode()
			? GameType::Creative
			: GameType::Survival;

        StartGamePacket(
            level->getSeed(),
            level->getLevelData()->getGeneratorVersion(),
            gameType,
            newPlayer->entityId,
            newPlayer->x, newPlayer->y - newPlayer->heightOffset, newPlayer->z
        ).write(&bitStream);

        rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ReadyPacket* packet)
{
	if (!level) return;

	if (packet->type == ReadyPacket::READY_CLIENTGENERATION)
		onReady_ClientGeneration(source);

	if (packet->type == ReadyPacket::READY_REQUESTEDCHUNKS)
		onReady_RequestedChunks(source);
}

void ServerSideNetworkHandler::onReady_ClientGeneration(const RakNet::RakNetGUID& source)
{
	Player* newPlayer = popPendingPlayer(source);
	if (!newPlayer) {
		for (int i = 0; i < 3; ++i)
			LOGE("We don't have a user associated with this player!\n");
		return;
	}
	// Create a bitstream that can be used by everyone (after ::reset() )
	RakNet::BitStream bitStream;

	// send level info
	SetTimePacket(level->getTime()).write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);

	// send all pre-existing players to the new player
	const PlayerList& players = level->players;
	for (unsigned int i = 0; i < players.size(); i++) {
		Player* player = players[i];

        bitStream.Reset();
        AddPlayerPacket(player).write(&bitStream);
        rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);

        if (player->getArmorTypeHash()) {
            bitStream.Reset();
            PlayerArmorEquipmentPacket(player).write(&bitStream);
            rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
        }
	}

	level->addEntity(newPlayer);
#ifndef STANDALONE_SERVER
	minecraft->gui.addMessage(newPlayer->name + " joined the game");
#else
	LOGW("%s joined the game\n", newPlayer->name.c_str());
#endif

	// Send all Entities to the new player
	for (unsigned int i = 0; i < level->entities.size(); ++i) {
		Entity* e = level->entities[i];
		Packet* packet = getAddPacketFromEntity(e);
		if(packet != NULL) {
			bitStream.Reset();
			packet->write(&bitStream);
			rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
			delete packet;
		}
	}

	// Additional packets
	// * set spawn
	/*
	bitStream.Reset();
	SetSpawnPositionPacket spawnPacket(level->getSharedSpawnPos());
	raknetInstance->send(source, spawnPacket);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
	*/

	// broadcast the new player to all other players
	bitStream.Reset();
	AddPlayerPacket(newPlayer).write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, true);
}

//
// Messages to be sent after client has finished applying changes
//
void ServerSideNetworkHandler::onReady_RequestedChunks(const RakNet::RakNetGUID& source)
{
	RakNet::BitStream bitStream;
	// Send all TileEntities to the new player
	for (unsigned int i = 0; i < level->tileEntities.size(); ++i) {
		TileEntity* e = level->tileEntities[i];
		Packet* packet = e->getUpdatePacket();
		if (packet != NULL) {
			bitStream.Reset();
			packet->write(&bitStream);
			rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
			delete packet;
		}
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, MovePlayerPacket* packet)
{
	if (!level) return;

	//LOGI("MovePlayerPacket\n");
	if (Entity* entity = level->getEntity(packet->entityId))
	{
		entity->xd = entity->yd = entity->zd = 0;
		entity->lerpTo(packet->x, packet->y, packet->z, packet->yRot, packet->xRot, 3);

		// broadcast this packet to other clients
		redistributePacket(packet, source);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, RemoveBlockPacket* packet){
	if (!level) return;

	Player* player = getPlayer(source);
	if (!player) return;

	player->swing();

	int x = packet->x, y = packet->y, z = packet->z;

	// code copied from GameMode.cpp
	int oldId = level->getTile(x, y, z);
	int data = level->getData(x, y, z);
	Tile* oldTile = Tile::tiles[oldId];
	bool changed = level->setTile(x, y, z, 0);
	if (oldTile != NULL && changed) {
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, oldTile->soundType->getBreakSound(), (oldTile->soundType->getVolume() + 1) / 2, oldTile->soundType->getPitch() * 0.8f);

		if (minecraft->gameMode->isSurvivalType() && player->canDestroy(oldTile))
			//oldTile->spawnResources(level, x, y, z, data, 1); //@todo
			oldTile->playerDestroy(level, player, x, y, z, data);

		oldTile->destroy(level, x, y, z, data);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, RequestChunkPacket* packet)
{
	if (!level)
        return;

    LevelChunk* chunk = level->getChunk(packet->x, packet->z);

	if (!chunk)
        return;

	ChunkDataPacket cpacket(chunk->x, chunk->z, chunk);

	RakNet::BitStream bitStream;
	cpacket.write(&bitStream);
	rakPeer->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, source, false);
    //LOGI("RequestChunkPacket @ (%d, %d). %d bytes\n", packet->x, packet->z, cpacket.chunkData.GetNumberOfBytesUsed());
	//LOGI("size: %d\n", bitStream.GetNumberOfBytesUsed());

    const LevelChunk::TEMap& teMap = chunk->getTileEntityMap();
    for (LevelChunk::TEMapCIterator cit = teMap.begin(); cit != teMap.end(); ++cit) {
        TileEntity* te = cit->second;
        if (Packet* p = te->getUpdatePacket()) {
            bitStream.Reset();
            p->write(&bitStream);
            raknetInstance->send(source, p);
        }
    }
}

void ServerSideNetworkHandler::levelGenerated( Level* level )
{
	this->level = level;
	
	if (minecraft->player) {
		minecraft->player->owner = rakPeer->GetMyGUID();
	}
	
	level->addListener(this);
#ifndef STANDALONE_SERVER
	allowIncomingConnections(minecraft->options.serverVisible);
#else
	allowIncomingConnections(true);
#endif
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerEquipmentPacket* packet)
{
	if (!level) return;

	Player* player = getPlayer(source);
	if (!player) return;
	if (rakPeer->GetMyGUID() == player->owner) return;

	// override the player's inventory
	//int slot = player->inventory->getSlot(packet->itemId, packet->itemAuxValue);
	int slot = Inventory::MAX_SELECTION_SIZE;
	if (slot >= 0) {
		if (packet->itemId == 0) {
			player->inventory->clearSlot(slot);
		} else {
			// @note: 128 is an ugly hack for depletable items.
			// @todo: fix
			ItemInstance newItem(packet->itemId, 128, packet->itemAuxValue);
			player->inventory->replaceSlot(slot, &newItem);
		}
		player->inventory->moveToSelectedSlot(slot, true);
		redistributePacket(packet, source);
	} else {
		LOGW("Warning: Remote player doesn't have his thing, Odd!\n");
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerArmorEquipmentPacket* packet) {
    if (!level) return;

    Player* player = getPlayer(source);
    if (!player) return;
    if (rakPeer->GetMyGUID() == player->owner) return;

    packet->fillIn(player);
    redistributePacket(packet, source);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, InteractPacket* packet) {
	if (!level) return;

	Entity* src = level->getEntity(packet->sourceId);
	Entity* entity = level->getEntity(packet->targetId);
	if (src && entity && src->isPlayer())
	{
		Player* player = (Player*) src;
		if (InteractPacket::Attack == packet->action) {
			player->swing();
			minecraft->gameMode->attack(player, entity);
		}
		if (InteractPacket::Interact == packet->action) {
			player->swing();
			minecraft->gameMode->interact(player, entity);
		}

		redistributePacket(packet, source);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, AnimatePacket* packet)
{
	if (!level)
        return;

    // Own player -> invalid
    if (minecraft->player && minecraft->player->entityId == packet->entityId) {
        return;
    }

	Entity* entity = level->getEntity(packet->entityId);
	if (!entity || !entity->isPlayer())
        return;

    Player* player = (Player*) entity;

	switch (packet->action) {
	case AnimatePacket::Swing:
		player->swing();
		break;
	default:
		LOGW("Unknown Animate action: %d\n", packet->action);
		break;
	}
	redistributePacket(packet, source);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, UseItemPacket* packet)
{
	if (!level) return;

	LOGI("UseItemPacket\n");
	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* player = (Player*) entity;
		int x = packet->x, y = packet->y, z = packet->z;
		Tile* t = Tile::tiles[level->getTile(x, y, z)];

		if (t == Tile::invisible_bedrock) return;
		if (t && t->use(level, x, y, z, player)) return;
		if (packet->item.isNull()) return;

		ItemInstance* item = &packet->item;

		if(packet->face == 255) {
            // Special case: x,y,z means direction-of-action
            player->aimDirection.set(packet->x / 32768.0f, packet->y / 32768.0f, packet->z / 32768.0f);
			minecraft->gameMode->useItem(player, level, item);
		}
		else {
			minecraft->gameMode->useItemOn(player, level, item, packet->x, packet->y, packet->z, packet->face,
				Vec3(packet->clickX + packet->x, packet->clickY + packet->y, packet->clickZ + packet->z));
		}
		
		//LOGW("Use Item not working! Out of synch?\n");

		// Don't have to redistribute (ugg.. this will mess up), cause tileUpdated is sent
		//redistributePacket(packet, source);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, EntityEventPacket* packet) {
	if (!level) return;

	if (Entity* e = level->getEntity(packet->entityId))
		e->handleEntityEvent(packet->eventId);
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, PlayerActionPacket* packet)
{
	if (!level) return;
	LOGI("PlayerActionPacket\n");
	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* player = (Player*) entity;
		if(packet->action == PlayerActionPacket::RELEASE_USE_ITEM) {
			minecraft->gameMode->releaseUsingItem(player);
			return;
		}
		else if(packet->action == PlayerActionPacket::STOP_SLEEPING) {
			player->stopSleepInBed(true, true, true);
		}
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, RespawnPacket* packet )
{
	if (!level) return;

	NetEventCallback::handle(level, source, packet );
	redistributePacket(packet, source);
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, SendInventoryPacket* packet )
{
	if (!level) return;

	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* p = (Player*)entity;
		p->inventory->replace(packet->items, packet->numItems);
		if ((packet->extra & SendInventoryPacket::ExtraDrop) != 0) {
			p->inventory->dropAll(false);
            //@todo @armor : Drop armor
		}
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, DropItemPacket* packet )
{
	if (!level) return;

	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* p = (Player*)entity;
		p->drop(new ItemInstance(packet->item), packet->dropType != 0);
	}
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerClosePacket* packet) {
	if (!level) return;

	Player* p = findPlayer(level, &source);
	if (!p) return;

	if (p != minecraft->player)
		static_cast<ServerPlayer*>(p)->doCloseContainer();
}

void ServerSideNetworkHandler::handle(const RakNet::RakNetGUID& source, ContainerSetSlotPacket* packet) {
	if (!level) return;

	Player* p = findPlayer(level, &source);
	if (!p) return;

	if (p->containerMenu == NULL) {
		LOGW("User has no container!\n");
		return;
	}
	if (p->containerMenu->containerId != packet->containerId)
	{
		LOGW("Wrong container id: %d vs %d\n", p->containerMenu->containerId, packet->containerId);
		return;
	}
	
	if (ContainerType::FURNACE == p->containerMenu->containerType) {
		//LOGI("Server:Setting slot %d: %s\n", packet->slot, packet->item.toString().c_str());
		p->containerMenu->setSlot(packet->slot, &packet->item);
		//p->containerMenu->setSlot(packet->slot, packet->item.isNull()? NULL : &packet->item);
	}
	if (ContainerType::CONTAINER == p->containerMenu->containerType) {
		//LOGI("Server:Setting slot %d: %s\n", packet->slot, packet->item.toString().c_str());
		p->containerMenu->setSlot(packet->slot, &packet->item);
		//p->containerMenu->setSlot(packet->slot, packet->item.isNull()? NULL : &packet->item);
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, SetHealthPacket* packet )
{
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		Player* p = level->players[i];
		if (p->owner == source) {
			if (packet->health <= -32) {
				int diff = packet->health - SetHealthPacket::HEALTH_MODIFY_OFFSET;
				if (diff > 0) p->hurt(NULL, diff);
				else if (diff < 0) p->heal(-diff);
			}
			break;
		}
	}
}

void ServerSideNetworkHandler::handle( const RakNet::RakNetGUID& source, SignUpdatePacket* packet ) {
	redistributePacket(packet, source);
	if (!level)
		return;

	TileEntity* te = level->getTileEntity(packet->x, packet->y, packet->z);
	if (TileEntity::isType(te, TileEntityType::Sign)) {
		SignTileEntity* ste = (SignTileEntity*) te;
		if (ste->isEditable()) {
			for (int i = 0; i < SignTileEntity::NUM_LINES; i++) {
				ste->messages[i] = packet->lines[i];
			}
			//ste->setChanged();
		}
	}
}

void ServerSideNetworkHandler::allowIncomingConnections( bool doAllow )
{
	if (doAllow) {
		raknetInstance->announceServer(minecraft->options.username);
	} else {
		raknetInstance->announceServer("");
	}
	_allowIncoming = doAllow;
}

Player* ServerSideNetworkHandler::popPendingPlayer( const RakNet::RakNetGUID& source )
{
	if (!level) {
		LOGE("Could not add player since Level is NULL!\n");
		return NULL;
	}

	for (unsigned int i = 0; i < _pendingPlayers.size(); ++i) {
		Player* p = _pendingPlayers[i];
		if (p->owner == source) {
			_pendingPlayers.erase(_pendingPlayers.begin() + i);
			return p;
		}
	}
	return NULL;
}

void ServerSideNetworkHandler::levelEvent( Player* source, int type, int x, int y, int z, int data )
{
	LevelEventPacket packet(type, x, y, z, data);
	redistributePacket(&packet, source? source->owner : rakPeer->GetMyGUID());
}

void ServerSideNetworkHandler::tileEvent( int x, int y, int z, int b0, int b1 )
{
	TileEventPacket packet(x, y, z, b0, b1);
	raknetInstance->send(packet);
}

Player* ServerSideNetworkHandler::getPlayer( const RakNet::RakNetGUID& source ) {
    for (unsigned int i = 0; i < level->players.size(); ++i)
        if (source == level->players[i]->owner) return level->players[i];
    return NULL;
}
