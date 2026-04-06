#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__TileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__TileEntity_H__

//package net.minecraft.world.level->tile.entity;

#include "../Tile.h"
#include <map>
#include <vector>
#include "TileEntityRendererId.h"

class Level;
class TileEntity;
class CompoundTag;
class Packet;

class TileEntityType {
public:
    static const int Undefined = 0;
    static const int Furnace = 1;
	static const int Chest = 2;
	static const int NetherReactor = 3;
	static const int Sign = 4;
};

class TileEntityFactory {
public:
	static TileEntity* createTileEntity(int type);
};

class TileEntity
{
    typedef std::map<std::string, int> MapIdType;
    typedef std::map<int, std::string> MapTypeId;

	static int _runningId;
public:
	static void initTileEntities();
	static void teardownTileEntities();

	TileEntity(int tileEntityType);
	virtual ~TileEntity() {}

	virtual bool shouldSave() = 0;
    virtual void load(CompoundTag* tag);
    virtual bool save(CompoundTag* tag);

    virtual void tick() {}
	virtual bool isFinished();

    static TileEntity* loadStatic(CompoundTag* tag);

    virtual void setLevelAndPos(Level* level, int x, int y, int z);

    int getData();
    void setData(int data);

    void setChanged();

    float distanceToSqr(float xPlayer, float yPlayer, float zPlayer);

    Tile* getTile();

    virtual Packet* getUpdatePacket();

    bool isRemoved() const;
    void setRemoved();
    void clearRemoved();

    virtual void triggerEvent(int b0, int b1);
    virtual void clearCache();

	bool isType(int Type);
	static bool isType(TileEntity* te, int Type);
public:
    Level*  level;
    int     x, y, z;

    int     data;
    int     type;
	int		runningId;

	bool	clientSideOnly;

	TileEntityRendererId rendererId;

protected:
    Tile*  tile;
    bool remove;

private:
    static MapIdType idClassMap;
    static MapTypeId classIdMap;
    static void setId(int type, const std::string& id);
};

int partitionTileEntities(const std::vector<TileEntity*>& in, std::vector<TileEntity*>& keep, std::vector<TileEntity*>& dontKeep);

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__TileEntity_H__*/
