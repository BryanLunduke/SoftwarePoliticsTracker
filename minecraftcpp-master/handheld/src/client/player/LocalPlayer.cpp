#include "LocalPlayer.h"
#include "../Minecraft.h"
#include "../../ErrorCodes.h"
#include "../../world/entity/EntityEvent.h"
#include "../../world/entity/player/Player.h"
#include "../../world/inventory/BaseContainerMenu.h"
#include "../../world/item/BowItem.h"
#include "../../world/level/Level.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/level/tile/entity/TileEntity.h"
#include "../../world/level/material/Material.h"
#include "../../network/packet/ContainerClosePacket.h"
#include "../../network/packet/MovePlayerPacket.h"
#include "../../network/packet/PlayerEquipmentPacket.h"
#include "../../network/RakNetInstance.h"
#include "../../network/packet/DropItemPacket.h"
#include "../../network/packet/SetHealthPacket.h"
#include "../../network/packet/SendInventoryPacket.h"
#include "../../network/packet/EntityEventPacket.h"
#include "../../network/packet/PlayerActionPacket.h"
#ifndef STANDALONE_SERVER
#include "../gui/Screen.h"
#include "../gui/screens/FurnaceScreen.h"
#include "../gui/screens/ChestScreen.h"
#include "../gui/screens/crafting/WorkbenchScreen.h"
#include "../gui/screens/crafting/StonecutterScreen.h"
#include "../gui/screens/InBedScreen.h"
#include "../gui/screens/TextEditScreen.h"
#include "../particle/TakeAnimationParticle.h"
#endif
#include "../../network/packet/AnimatePacket.h"
#include "../../world/item/ArmorItem.h"
#include "../../network/packet/PlayerArmorEquipmentPacket.h"

//@note: doesn't work completely, since it doesn't care about stairs rotation
static bool isJumpable(int tileId) {
	return tileId != Tile::fence->id
		&& tileId != Tile::fenceGate->id
		&& tileId != Tile::stoneSlabHalf->id
		&& tileId != Tile::trapdoor->id
        && tileId != Tile::sign->id
        && tileId != Tile::wallSign->id
		&& (Tile::tiles[tileId] != NULL && Tile::tiles[tileId]->getRenderShape() != Tile::SHAPE_STAIRS);
}

LocalPlayer::LocalPlayer(Minecraft* minecraft, Level* level, User* user, int dimension, bool isCreative)
:	Player(level, isCreative),
	minecraft(minecraft),
	input(NULL),
	sentInventoryItemId(-1),
	sentInventoryItemData(-1),
	autoJumpEnabled(true),
	armorTypeHash(0)
{
	this->dimension = dimension;
	_init();

	if (user != NULL) {
		if (user->name.length() > 0)
			//customTextureUrl = "http://s3.amazonaws.com/MinecraftSkins/" + user.name + ".png";
			this->name = user->name;
	}
}

LocalPlayer::~LocalPlayer() {
	//delete input;
	//input = NULL;
}

/*private*/
void LocalPlayer::calculateFlight(float xa, float ya, float za) {

    xa = xa * minecraft->options.flySpeed;
    ya = 0;
    za = za * minecraft->options.flySpeed;

#ifdef ANDROID
    if (Keyboard::isKeyDown(103)) ya = .2f * minecraft->options.flySpeed;
    if (Keyboard::isKeyDown(102)) ya = -.2f * minecraft->options.flySpeed;
#else
    if (Keyboard::isKeyDown(Keyboard::KEY_E)) ya = .2f * minecraft->options.flySpeed;
    if (Keyboard::isKeyDown(Keyboard::KEY_Q)) ya = -.2f * minecraft->options.flySpeed;
#endif

    flyX = 10 * smoothFlyX.getNewDeltaValue(xa, .35f * minecraft->options.sensitivity);
    flyY = 10 * smoothFlyY.getNewDeltaValue(ya, .35f * minecraft->options.sensitivity);
    flyZ = 10 * smoothFlyZ.getNewDeltaValue(za, .35f * minecraft->options.sensitivity);
}

bool LocalPlayer::isSolidTile(int x, int y, int z) {
	int tileId = level->getTile(x, y, z);
	return tileId > 0 && Tile::tiles[tileId]->material->isSolid();
}

