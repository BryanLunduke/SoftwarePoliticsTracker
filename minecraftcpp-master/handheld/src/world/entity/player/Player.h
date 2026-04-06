#ifndef NET_MINECRAFT_WORLD_ENTITY_PLAYER__Player_H__
#define NET_MINECRAFT_WORLD_ENTITY_PLAYER__Player_H__

//package net.minecraft.world.entity.player;

#include "Abilities.h"
#include "../Mob.h"
#include "../../Pos.h"
#include "../../food/SimpleFoodData.h"
#include "../../item/crafting/Recipe.h"

class Tile;
class ItemEntity;
class ItemInstance;
class Inventory;
class FillingContainer;
class FurnaceTileEntity;
class CompoundTag;
class ChestTileEntity;
class BaseContainerMenu;
class TileEntity;
class BedSleepingResult {
public:	
	static const int OK = 0;
	static const int NOT_POSSIBLE_HERE = 1;
	static const int NOT_POSSIBLE_NOW = 2;
	static const int TOO_FAR_AWAY = 3;
	static const int OTHER_PROBLEM = 4;
	static const int NOT_SAFE = 5;
};

class Player: public Mob
{
	typedef Mob super;
	typedef SynchedEntityData::TypeChar PlayerFlagIDType;

	static const int DATA_PLAYER_FLAGS_ID = 16;
	static const int DATA_BED_POSITION_ID = 17;
	static const int PLAYER_SLEEP_FLAG = 1;

public:
	static const int MAX_NAME_LENGTH = 16;
    static const int MAX_HEALTH = 20;
	static const float DEFAULT_WALK_SPEED;
	static const float DEFAULT_FLY_SPEED;
	static const int SLEEP_DURATION = 100;
	static const int WAKE_UP_DURATION = 10;

	Player(Level* level, bool isCreative);
	virtual ~Player();

	void _init();
	virtual void reset();

    static bool isPlayer(Entity* e);
    static Player* asPlayer(Entity* e);

	virtual void tick();
    void aiStep();
	void travel(float xa, float ya);

	virtual float getWalkingSpeedModifier();

    void die(Entity* source);
    void remove();
    void respawn();
    void resetPos(bool clearMore);
    Pos getRespawnPosition();
    void setRespawnPosition(const Pos& respawnPosition);
    
    bool isShootable();
    bool isCreativeModeAllowed();
	bool isPlayer();
    bool isInWall();

	virtual bool hasResource( int id );

	bool isUsingItem();
	ItemInstance* getUseItem();

	void startUsingItem(ItemInstance instance, int duration);
	void stopUsingItem();
	void releaseUsingItem();
	virtual void completeUsingItem();

	int getUseItemDuration();
	int getTicksUsingItem();

    int getScore();
    void awardKillScore(Entity* victim, int score);
	void handleEntityEvent(char id);

	virtual void take(Entity* e, int orgCount);
	//void drop();
	virtual void drop(ItemInstance* item);
	virtual void drop(ItemInstance* item, bool randomly);
	void reallyDrop(ItemEntity* thrownItem);

    bool canDestroy(Tile* tile);
	float getDestroySpeed(Tile* tile);

	int getMaxHealth();
	bool isHurt();

	bool hurt(Entity* source, int dmg);
	void hurtArmor(int dmg);
	void setArmor(int slot, const ItemInstance* item);
	ItemInstance* getArmor(int slot);
	int getArmorTypeHash();

    void interact(Entity* entity);
    void attack(Entity* entity);
	virtual ItemInstance* getCarriedItem();
	bool canUseCarriedItemWhileMoving();

	virtual void startCrafting(int x, int y, int z, int tableSize);
	virtual void startStonecutting(int x, int y, int z);

	virtual void openContainer(ChestTileEntity* container);
	virtual void openFurnace(FurnaceTileEntity* e);
	void tileEntityDestroyed( int tileEntityId );

	virtual void displayClientMessage(const std::string& messageId);
	virtual void animateRespawn();
	float getHeadHeight();

	// id == 0 -> not possible to create via serialization (yet)
	int getEntityTypeId() const { return 0; }

	int getItemInHandIcon(ItemInstance* item, int layer);
	bool isSleeping();
	virtual int startSleepInBed(int x, int y, int z);
	virtual void stopSleepInBed(bool forcefulWakeUp, bool updateLevelList, bool saveRespawnPoint);
	virtual int getSleepTimer();
	void setAllPlayersSleeping();
	float getSleepRotation();
	bool isSleepingLongEnough();
	ItemInstance* getSelectedItem();
	Inventory* inventory;
	bool hasRespawnPosition();
	virtual void openTextEdit( TileEntity* tileEntity );
    //AbstractContainerMenu inventoryMenu;
    //AbstractContainerMenu containerMenu;
    int getArmorValue();
protected:
	bool isImmobile();
	void updateAi();
	virtual void closeContainer();
	void setDefaultHeadHeight();

	void readAdditionalSaveData(CompoundTag* entityTag);
	void addAdditonalSaveData(CompoundTag* entityTag);
	
	void setBedOffset(int bedDirection);
	bool checkBed();

	static void animateRespawn(Player* player, Level* level);
	void spawnEatParticles(const ItemInstance* useItem, int count);
private:
	void touch(Entity* entity);
	
	//void eat( ItemInstance* instance );
	
public:
    char userType;
    int score;
    float oBob, bob;

    std::string name;
    int dimension;

	Abilities abilities;
	SimpleFoodData foodData;
    //Stats stats;

	BaseContainerMenu* containerMenu;

	// ok I know it's not so nice to build in RakNet dependency here, BUT I DON'T CARE! MUAHAHAHAHAHAHAHAHAHAHAHAHAHAHAHHAHAHAHAHAHAHAHAHHAHAHAHAHAHAHA
	RakNet::RakNetGUID owner;
	bool hasFakeInventory;
	Pos bedPosition;
	float bedOffsetX;
	float bedOffsetY;
	float bedOffsetZ;
protected:
	ItemInstance useItem;
	int useItemDuration;
	short sleepCounter;
	
	static const int NUM_ARMOR = 4;
private:
    Pos respawnPosition;
	bool playerHasRespawnPosition;
	bool playerIsSleeping;
	bool allPlayersSleeping;

    ItemInstance armor[NUM_ARMOR];
	//FishingHook fishing = NULL;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PLAYER__Player_H__*/
