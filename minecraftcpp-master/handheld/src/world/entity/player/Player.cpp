#include "Player.h"
#include "Inventory.h"
#include "../item/ItemEntity.h"
#include "../../level/Level.h"
#include "../../item/ItemInstance.h"
#include "../../item/BowItem.h"
#include "../../inventory/BaseContainerMenu.h"
#include "../../../nbt/CompoundTag.h"

#include "../../../network/RakNetInstance.h"
#include "../../../network/packet/AnimatePacket.h"
#include "../../inventory/FurnaceMenu.h"
#include "../SharedFlags.h"
#include "../../level/tile/BedTile.h"
#include "../../Direction.h"
#include "../EntityEvent.h"
#include "../../Difficulty.h"
#include "../../item/ArmorItem.h"

const float Player::DEFAULT_WALK_SPEED = 0.1f;
const float Player::DEFAULT_FLY_SPEED = 0.02f;

// @todo: Move out to ArmorInventory
static ListTag* saveArmor(ItemInstance* armor);
static void loadArmor(ItemInstance* armor, ListTag* listTag);

Player::Player(Level* level, bool isCreative)
:	super(level),
	userType(0),
	playerHasRespawnPosition(false),
	hasFakeInventory(false),
	containerMenu(NULL),
	useItemDuration(0),
	playerIsSleeping(false),
	sleepCounter(0),
	bedOffsetX(0),
	bedOffsetY(0),
	bedOffsetZ(0),
	respawnPosition(0, -1, 0),
	allPlayersSleeping(false)
{
	canRemove = false;

	_init();
	entityRendererId = ER_PLAYER_RENDERER;

	autoSendPosRot = false;
	inventory = new Inventory(this, isCreative);

	//inventoryMenu = /*new*/ InventoryMenu(inventory, !level.isOnline);
    //containerMenu = inventoryMenu;

	heightOffset = 1.62f;

	Pos spawnPos = level->getSharedSpawnPos();
    this->moveTo((float)spawnPos.x + 0.5f, (float)(spawnPos.y + 1), (float)spawnPos.z + 0.5f, 0, 0);

    health = MAX_HEALTH;
    modelName = "humanoid";
    rotOffs = 180;
    flameTime = 20;

    textureName = "mob/char.png";
	entityData.define(DATA_PLAYER_FLAGS_ID, (PlayerFlagIDType) 0);
	entityData.define(DATA_BED_POSITION_ID, Pos());
	//entityData.define(DATA_PLAYER_RUNNING_ID, (SynchedEntityData::TypeChar) 0);
}

Player::~Player() {
	delete inventory;
}
bool Player::isSleeping() {
	return playerIsSleeping;
}
int Player::startSleepInBed( int x, int y, int z ) {
	if(!level->isClientSide) {
		if(isSleeping() || !isAlive()) {
			return BedSleepingResult::OTHER_PROBLEM;
		}
		if(Mth::abs(this->x - x) > 3 || Mth::abs(this->y - y) > 4 || Mth::abs(this->z - z) > 3) {
			return BedSleepingResult::TOO_FAR_AWAY;
		}
		if(level->dimension->isNaturalDimension()) {
			return BedSleepingResult::NOT_POSSIBLE_HERE;
		}
		if(level->isDay()) {
			return BedSleepingResult::NOT_POSSIBLE_NOW;
		}
		float hRange = 8;
		float vRange = 5;
		EntityList monsters;
		level->getEntitiesOfClass(MobTypes::BaseEnemy, AABB(x- hRange, y - vRange, z - hRange, x + hRange, y + vRange, z + hRange), monsters);
		if(!monsters.empty()) {
			return BedSleepingResult::NOT_SAFE;
		}
	}

	setSize(0.2f, 0.2f);
	heightOffset = 0.2f;
	if(level->hasChunkAt(x, y, z)) {
		int data = level->getData(x, y, z);
		int direction = BedTile::getDirection(data);
		float xo = 0.5f, zo = 0.5f;
		switch(direction) {
		case Direction::SOUTH:
			zo = 0.9f;
			break;
		case Direction::NORTH:
			zo = 0.1f;
			break;
		case Direction::WEST:
			xo = 0.1f;
			break;
		case Direction::EAST:
			xo = 0.9f;
			break;
		}
		setBedOffset(direction);
		setPos(x + xo, y + 15.0f / 16.0f, z + zo);
	} else {
		setPos(x + 0.5f, y + 1.0f / 16.0f, z + 0.5f);
	}
	playerIsSleeping = true;
	sleepCounter = 0;
	bedPosition = Pos(x, y, z);
	xd = zd = yd = 0;
	if(!level->isClientSide) {
		level->updateSleepingPlayerList();
	}
	entityData.set<Pos>(DATA_BED_POSITION_ID, bedPosition);
	entityData.setFlag<SharedFlagsInformation::SharedFlagsInformationType>(DATA_PLAYER_FLAGS_ID, PLAYER_SLEEP_FLAG);
	return BedSleepingResult::OK;
}