void LocalPlayer::tick() {

	super::tick();
	if(!useItem.isNull()) {
		ItemInstance* item = inventory->getSelected();
		if(item != NULL && *item == useItem) {
			if (useItemDuration <= 25 && useItemDuration % 4 == 0) {
				spawnEatParticles(item, 5);
			}
			if(--useItemDuration == 0) {
				if(!level->isClientSide) {
					completeUsingItem();
				} else {
					EntityEventPacket p(entityId, EntityEvent::USE_ITEM_COMPLETE);
					level->raknetInstance->send(p);
				}
			}
		}
		else {
			stopUsingItem();
		}
	}
	if (minecraft->isOnline())
	{
		if (std::abs(x - sentX) > .1f || std::abs(y - sentY) > .01f || std::abs(z - sentZ) > .1f || std::abs(sentRotX - xRot) > 1 || std::abs(sentRotY - yRot) > 1)
		{
			MovePlayerPacket packet(entityId, x, y - heightOffset, z, xRot, yRot);
			minecraft->raknetInstance->send(packet);
			sentX = x;
			sentY = y;
			sentZ = z;
			sentRotX = xRot;
			sentRotY = yRot;
		}

		ItemInstance* item = inventory->getSelected();
		int newItemId   = (item && item->count > 0) ? item->id : 0;
		int newItemData = (item && item->count > 0) ? item->getAuxValue() : 0;

		if (sentInventoryItemId != newItemId
		||  sentInventoryItemData != newItemData)
		{
			sentInventoryItemId   = newItemId;
			sentInventoryItemData = newItemData;
			PlayerEquipmentPacket packet(entityId, newItemId, newItemData);
			minecraft->raknetInstance->send(packet);
		}
	}
/*
    for (int i = 0; i < 4; ++i) {
        ItemInstance* a = getArmor(i);
        if (!a) continue;

        ArmorItem* item = (ArmorItem*) a->getItem();

        printf("armor %d: %d\n", i, a->getAuxValue());
    }
/**/

	updateArmorTypeHash();
#ifndef STANDALONE_SERVER
	if (!minecraft->screen && containerMenu) {
		static bool hasPostedError = false;
		if (!hasPostedError) {
			minecraft->gui.postError( ErrorCodes::ContainerRefStillExistsAfterDestruction );
			hasPostedError = true;
		}
	}
#endif
	//LOGI("biome: %s\n", level->getBiomeSource()->getBiome((int)x >> 4, (int)z >> 4)->name.c_str());
}

/*public*/
void LocalPlayer::aiStep() {
	jumpTriggerTime--;
	ascendTriggerTime--;
	descendTriggerTime--;

	bool wasJumping = input->jumping;
#ifndef STANDALONE_SERVER
    bool screenCovering = minecraft->screen && !minecraft->screen->passEvents;
	if (!screenCovering)
		input->tick(this);

    if (input->sneaking) {
        if (ySlideOffset < 0.2f) ySlideOffset = 0.2f;
    }
#endif
	if (abilities.mayfly) {
		// Check for flight toggle
		if (!wasJumping && input->jumping) {
			if (jumpTriggerTime <= 0) jumpTriggerTime = 7;
			else {
				abilities.flying = !abilities.flying;
				jumpTriggerTime = 0;
			}
		}
		if (abilities.flying) {
			if (input->wantUp) {
				yd += 0.15f;
				//xd = zd = 0;
			}
			if (input->wantDown) {
				yd -= 0.15f;
			}
		}
	}

	if(isUsingItem()) {
		const float k = 0.35f;
		input->xa *= k;
		input->ya *= k;
	}

	Mob::aiStep();
    super::aiStep();

	//if (onGround && abilities.flying)
	//	abilities.flying = false;

	if (interpolateOnly())
		updateAi();
}

/*public*/
void LocalPlayer::closeContainer() {
	if (level->isClientSide) {
		ContainerClosePacket packet(containerMenu->containerId);
		minecraft->raknetInstance->send(packet);
	}
	super::closeContainer();
    minecraft->setScreen(NULL);
}

