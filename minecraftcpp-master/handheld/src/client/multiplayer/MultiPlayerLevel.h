#ifndef NET_MINECRAFT_CLIENT_MULTIPLAYER__MultiPlayerLevel_H__
#define NET_MINECRAFT_CLIENT_MULTIPLAYER__MultiPlayerLevel_H__

#include "../../world/Pos.h"
#include "../../world/level/Level.h"
//#include "../../network/packet/DisconnectPacket.h"
#include "../../world/entity/Entity.h"
#include "../../world/level/chunk/ChunkSource.h"
#include "../../world/level/dimension/Dimension.h"
//#include "../../world/level/storage/MockedLevelStorage.h"

#include <set>
#include <map>
#include <list>

class MultiPlayerLevel: public Level
{
    typedef Level super;
    typedef std::map<int, Entity*> EntityIdMap;
	typedef std::set<Entity*> EntitySet;

    static const int TICKS_BEFORE_RESET = SharedConstants::TicksPerSecond * 4;

    class ResetInfo {
    public:
        int x, y, z, ticks, tile, data;

        ResetInfo(int x, int y, int z, int tile, int data)
        :   x(x),
            y(y),
            z(z),
            ticks(TICKS_BEFORE_RESET),
            tile(tile),
            data(data)
        {}
    };
	typedef std::list<ResetInfo> ResetInfoList;

public:
	MultiPlayerLevel(LevelStorage* levelStorage, const std::string& levelName, const LevelSettings& settings, int levelVersion)
	:	super(levelStorage, levelName, settings, levelVersion)
	{
		//super(new MockedLevelStorage(), "MpServer", Dimension.getNew(dimension), levelSettings);
		//this->difficulty = difficulty;
		//setSpawnPos(new Pos(8, 64, 8));
		//this.savedDataStorage = connection.savedDataStorage;
	}

	//@todo: destructor (to <delete> Entities)

    void tick() {
        if (adventureSettings.doTickTime) {
            setTime(getTime() + 1);
        }
		updateSkyDarken();

        for (int i = 0; i < 10 && !reEntries.empty(); i++) {
			EntitySet::iterator it = reEntries.begin();
            Entity* e = *it;//reEntries.iterator().next();
            reEntries.erase(it);

			//if (!entities.contains(e)) addEntity(e);
			EntityList::iterator jt = std::find(entities.begin(), entities.end(), e);
			if (jt == entities.end())
				addEntity(e);
        }

        for (ResetInfoList::iterator it = updatesToReset.begin(); it != updatesToReset.end();) {
            ResetInfo& r = *it;
            if (--r.ticks == 0) {
                super::setTileAndDataNoUpdate(r.x, r.y, r.z, r.tile, r.data);
                super::sendTileUpdated(r.x, r.y, r.z);
                it = updatesToReset.erase(it);
            } else
                ++it;
        }
    }

    void clearResetRegion(int x0, int y0, int z0, int x1, int y1, int z1) {
        for (ResetInfoList::iterator it = updatesToReset.begin(); it != updatesToReset.end();) {
            ResetInfo& r = *it;
            if (r.x >= x0 && r.y >= y0 && r.z >= z0 && r.x <= x1 && r.y <= y1 && r.z <= z1) {
                it = updatesToReset.erase(it);
            } else
                ++it;
        }
    }

    void validateSpawn() {
        setSpawnPos(Pos(8, 64, 8));
    }

    void addToTickNextTick(int x, int y, int z, int tileId, int tickDelay) {
    }

    bool tickPendingTicks(bool force) {
        return false;
    }

	//@todo
    //void setChunkVisible(int x, int z, bool visible) { 
    //    if (visible) chunkCache.create(x, z);
    //    else chunkCache.drop(x, z);
    //    if (!visible) {
    //        this->setTilesDirty(x * 16, 0, z * 16, x * 16 + 15, depth, z * 16 + 15);
    //    }
    //}

    bool addEntity(Entity* e) {
        bool ok = super::addEntity(e);
        forced.insert(e);

        if (!ok) {
            reEntries.insert(e);
        }

        return ok;
    }

    void removeEntity(Entity* e) {
        super::removeEntity(e);
        forced.erase(e);
    }