void Player::stopSleepInBed( bool forcefulWakeUp, bool updateLevelList, bool saveRespawnPoint ) {
	if(!isSleeping())
		return;
	setSize(0.6f, 1.8f);
	setDefaultHeadHeight();
	Pos standUp = bedPosition;
	if(level->getTile(int(bedPosition.x), int(bedPosition.y), int(bedPosition.z)) == Tile::bed->id) {
		BedTile::setOccupied(level, int(bedPosition.x), int(bedPosition.y), int(bedPosition.z), false);
		bool foundStandUpPosition = BedTile::findStandUpPosition(level, int(bedPosition.x), int(bedPosition.y), int(bedPosition.z), 0, standUp);
		if(!foundStandUpPosition) {
			standUp = Pos(bedPosition.x, bedPosition.y, bedPosition.z);
		}
		setPos(standUp.x + 0.5f, standUp.y + heightOffset + 0.1f, standUp.z + 0.5f);
	}
	playerIsSleeping = false;
	if(!level->isClientSide && updateLevelList) {
		level->updateSleepingPlayerList();
	}
	if(forcefulWakeUp) {
		sleepCounter = 0;
	} else {
		sleepCounter = SLEEP_DURATION;
	}
	// Quick fix to make the spawn position always saved, not sure if we always want to save this position but I like it.
	if(true || saveRespawnPoint) {
		Pos newRespawnPos;
		BedTile::findStandUpPosition(level, bedPosition.x, bedPosition.y, bedPosition.z, 0, newRespawnPos);
		setRespawnPosition(newRespawnPos);
	}
	entityData.clearFlag<SharedFlagsInformation::SharedFlagsInformationType>(DATA_PLAYER_FLAGS_ID, PLAYER_SLEEP_FLAG);
	allPlayersSleeping = false;
}

int Player::getSleepTimer() {
	return allPlayersSleeping ? sleepCounter : 0;
}
void Player::setAllPlayersSleeping() {
	sleepCounter = 0;
	allPlayersSleeping = true;
}
void Player::setBedOffset( int bedDirection ) {
	bedOffsetX = 0;
	bedOffsetZ = 0;
	switch(bedDirection) {
	case Direction::SOUTH:
		bedOffsetZ = -1.8f;
		break;
	case Direction::NORTH:
		bedOffsetZ = 1.8f;
		break;
	case Direction::WEST:
		bedOffsetX = 1.8f;
		break;
	case Direction::EAST:
		bedOffsetX = -1.8f;
		break;
	}
}
bool Player::isSleepingLongEnough() {
	return isSleeping() && sleepCounter >= SLEEP_DURATION;
}

float Player::getSleepRotation() {
	if(isSleeping()) {
		int data = level->getData(bedPosition.x, bedPosition.y, bedPosition.z);
		int direction = BedTile::getDirection(data);
		switch(direction) {
		case Direction::SOUTH:
			return 90;
		case Direction::WEST:
			return 0;
		case Direction::NORTH:
			return 270;
		case Direction::EAST:
			return 180;
		}
	}
	return 0;
}

bool Player::checkBed() {
	return (level->getTile(bedPosition.x, bedPosition.y, bedPosition.z) == Tile::bed->id);
}

