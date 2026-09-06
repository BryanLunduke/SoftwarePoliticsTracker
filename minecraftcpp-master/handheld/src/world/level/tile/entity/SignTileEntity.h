#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__SignTileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__SignTileEntity_H__

//package net.minecraft.world.level.tile.entity;

/* import net.minecraft.network.packet.* */

#include "../../../../nbt/CompoundTag.h"

#include "TileEntity.h"

class SignTileEntity: public TileEntity
{
	typedef TileEntity super;
public:
	static const int MAX_LINE_LENGTH = 15;
    static const int NUM_LINES = 4;

	SignTileEntity();

    /*@Override*/
    bool save(CompoundTag* tag);
    /*@Override*/
    void load(CompoundTag* tag);
    bool shouldSave();

    bool isEditable();
    void setEditable(bool isEditable);
	void setLevelAndPos(Level* level, int x, int y, int z);

	Packet* getUpdatePacket();

    std::string messages[NUM_LINES];
    int selectedLine;
private:
    bool editable;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__SignTileEntity_H__*/
