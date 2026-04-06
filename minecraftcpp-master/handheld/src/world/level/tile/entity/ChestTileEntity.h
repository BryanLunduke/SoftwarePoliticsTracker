#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__ChestTileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__ChestTileEntity_H__

//package net.minecraft.world.level->tile.entity;

#include "TileEntity.h"
#include "../../../inventory/FillingContainer.h"
#include "../../../item/ItemInstance.h"
#include <string>

class CompoundTag;
class Player;

/* import com.mojang.nbt.* */

class ChestTileEntity:	public TileEntity,
						public FillingContainer
{
	typedef TileEntity super;
public:
	ChestTileEntity();

    int getContainerSize() const;
	int getMaxStackSize() const;
	std::string getName() const;

    ItemInstance* getItem(int slot);
	//void setItem(int slot, ItemInstance* item);
    //ItemInstance removeItem(int slot, int count);
    //ItemInstance removeItemNoUpdate(int slot);

	bool shouldSave();
    void load(CompoundTag* base);
    bool save(CompoundTag* base);

    bool stillValid(Player* player);

    void clearCache();
    void checkNeighbors();

    /*@Override*/
    void tick();

    void triggerEvent(int b0, int b1);

    void startOpen();
    void stopOpen();

    /*@Override*/
    void setRemoved();

	bool hasCheckedNeighbors;
	ChestTileEntity* n;
	ChestTileEntity* e;
	ChestTileEntity* w;
	ChestTileEntity* s;

	float openness, oOpenness;
	int openCount;

private:
	static const int ItemsSize = 9*3;
	int tickInterval;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__ChestTileEntity_H__*/
