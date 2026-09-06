#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__FurnaceTileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__FurnaceTileEntity_H__

//package net.minecraft.world.level->tile.entity;

#include "TileEntity.h"
#include "../../../Container.h"
#include "../../../item/ItemInstance.h"

class CompoundTag;
class Player;

class FurnaceTileEntity: public TileEntity,
						 public Container
{
    typedef TileEntity super;
    static const int BURN_INTERVAL = 10 * 20;
    static const int NumItems = 3;
public:
    FurnaceTileEntity();
	~FurnaceTileEntity();

	// Container
    ItemInstance* getItem(int slot);
	void setItem(int slot, ItemInstance* item);
    ItemInstance removeItem(int slot, int count);

	std::string getName() const;
	int getMaxStackSize() const;
	int getContainerSize() const;

    bool stillValid(Player* player);

    void startOpen();
    void stopOpen();

	void setContainerChanged();

	// Furnace
    void load(CompoundTag* base);
    bool save(CompoundTag* base);
	bool shouldSave();

	int getBurnProgress(int max);
    int getLitProgress(int max);

	bool isLit();
	bool isFinished();
	bool isSlotEmpty( int slot );

	void tick();

    void burn();

	static bool isFuel(const ItemInstance& itemInstance);
	static int getBurnDuration(const ItemInstance& itemInstance);
private:
    bool canBurn();
	
public:
    int litTime;
    int litDuration;
    int tickCount;
    ItemInstance items[NumItems];

	static const int SLOT_INGREDIENT = 0;
	static const int SLOT_FUEL = 1;
	static const int SLOT_RESULT = 2;
private:
	bool _canBeFinished;
	bool finished;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__FurnaceTileEntity_H__*/