//@Override
void LocalPlayer::move(float xa, float ya, float za) {
    //@note: why is this == minecraft->player needed?
    if (this == minecraft->player && minecraft->options.isFlying) {
        noPhysics = true;
        float tmp = walkDist; // update
        calculateFlight((float) xa, (float) ya, (float) za);
        fallDistance = 0;
        yd = 0;
        super::move(flyX, flyY, flyZ);
        onGround = true;
        walkDist = tmp;
    } else {
		if (autoJumpTime > 0) {
			autoJumpTime--;
			input->jumping = true;
		}
		float prevX = x, prevZ = z;

        super::move(xa, ya, za);

		float newX = x, newZ = z;

		if (autoJumpTime <= 0 && autoJumpEnabled)
		{
			// auto-jump when crossing the middle of a tile, and the tile in the front is blocked
			bool jump = false;
			if (Mth::floor(prevX * 2.0f) != Mth::floor(newX * 2.0f) || Mth::floor(prevZ * 2.0f) != Mth::floor(newZ * 2.0f))
			{
				float dist = Mth::sqrt(xa * xa + za * za);
				const int xx = Mth::floor(x + xa / dist);
				const int zz = Mth::floor(z + za / dist);
				const int tileId = level->getTile(xx, (int)(y-1), zz);
				jump = (isSolidTile(xx, (int)(y-1), zz) // Solid block to jump up on
					&& !isSolidTile(xx, (int)y, zz) && !isSolidTile(xx, (int)(y+1), zz)) // Enough space
					&& isJumpable(tileId);
			}
			if (jump)
			{
				autoJumpTime = 1;
			}
		}
    }
}

void LocalPlayer::updateAi() {
    super::updateAi();
    this->xxa = input->xa;
    this->yya = input->ya;
    this->jumping = input->jumping || autoJumpTime > 0;
}

void LocalPlayer::take( Entity* e, int orgCount )
{
#ifndef STANDALONE_SERVER
	if (e->isItemEntity())
		minecraft->particleEngine->add(new TakeAnimationParticle(minecraft->level, (ItemEntity*)e, this, -0.5f));
#endif
}

void LocalPlayer::setKey( int eventKey, bool eventKeyState )
{
	input->setKey(eventKey, eventKeyState);
}

void LocalPlayer::releaseAllKeys()
{
	if (input) input->releaseAllKeys();
}

float LocalPlayer::getFieldOfViewModifier() {
	float targetFov = 1.0f;
	if(abilities.flying) targetFov *= 1.1f;
	targetFov *= ((walkingSpeed * getWalkingSpeedModifier()) / DEFAULT_WALK_SPEED +1) / 2;

	if(isUsingItem() && getUseItem()->id == Item::bow->id) {
		float ticksHeld = (float)getTicksUsingItem();
		float scale = ticksHeld / BowItem::MAX_DRAW_DURATION;
		if(scale > 1) {
			scale = 1;
		}
		else {
			scale *= scale;
		}
		targetFov *= 1.0f - scale * 0.15f;
	}
	return targetFov;
}
void LocalPlayer::addAdditonalSaveData( CompoundTag* entityTag )
{
	super::addAdditonalSaveData(entityTag);
	entityTag->putInt("Score", score);
}

void LocalPlayer::readAdditionalSaveData( CompoundTag* entityTag )
{
	super::readAdditionalSaveData(entityTag);
	score = entityTag->getInt("Score");
}

bool LocalPlayer::isSneaking()
{
	return input->sneaking;
}

void LocalPlayer::hurtTo( int newHealth )
{
	int dmg = health - newHealth;
	if (dmg <= 0) {
		this->health = newHealth;
	} else {
		lastHurt = dmg;
		lastHealth = health;
		invulnerableTime = invulnerableDuration;

		minecraft->player->bypassArmor = true;
		actuallyHurt(dmg);
		minecraft->player->bypassArmor = false;

		hurtTime = hurtDuration = 10;
	}
}

void LocalPlayer::actuallyHurt( int dmg )
{
#ifndef STANDALONE_SERVER
    if (minecraft->screen && minecraft->screen->closeOnPlayerHurt()) {
		if (containerMenu) closeContainer();
        else minecraft->setScreen(NULL);
	}
#endif
    super::actuallyHurt(dmg);
}

void LocalPlayer::respawn()
{
	minecraft->respawnPlayer();
}

void LocalPlayer::die(Entity* source)
{
	// If we're an online client, send the inventory to be dropped
	// If we're the server, drop the inventory immediately
	if (level->isClientSide) {
		SendInventoryPacket packet(this, true);
		minecraft->raknetInstance->send(packet);
	}
	inventory->dropAll(level->isClientSide);
	for (int i = 0; i < NUM_ARMOR; ++i) {
		ItemInstance* item = getArmor(i);
		if (!ItemInstance::isArmorItem(item)) return;

		drop(new ItemInstance(*item), true);
		setArmor(i, NULL);
	}

	super::die(source);
}

