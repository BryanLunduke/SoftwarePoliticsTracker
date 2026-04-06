#include "GameMode.h"
#include "../Minecraft.h"
#include "../../network/packet/UseItemPacket.h"
#include "../../network/packet/PlayerActionPacket.h"
#include "../../world/level/Level.h"
#include "../../world/item/ItemInstance.h"
#include "../player/LocalPlayer.h"
#ifndef STANDALONE_SERVER
#include "../sound/SoundEngine.h"
#include "../particle/ParticleEngine.h"
#endif
#include "../../network/RakNetInstance.h"
#include "../../network/packet/RemoveBlockPacket.h"
#ifndef STANDALONE_SERVER
#include "../renderer/LevelRenderer.h"
#endif
#include "../../world/level/material/Material.h"

GameMode::GameMode( Minecraft* minecraft)
:	minecraft(minecraft),
	destroyProgress(0),
	oDestroyProgress(0),
	destroyTicks(0),
	destroyDelay(0)
{
}

/*virtual*/
Player* GameMode::createPlayer(Level* level) {
    return new LocalPlayer(minecraft, level, minecraft->user, level->dimension->id, isCreativeType());
}

/*virtual*/
void GameMode::interact(Player* player, Entity* entity) {
    player->interact(entity);
}

/*virtual*/
void GameMode::attack(Player* player, Entity* entity) {
	if (minecraft->level->adventureSettings.noPvP && entity->isPlayer())
		return;
	if (minecraft->level->adventureSettings.noPvM && entity->isMob())
		return;
    player->attack(entity);
}

/* virtual */
void GameMode::startDestroyBlock( int x, int y, int z, int face ) {
	if(minecraft->player->getCarriedItem() != NULL && minecraft->player->getCarriedItem()->id == Item::bow->id)
		return;
	destroyBlock(x, y, z, face);
}

/*virtual*/
bool GameMode::destroyBlock(int x, int y, int z, int face) {
    Level* level = minecraft->level;
    Tile* oldTile = Tile::tiles[level->getTile(x, y, z)];
	if (!oldTile)
		return false;

    if (level->adventureSettings.immutableWorld) {
        if (oldTile != (Tile*)Tile::leaves
         && oldTile->material != Material::plant) {
             return false;
        }
    }
#ifndef STANDALONE_SERVER
	minecraft->particleEngine->destroy(x, y, z);
#endif
	int data = level->getData(x, y, z);
    bool changed = level->setTile(x, y, z, 0);
    if (changed) {
#ifndef STANDALONE_SERVER
        minecraft->soundEngine->play(oldTile->soundType->getBreakSound(), x + 0.5f, y + 0.5f, z + 0.5f, (oldTile->soundType->getVolume() + 1) / 2, oldTile->soundType->getPitch() * 0.8f);
#endif
        oldTile->destroy(level, x, y, z, data);
		if (minecraft->options.destroyVibration) minecraft->platform()->vibrate(24);

		if (minecraft->isOnline()) {
			RemoveBlockPacket packet(minecraft->player, x, y, z);
			minecraft->raknetInstance->send(packet);
		}
	}
    return changed;
}
/*virtual*/
bool GameMode::useItemOn(Player* player, Level* level, ItemInstance* item, int x, int y, int z, int face, const Vec3& hit) {
	float clickX = hit.x - x;
	float clickY = hit.y - y;
	float clickZ = hit.z - z;
	item = player->inventory->getSelected();
	if(level->isClientSide) {
		UseItemPacket packet(x, y, z, face, item, player->entityId, clickX, clickY, clickZ);
		minecraft->raknetInstance->send(packet);
	}
    int t = level->getTile(x, y, z);
	if (t == Tile::invisible_bedrock->id) return false;
    if (t > 0 && Tile::tiles[t]->use(level, x, y, z, player))
		return true;

	if (item == NULL) return false;
	if(isCreativeType()) {
		int aux = item->getAuxValue();
		int count = item->count;
		bool success = item->useOn(player, level, x, y, z, face, clickX, clickY, clickZ);
		item->setAuxValue(aux);
		item->count = count;
		return success;
	} else {
		return item->useOn(player, level, x, y, z, face, clickX, clickY, clickZ);
	}
}

bool GameMode::useItem( Player* player, Level* level, ItemInstance* item ) {
	int oldCount = item->count;

	ItemInstance* itemInstance = item->use(level, player);
	if(level->isClientSide) {
		UseItemPacket packet(item, player->entityId, player->aimDirection);
		minecraft->raknetInstance->send(packet);
	}
	if (itemInstance != item || (itemInstance != NULL && itemInstance->count != oldCount)) {
	    //player.inventory.items[player.inventory.selected] = itemInstance;
	    //if (itemInstance.count == 0) {
	    //    player.inventory.items[player.inventory.selected] = NULL;
	    //}
	    return true;
	}
	return false;
}

ItemInstance* GameMode::handleInventoryMouseClick( int containerId, int slotNum, int buttonNum, Player* player ) {
	//return player.containerMenu.clicked(slotNum, buttonNum, player);
	return NULL;
}

void GameMode::handleCloseInventory( int containerId, Player* player ) {
	//player.containerMenu.removed(player);
	//player.containerMenu = player.inventoryMenu;
}

float GameMode::getPickRange() {
	return 5.0f;
}

void GameMode::initPlayer( Player* player ) {
	initAbilities(player->abilities);
}

void GameMode::releaseUsingItem(Player* player){
	if(minecraft->level->isClientSide) {
		PlayerActionPacket packet(PlayerActionPacket::RELEASE_USE_ITEM,  0, 0, 0, 0, player->entityId);
		minecraft->raknetInstance->send(packet);
	}
	player->releaseUsingItem();
}

void GameMode::tick() {
	oDestroyProgress = destroyProgress;
}

void GameMode::render( float a ) {
#ifndef STANDALONE_SERVER
	if (destroyProgress <= 0) {
		minecraft->gui.progress = 0;
		minecraft->levelRenderer->destroyProgress = 0;
	} else {
		float dp = oDestroyProgress + (destroyProgress - oDestroyProgress) * a;
		minecraft->gui.progress = dp;
		minecraft->levelRenderer->destroyProgress = dp;
	}
#endif
}