    void putEntity(int id, Entity* e) {
        Entity* old = getEntity(id);
        if (old != NULL) {
            removeEntity(old);
        }

        forced.insert(e);
        e->entityId = id;
        if (!addEntity(e)) {
            reEntries.insert(e);
        }
        entitiesById.insert(std::make_pair(id, e));
    }

    Entity* getEntity(int id) {
        EntityIdMap::const_iterator cit = entitiesById.find(id);
        return cit != entitiesById.end()? cit->second : NULL;
    }

    Entity* removeEntity(int id) {
        EntityIdMap::iterator it = entitiesById.find(id);
        if (it != entitiesById.end()) {
            Entity* e = it->second;
            entitiesById.erase(it);
            removeEntity(e);
            return it->second;
        }
        return NULL;
    }

    bool setDataNoUpdate(int x, int y, int z, int data) {
        int t = getTile(x, y, z);
        int d = getData(x, y, z);
        if (super::setDataNoUpdate(x, y, z, data)) {
            updatesToReset.push_back(/*new*/ ResetInfo(x, y, z, t, d));
            return true;
        }
        return false;
    }

    bool setTileAndDataNoUpdate(int x, int y, int z, int tile, int data) {
        int t = getTile(x, y, z);
        int d = getData(x, y, z);
        if (super::setTileAndDataNoUpdate(x, y, z, tile, data)) {
            updatesToReset.push_back(/*new*/ ResetInfo(x, y, z, t, d));
            return true;
        }
        return false;
    }

    bool setTileNoUpdate(int x, int y, int z, int tile) {
        int t = getTile(x, y, z);
        int d = getData(x, y, z);
        if (super::setTileNoUpdate(x, y, z, tile)) {
            updatesToReset.push_back(/*new*/ ResetInfo(x, y, z, t, d));
            return true;
        }
        return false;
    }

    bool doSetTileAndData(int x, int y, int z, int tile, int data) {
        clearResetRegion(x, y, z, x, y, z);
        if (super::setTileAndDataNoUpdate(x, y, z, tile, data)) {
            // if (setTile(x, y, z, tile)) {
            // setData(x, y, z, data);
            tileUpdated(x, y, z, tile);
            return true;
        }
        return false;
    }

    void disconnect() {
        //connection.sendAndDisconnect(/*new*/ DisconnectPacket("Quitting"));
    }

protected:
    void tickTiles() {
    }
    /*@Override*/
    ChunkSource* createChunkSource() {
        //chunkCache = /*new*/ MultiPlayerChunkCache(this);
        //return chunkCache;
		return NULL;
    }

    void entityAdded(Entity* e) {
        super::entityAdded(e);
		EntitySet::iterator it = reEntries.find(e);
		if (it != reEntries.end()) {
            reEntries.erase(it);
        }
    }

    void entityRemoved(Entity* e) {
        super::entityRemoved(e);
		EntitySet::iterator it = forced.find(e);
		if (it != forced.end()) {
			if (e->isAlive()) {
				reEntries.insert(e);
			} else {
				forced.erase(it);
			}
        }
    }

//     void tickWeather() {
//         if (dimension.hasCeiling) return;
//
//         if (lightningTime > 0) {
//             lightningTime--;
//         }
// 
//         oRainLevel = rainLevel;
//         if (levelData.isRaining()) {
//             rainLevel += 0.01;
//         } else {
//             rainLevel -= 0.01;
//         }
//         if (rainLevel < 0) rainLevel = 0;
//         if (rainLevel > 1) rainLevel = 1;
// 
//         oThunderLevel = thunderLevel;
//         if (levelData.isThundering()) {
//             thunderLevel += 0.01;
//         } else {
//             thunderLevel -= 0.01;
//         }
//         if (thunderLevel < 0) thunderLevel = 0;
//         if (thunderLevel > 1) thunderLevel = 1;
//     }
private:
    ResetInfoList updatesToReset;
    //ClientConnection connection;
    //MultiPlayerChunkCache chunkCache;

    EntityIdMap entitiesById;
    EntitySet forced;
    EntitySet reEntries;
};

#endif /*NET_MINECRAFT_CLIENT_MULTIPLAYER__MultiPlayerLevel_H__*/