void LocalPlayer::swing() {
    super::swing();

    if (swingTime == -1) {
        AnimatePacket packet(AnimatePacket::Swing, this);
        packet.reliability = UNRELIABLE;
        packet.priority = MEDIUM_PRIORITY;
        minecraft->raknetInstance->send(packet);
    }
}

void LocalPlayer::reset() {
	super::reset();
	this->_init();
}

void LocalPlayer::_init() {
	autoJumpTime		= 0;
	jumpTriggerTime		= 0;
	ascendTriggerTime	= 0;
	descendTriggerTime	= 0;
	ascending	= false;
	descending	= false;

	ItemInstance* item = inventory->getSelected();
	sentInventoryItemId = item? item->id : 0;
	sentInventoryItemData = item? item->getAuxValue() : 0;
}

void LocalPlayer::startCrafting(int x, int y, int z, int tableSize) {
#ifndef STANDALONE_SERVER
	if (!minecraft->isCreativeMode())
		minecraft->setScreen( new WorkbenchScreen(tableSize) );
#endif
}

void LocalPlayer::startStonecutting(int x, int y, int z) {
#ifndef STANDALONE_SERVER
	if (!minecraft->isCreativeMode())
		minecraft->setScreen( new StonecutterScreen() );
#endif
}

void LocalPlayer::openFurnace( FurnaceTileEntity* e ) {
#ifndef STANDALONE_SERVER
	if (!minecraft->isCreativeMode())
		minecraft->setScreen( new FurnaceScreen(this, e) );
#endif
}

void LocalPlayer::openContainer( ChestTileEntity* container ) {
#ifndef STANDALONE_SERVER
	if (!minecraft->isCreativeMode())
		minecraft->setScreen( new ChestScreen(this, container) );
#endif
}

void LocalPlayer::drop( ItemInstance* item, bool randomly )
{
	if (!item)
		return;

	if (level->isClientSide) {
		DropItemPacket packet(entityId, *item);
		minecraft->raknetInstance->send(packet);
		// delete the ItemEntity here, since we don't add it to level
		delete item;
	} else {
		super::drop(item, randomly);
	}
}

void LocalPlayer::causeFallDamage( float distance )
{
	int dmg = (int) ceil((distance - 3));
	if (dmg > 0) {
		if (level->isClientSide) {
			SetHealthPacket packet(SetHealthPacket::HEALTH_MODIFY_OFFSET + dmg);
			minecraft->raknetInstance->send(packet);
		}
	}
	super::causeFallDamage(distance);

}

void LocalPlayer::displayClientMessage( const std::string& messageId ) {
#ifndef STANDALONE_SERVER
	minecraft->gui.displayClientMessage(messageId);
#endif
}

int LocalPlayer::startSleepInBed( int x, int y, int z ) {
	int startSleepInBedReturnValue = super::startSleepInBed(x, y, z);
#ifndef STANDALONE_SERVER
	if(startSleepInBedReturnValue == BedSleepingResult::OK)
		minecraft->setScreen(new InBedScreen());
#endif
	return startSleepInBedReturnValue;
}

void LocalPlayer::stopSleepInBed( bool forcefulWakeUp, bool updateLevelList, bool saveRespawnPoint ) {
	if(level->isClientSide) {
		PlayerActionPacket packet(PlayerActionPacket::STOP_SLEEPING, 0, 0, 0, 0, entityId);
		minecraft->raknetInstance->send(packet);
	}
#ifndef STANDALONE_SERVER
	minecraft->setScreen(NULL);
#endif
	super::stopSleepInBed(forcefulWakeUp, updateLevelList, saveRespawnPoint);
}

void LocalPlayer::openTextEdit( TileEntity* tileEntity ) {
#if !defined(STANDALONE_SERVER) && !defined(RPI)
	if(tileEntity->type == TileEntityType::Sign)
		minecraft->setScreen(new TextEditScreen((SignTileEntity*) tileEntity));
#endif
}

void LocalPlayer::updateArmorTypeHash() {
    int hash = getArmorTypeHash();
    if (hash != armorTypeHash) {
        PlayerArmorEquipmentPacket p(this);
        minecraft->raknetInstance->send(p);
        armorTypeHash = hash;
    }
}