void Player::tick() {
	bool shouldSleep = entityData.getFlag<SharedFlagsInformation::SharedFlagsInformationType>(DATA_PLAYER_FLAGS_ID, PLAYER_SLEEP_FLAG);
	if(shouldSleep != isSleeping()) {
		if(isSleeping()) {
			stopSleepInBed(true, true, true);
		} else {
			bedPosition = entityData.getPos(DATA_BED_POSITION_ID);
			startSleepInBed(bedPosition.x, bedPosition.y, bedPosition.z);
		}
	}
	if(isSleeping()) {
		sleepCounter++;
		if(sleepCounter > SLEEP_DURATION) {
			sleepCounter = SLEEP_DURATION;
		}
		if(!level->isClientSide) {
			if(!checkBed()) {
				stopSleepInBed(true, true, false);
			} else if(level->isDay()) {
				stopSleepInBed(false, true, true);
			}
		} 
	} else if(sleepCounter > 0) {
		sleepCounter++;
		if(sleepCounter >= (SLEEP_DURATION + WAKE_UP_DURATION)) {
			sleepCounter = 0;
		}
	}
    super::tick();

	if (!level->isClientSide) {
		foodData.tick(this);
	//	if (containerMenu != NULL && !containerMenu->stillValid(this)) {
	//		closeContainer();
	//	}
	}
}

int Player::getMaxHealth() {
	return MAX_HEALTH;
}

//
// Use items
//
bool Player::isUsingItem() {
	return !useItem.isNull();
}

ItemInstance* Player::getUseItem() {
	return &useItem;
}

void Player::spawnEatParticles(const ItemInstance* useItem, int count) {
	if (useItem->getUseAnimation() == UseAnim::drink) {
		level->playSound(this, "random.drink", 0.5f, level->random.nextFloat() * 0.1f + 0.9f);
	}
	else if (useItem->getUseAnimation() == UseAnim::eat) {
		for (int i = 0; i < count; i++) {
			const float xx = -xRot * Mth::PI / 180;
			const float yy = -yRot * Mth::PI / 180;
			Vec3 d((random.nextFloat() - 0.5f) * 0.1f, Mth::random() * 0.1f + 0.1f, 0);
			d.xRot(xx);
			d.yRot(yy);
			Vec3 p((random.nextFloat() - 0.5f) * 0.3f, -random.nextFloat() * 0.6f - 0.3f, 0.6f);
			p.xRot(xx);
			p.yRot(yy);
			p = p.add(x, y + getHeadHeight(), z);
			level->addParticle(PARTICLETYPE(iconcrack), p.x, p.y, p.z, d.x, d.y + 0.05f, d.z, useItem->getItem()->id);
		}
		level->playSound(this, "random.eat", .5f + .5f * random.nextInt(2), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
	}
}

void Player::startUsingItem(ItemInstance instance, int duration) {
	if(instance == useItem) return;
	useItem = instance;
	useItemDuration = duration;
	if(!level->isClientSide) {
		setSharedFlag(SharedFlagsInformation::FLAG_USINGITEM, true);
	}
}

void Player::stopUsingItem() {
	if(getCarriedItem() != NULL && useItem.id == getCarriedItem()->id)
		getCarriedItem()->setAuxValue(useItem.getAuxValue());
	useItem.setNull();
	useItemDuration = 0;
	if(!level->isClientSide) {
		setSharedFlag(SharedFlagsInformation::FLAG_USINGITEM, false);
	}
}

void Player::releaseUsingItem() {
	if(!useItem.isNull()) {
		useItem.releaseUsing(level, this, useItemDuration);
	}
	stopUsingItem();
}

void Player::completeUsingItem() {
	if(!useItem.isNull()) {
		spawnEatParticles(&useItem, 10);

		// Check if the item is valid, and if we should overwrite the
		// inventory item afterwards.
		ItemInstance* selected = inventory->getSelected();
		bool doOverwrite = selected && ItemInstance::matches(&useItem, selected);

		ItemInstance itemInstance = useItem.useTimeDepleted(level, this);

		if (doOverwrite) {
			*selected = useItem;
			if (selected->count == 0)
				inventory->clearSlot(inventory->selected);
		}

		/*
		int oldCount = useItem.count;
		if (!itemInstance.matches(&useItem)) {
			ItemInstance* selected = inventory->getSelected();
			if (ItemInstance::matches(&useItem, selected)) {
				if (selected) *selected = itemInstance;
				if (itemInstance.count == 0) {
					inventory->clearSlot(inventory->selected);
				}
			}
		}
		*/

		stopUsingItem();
	}
}

int Player::getUseItemDuration() {
	return useItemDuration;
}

int Player::getTicksUsingItem() {
	if(isUsingItem()) {
		return useItem.getUseDuration() - useItemDuration;	
	}
	return 0;
}

void Player::travel(float xa, float ya) {
	if (abilities.flying) {
		float ydo = yd;
		float ofs = flyingSpeed;
		flyingSpeed = 0.05f;
		super::travel(xa, ya);
		yd = ydo * 0.6f;
		flyingSpeed = ofs;
	} else {
		super::travel(xa, ya);
	}
}

/*protected*/
bool Player::isImmobile() {
    return health <= 0 || isSleeping();
}

/*protected*/
void Player::closeContainer() {
	containerMenu = NULL;
    //containerMenu = inventoryMenu;
}

void Player::resetPos(bool clearMore) {
    if(!isSleeping()) {
		heightOffset = 1.62f;
		setSize(0.6f, 1.8f);
		super::resetPos(clearMore);
	}
	invisible = false;
	
	if (clearMore) {
		health = getMaxHealth();
		deathTime = 0;
		playerIsSleeping = false;
	}
}

/*protected*/
void Player::updateAi() {
	updateAttackAnim();
}

int Player::getItemInHandIcon(ItemInstance* item, int layer) {
	int icon = item->getIcon();
	if(useItem.id != 0 && item->id == Item::bow->id) {
		int ticksHeld = (item->getUseDuration() - useItemDuration);
		if(ticksHeld >= BowItem::MAX_DRAW_DURATION - 2) {
			return 5 + 8 * 16;
		}
		if(ticksHeld > (2 * BowItem::MAX_DRAW_DURATION) / 3) {
			return 5 + 7*16;
		}
		if(ticksHeld > 0) {
			return 5 + 6 * 16;
		}
	}
	return icon;
}

void Player::aiStep() {
    if (level->difficulty == Difficulty::PEACEFUL && health < MAX_HEALTH) {
        if (tickCount % (12 * SharedConstants::TicksPerSecond) == 0) heal(1);
    }
    //inventory.tick();
    oBob = bob;
	// moved the super::aiStep() part to the local player

    float tBob = (float) Mth::sqrt(xd * xd + zd * zd);
    float tTilt = (float) Mth::atan(-yd * 0.2f) * 15.f;
    if (tBob > 0.1f) tBob = 0.1f;
    if (!onGround || health <= 0) tBob = 0;
    if (onGround || health <= 0) tTilt = 0;
    bob += (tBob - bob) * 0.4f;
    tilt += (tTilt - tilt) * 0.8f;

    if (health > 0) {
        EntityList& entities = level->getEntities(this, bb.grow(1, 0, 1));
        for (unsigned int i = 0; i < entities.size(); i++) {
            Entity* e = entities[i];
            if (!e->removed) {
                touch(e);
            }
        }
    }
}

/*private*/
void Player::touch(Entity* entity) {
    entity->playerTouch(this);
}

int Player::getScore() {
    return score;
}

void Player::die(Entity* source) {
    super::die(source);
    this->setSize(0.2f, 0.2f);
    setPos(x, y, z);
    yd = 0.1f;

	//inventory->dropAll(level->isClientSide);

    if (source != NULL) {
        xd = -(float) Mth::cos((hurtDir + yRot) * Mth::PI / 180) * 0.1f;
        zd = -(float) Mth::sin((hurtDir + yRot) * Mth::PI / 180) * 0.1f;
    } else {
        xd = zd = 0;
    }
    this->heightOffset = 0.1f;
}

void Player::reset() {
	super::reset();
	this->_init();
}

void Player::_init() {
	oBob = bob = 0;
	swinging = 0;
	swingTime = 0;
	score = 0;
}

float Player::getWalkingSpeedModifier() {
	return 1.0f;
}


void Player::awardKillScore(Entity* victim, int score) {
    this->score += score;
}

bool Player::isShootable() {
    return true;
}

bool Player::isCreativeModeAllowed() {
    return true;
}

//void Player::drop() {
//    //drop(inventory.removeItem(inventory.selected, 1), false);
//}

void Player::drop(ItemInstance* item) {
    drop(item, false);
}

void Player::drop(ItemInstance* item, bool randomly) {
    if (item == NULL || item->isNull())
        return;

    ItemEntity* thrownItem = new ItemEntity(level, x, y - 0.3f + getHeadHeight(), z, *item);
	{ //@todo:itementity
		delete item;
		item = NULL;
	}
    thrownItem->throwTime = 20 * 2;

    float pow = 0.1f;
    if (randomly) {
        float _pow = random.nextFloat() * 0.5f;
        float dir = random.nextFloat() * Mth::PI * 2;
        thrownItem->xd = -Mth::sin(dir) * _pow;
        thrownItem->zd = Mth::cos(dir) * _pow;
        thrownItem->yd = 0.2f;

    } else {
        pow = 0.3f;
        thrownItem->xd = -Mth::sin(yRot / 180 * Mth::PI) * Mth::cos(xRot / 180 * Mth::PI) * pow;
        thrownItem->zd = Mth::cos(yRot / 180 * Mth::PI) * Mth::cos(xRot / 180 * Mth::PI) * pow;
        thrownItem->yd = -Mth::sin(xRot / 180 * Mth::PI) * pow + 0.1f;
        pow = 0.02f;

        float dir = random.nextFloat() * Mth::PI * 2;
        pow *= random.nextFloat();
        thrownItem->xd += Mth::cos(dir) * pow;
        thrownItem->yd += (random.nextFloat() - random.nextFloat()) * 0.1f;
        thrownItem->zd += Mth::sin(dir) * pow;
    }

    reallyDrop(thrownItem);
}

/*protected*/
void Player::reallyDrop(ItemEntity* thrownItem) {
    level->addEntity(thrownItem);
}

float Player::getDestroySpeed(Tile* tile) {
    float speed = inventory->getDestroySpeed(tile);
    //if (isUnderLiquid(Material.water)) speed /= 5;
    //if (!onGround) speed /= 5;
    return speed;
}

bool Player::canDestroy(Tile* tile) {
	return inventory->canDestroy(tile);
}

//@SuppressWarnings("unchecked")
void Player::readAdditionalSaveData(CompoundTag* entityTag) {
    super::readAdditionalSaveData(entityTag);

	if (entityTag->contains("Inventory", Tag::TAG_List)) {
		ListTag* inventoryList = entityTag->getList("Inventory");
		inventory->load(inventoryList);
	}
	if (entityTag->contains("Armor", Tag::TAG_List)) {
		loadArmor(armor, entityTag->getList("Armor"));
	}

    dimension = entityTag->getInt("Dimension");

	//return;
	if(entityTag->contains("Sleeping") && entityTag->contains("SleepTimer")
		&& entityTag->contains("BedPositionX") && entityTag->contains("BedPositionY") && entityTag->contains("BedPositionZ")) {
			playerIsSleeping = entityTag->getBoolean("Sleeping");
			sleepCounter = entityTag->getShort("SleepTimer");
			bedPosition = Pos(entityTag->getInt("BedPositionX"), entityTag->getInt("BedPositionY"), entityTag->getInt("BedPositionZ"));
	} else {
		playerIsSleeping = false;
		bedPosition = Pos(0,0,0);
	}

	if(!playerIsSleeping) {
		stopSleepInBed(true, true, false);
		entityData.clearFlag<SharedFlagsInformation::SharedFlagsInformationType>(DATA_PLAYER_FLAGS_ID, PLAYER_SLEEP_FLAG);
	} else {
		playerIsSleeping = false;
		startSleepInBed(bedPosition.x, bedPosition.y, bedPosition.z);
		entityData.setFlag<SharedFlagsInformation::SharedFlagsInformationType>(DATA_PLAYER_FLAGS_ID, PLAYER_SLEEP_FLAG);
	}
	entityData.set<Pos>(DATA_BED_POSITION_ID, bedPosition);
    if (entityTag->contains("SpawnX") && entityTag->contains("SpawnY") && entityTag->contains("SpawnZ")) {
		respawnPosition.set(entityTag->getInt("SpawnX"), entityTag->getInt("SpawnY"), entityTag->getInt("SpawnZ"));
    }
	playerHasRespawnPosition = respawnPosition.y >= 0;
}

void Player::addAdditonalSaveData(CompoundTag* entityTag) {
    super::addAdditonalSaveData(entityTag);
	
	ListTag* inventoryTag = inventory->save(new ListTag());
 //   if (inventoryTag->size() > 0)
		entityTag->put("Inventory", inventoryTag);
	//else
	//	delete inventoryTag;

	ListTag* armorTag = saveArmor(armor);
	entityTag->put("Armor", armorTag);

    entityTag->putInt("Dimension", dimension);
	//return;
	
	entityTag->putBoolean("Sleeping", isSleeping());
	entityTag->putShort("SleepTimer", sleepCounter);
	entityTag->putInt("BedPositionX", bedPosition.x);
	entityTag->putInt("BedPositionY", bedPosition.y);
	entityTag->putInt("BedPositionZ", bedPosition.z);
    
	entityTag->putInt("SpawnX", respawnPosition.x);
	entityTag->putInt("SpawnY", respawnPosition.y);
	entityTag->putInt("SpawnZ", respawnPosition.z);
}

//static Pos getRespawnPosition(Level level, CompoundTag entityTag) {
//    if (entityTag.contains("SpawnX") && entityTag.contains("SpawnY") && entityTag.contains("SpawnZ")) {
//        return /*new*/ Pos(entityTag.getInt("SpawnX"), entityTag.getInt("SpawnY"), entityTag.getInt("SpawnZ"));
//    }
//    return level.getSharedSpawnPos();
//}

void Player::startCrafting(int x, int y, int z, int tableSize) {
}

void Player::startStonecutting(int x, int y, int z) {
}

void Player::openFurnace(FurnaceTileEntity* e) {
}

void Player::take(Entity* e, int orgCount) {
}

float Player::getHeadHeight() {
    return 0.12f; // heightOffset; // 0.12f;
}

/*protected*/
void Player::setDefaultHeadHeight() {
    heightOffset = 1.62f;
}

bool Player::isHurt() {
	return health > 0 && health < getMaxHealth();
}

bool Player::hurt(Entity* source, int dmg) {
	if (abilities.invulnerable) return false;

	noActionTime = 0;
    if (health <= 0) return false;
	if(isSleeping() && !level->isClientSide) {
		stopSleepInBed(true, true, false);
	}

    if (source != NULL && (source->getCreatureBaseType() == MobTypes::BaseEnemy
						|| source->getEntityTypeId() == EntityTypes::IdArrow)) {

		if (source->isMob() && level->adventureSettings.noMvP)
			return false;

        if (level->difficulty == Difficulty::PEACEFUL) dmg = 0;
        else if (level->difficulty == Difficulty::EASY) dmg = dmg / 3 + 1;
        else if (level->difficulty == Difficulty::HARD) dmg = dmg * 3 / 2;
    }

   if (dmg == 0) return false;

 //   Entity* attacker = source;
 //   //if (attacker instanceof Arrow) {
 //   //    if (((Arrow) attacker).owner != NULL) {
 //   //        attacker = ((Arrow) attacker).owner;
 //   //    }
 //   //}
    return super::hurt(source, dmg);
}

void Player::interact(Entity* entity) {
    if (entity->interact(this)) return;
	ItemInstance* item = inventory->getSelected();
	if (item != NULL && entity->isMob()) {
        item->interactEnemy((Mob*)entity);
        if (item->count <= 0) {
            //item.snap(this);
            inventory->clearSlot(inventory->selected);
        }
    }
}

//void Player::swing() {
	//LOGI("swinging: %d\n", swinging);
//	if (!swinging || swingTime >= 3 || swingTime < 0) {
//		swingTime = -1;
//		swinging = true;
//	}

		//level->raknetInstance->send(owner, new AnimatePacket(AnimatePacket::Swing, this));
	//}
//}

void Player::attack(Entity* entity) {
	int dmg = inventory->getAttackDamage(entity);
    if (dmg > 0) {
        entity->hurt(this, dmg);
        ItemInstance* item = inventory->getSelected();
        if (item != NULL && entity->isMob() && abilities.instabuild != true) {
            item->hurtEnemy((Mob*) entity);
            if (item->count <= 0) {
                //item->snap(this);
                inventory->clearSlot(inventory->selected);
            }
        }
    }
}

void Player::respawn() {
}

/*protected static*/
void Player::animateRespawn(Player* player, Level* level) {
    //for (int i = 0; i < 45; i++) {
    //    float angle = i * Mth::PI * 4.0f / 25.0f;
    //    float xo = Mth::cos(angle) * .7f;
    //    float zo = Mth::sin(angle) * .7f;
    //}
}

/*virtual*/
void Player::animateRespawn() {}

//void carriedChanged(ItemInstance carried) {
//}
ItemInstance* Player::getCarriedItem() {
	return inventory->getSelected();
}

void Player::remove() {
	invisible = true;
    super::remove();
}

//@Override
bool Player::isInWall() {
    return super::isInWall();
}

bool Player::hasResource( int id ) {
	return inventory->hasResource(id);
}

/**
    * This method is currently only relevant to client-side players. It will
    * try to load the messageId from the language file and display it to the
    * client.
    */
void Player::displayClientMessage(const std::string& messageId) {
}

Pos Player::getRespawnPosition() {
    return respawnPosition;
}

void Player::setRespawnPosition(const Pos& respawnPosition) { // @attn WARNING CHECK THIS THROUGH @fix @todo: rewrite
	if (respawnPosition.y < 0) {
		playerHasRespawnPosition = false;
	}
	else {
		playerHasRespawnPosition = true;
	}
	this->respawnPosition = respawnPosition;
}

bool Player::isPlayer() { return true; }

/*static*/
bool Player::isPlayer( Entity* e ) {
    return e && e->isPlayer();
}

Player* Player::asPlayer( Entity* e ) {
    return isPlayer(e)? (Player*) e : NULL;
}

void Player::openContainer(ChestTileEntity* container)
{
}

void Player::tileEntityDestroyed( int tileEntityId ) {

	//LOGI("TileEntityDestroyed, container: %p, %p\n", this, containerMenu);

	if (!containerMenu) return;

	//LOGI("TileEntityDestroyed, id: %d, %p, %d\n", this, tileEntityId, ((FurnaceMenu*)containerMenu)->furnaceTileEntityId);

	if (containerMenu->tileEntityDestroyedIsInvalid(tileEntityId))
		closeContainer();
}

bool Player::canUseCarriedItemWhileMoving() {
	ItemInstance* item = getCarriedItem();
	return item &&
		(	item->id == Item::bow->id
		||	item->getItem()->isFood());
}

void Player::handleEntityEvent( char id ) {
	if (id == EntityEvent::USE_ITEM_COMPLETE) {
		completeUsingItem();
	} else {
		super::handleEntityEvent(id);
	}
}

ItemInstance* Player::getSelectedItem() {
	return inventory->getSelected();
}

bool Player::hasRespawnPosition(){
	return playerHasRespawnPosition;
}

void Player::openTextEdit( TileEntity* tileEntity ) {
	// Do nothing on the server, this is a client thing :)
}

//
// Armor code. Move this out to an ArmorInventory
//
static ListTag* saveArmor(ItemInstance* armor) {
	ListTag* listTag = new ListTag();
	for (int i = 0; i < 4; ++i) {
		CompoundTag* tag = new CompoundTag();
		armor[i].save(tag);
		listTag->add(tag);
	}
	return listTag;
}

static void loadArmor(ItemInstance* armor, ListTag* listTag) {
	if (!listTag)
		return;

	const int count = Mth::Min(4, listTag->size());
	for (int i = 0; i < count; ++i) {
		Tag* tag = listTag->get(i);
		if (tag->getId() != Tag::TAG_Compound) continue;

		armor[i].load((CompoundTag*) tag);
	}
}

ItemInstance* Player::getArmor(int slot) {
	if (slot < 0 || slot >= NUM_ARMOR)
		return NULL;

	if (armor[slot].isNull())
		return NULL;

	return &armor[slot];
}

void Player::setArmor(int slot, const ItemInstance* item) {
	if (item == NULL)
		armor[slot].setNull();
	else {
		armor[slot] = *item;
	}
}

void Player::hurtArmor( int dmg ) {
	dmg = Mth::Max(1, dmg / 4);

	for (int i = 0; i < NUM_ARMOR; i++) {
		ItemInstance& item = armor[i];
		if (!ItemInstance::isArmorItem(&item))
			continue;

		item.hurt(dmg);
		if (item.count == 0) {
			item.setNull();
		}
	}
}

int Player::getArmorTypeHash() {
	return	(armor[0].id      ) +
			(armor[1].id <<  8) +
			(armor[2].id << 16) +
			(armor[3].id << 24);
}

int Player::getArmorValue() {
    int val = 0;

    for (int i = 0; i < NUM_ARMOR; i++) {
        ItemInstance& item = armor[i];
        if (!ItemInstance::isArmorItem(&item))
            continue;

        int baseProtection = ((ArmorItem*) item.getItem())->defense;
        val += baseProtection;
    }
    return val;
}

