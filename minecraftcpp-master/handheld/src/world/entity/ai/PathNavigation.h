#ifndef NET_MINECRAFT_WORLD_ENTITY_AI__PathNavigation_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI__PathNavigation_H__

//package net.minecraft.world.entity.ai;

#include "../Mob.h"
#include "../../level/Level.h"
#include "../../level/material/Material.h"
#include "../../level/tile/Tile.h"
#include "../../phys/Vec3.h"

#include <cmath>
#include "../../level/pathfinder/Path.h"
#include "control/MoveControl.h"
#include "../../../util/MemUtils.h"

class PathNavigation
{
public:
    PathNavigation(Mob* mob, Level* level, float maxDist)
    :   mob(mob),
        level(level),
        maxDist(maxDist),
		_tick(0),
        avoidSun(false),
        avoidWater(false),
        canOpenDoors(false),
        lastStuckCheckPos(0, 0, 0),
		speed(0),
		lastStuckCheck(0),
		path(NULL),
		deletePath(false)
    {
    }
	~PathNavigation() {
		deletePathIfNeeded();
	}

    void setSpeed(float speed) {
        this->speed = speed;
    }

    Path* createPath(float x, float y, float z) {
        //LOGI("can update? %d\n", canUpdatePath());
		if (!canUpdatePath())
			return NULL;
		Path* p = new Path();
        level->findPath(p, mob, Mth::floor(x), (int) y, Mth::floor(z), maxDist, canOpenDoors, avoidWater);
		return p;
    }

    bool moveTo(float x, float y, float z, float speed) {
        Path* newPath = createPath(x, y, z);
        return moveTo(newPath, speed, true);
    }

    Path* createPath(Mob* target) {
		//LOGI("can update? %d\n", canUpdatePath());
        if (!canUpdatePath())
			return NULL;
		Path* p = new Path();
        level->findPath(p, mob, target, maxDist, canOpenDoors, avoidWater);
		return p;
    }

    bool moveTo(Mob* target, float speed) {
        Path* newPath = createPath(target);
        //if (!newPath->isEmpty()) return moveTo(newPath, speed);
		if (newPath) return moveTo(newPath, speed, true);
        else return false;
    }

    bool moveTo(Path* newPath, float speed, bool navIsPathOwner) {
        if (newPath == NULL) {
			deletePathIfNeeded();
            return false;
        }
		//if (newPath.isEmpty()) {
		//	path.destroy();
		//	return false;
		//}

		Node* last = newPath->last();
		if (last) {
			//LOGI("> %d, %d, %d in %d steps\n", last->x, last->y, last->z, newPath->getSize());
		}

        /*if (!newPath->sameAs(path))*/ {
			// Delete old path if we are owner
			deletePathIfNeeded();
			path = newPath;
			deletePath = navIsPathOwner;
		}
        if (avoidSun) trimPathFromSun();
        if (path->getSize() == 0) return false;

        this->speed = speed;
        Vec3 mobPos = getTempMobPos();
        lastStuckCheck = _tick;
        lastStuckCheckPos.x = mobPos.x;
        lastStuckCheckPos.y = mobPos.y;
        lastStuckCheckPos.z = mobPos.z;
        return true;
    }

    //Ref<Path>* getPath() {
	Path* getPath() {
        return path;
    }

    void tick() {
        ++_tick;
		//LOGI("hehe %d\n", _tick);
        if (isDone()) return;

		//LOGI("hehe2 %d %d\n", _tick, canUpdatePath());
        if (canUpdatePath()) updatePath();

		//LOGI("hehe3 %d\n", _tick);
        if (isDone()) {
			//LOGI("done!\n");
			return;
		}
        Vec3 target = path->currentPos(mob);
		//LOGI("hehe4 %d\n", _tick);

        mob->getMoveControl()->setWantedPosition(target.x, target.y, target.z, speed);
    }

    bool isDone() {
        return !path || path->isDone();// path == NULL || path.isDone();
    }

    void stop() {
		deletePathIfNeeded();
		//if (path) {
		//	path->destroy(); //@?
		//	path = NULL;
		//}
    }

private:
	void deletePathIfNeeded() {
		//return;
		if (deletePath && path) {
			LOGI("nav-deleting %p (%d)\n", path, path->id);
			delete path;
			//deletePath = false;
		}
		path = NULL;
	}

    void updatePath() {
        Vec3 mobPos = getTempMobPos();

		Path& path = *this->path;

        // find first elevations in path
        int firstElevation = path.getSize();
        for (int i = path.getIndex(); i < path.getSize(); ++i)
            if (path.get(i)->y != (int) mobPos.y) {
                firstElevation = i;
                break;
            }

        // remove those within way point radius (this is not optimal, should
        // check canWalkDirectly also) possibly only check next as well
        float waypointRadiusSqr = mob->bbWidth * mob->bbWidth;
        for (int i = path.getIndex(); i < firstElevation; ++i) {
            if (mobPos.distanceToSqr(path.getPos(mob, i)) < waypointRadiusSqr) {
                path.setIndex(i + 1);
            }
        }

        // smooth remaining on same elevation
        int sx = (int)ceil(mob->bbWidth);
        int sy = (int) mob->bbHeight + 1;
        int sz = sx;
        for (int i = firstElevation - 1; i >= path.getIndex(); --i) {
            if (canMoveDirectly(mobPos, path.getPos(mob, i), sx, sy, sz)) {
                path.setIndex(i);
                break;
            }
        }

        // stuck detection (probably pushed off path)
        if (_tick - lastStuckCheck > 100) {
            if (mobPos.distanceToSqr(lastStuckCheckPos) < 1.5 * 1.5) stop();
            lastStuckCheck = _tick;
            lastStuckCheckPos.x = mobPos.x;
            lastStuckCheckPos.y = mobPos.y;
            lastStuckCheckPos.z = mobPos.z;
        }
    }

    Vec3 getTempMobPos() {
        return Vec3(mob->x, (float)getSurfaceY(), mob->z);
    }

    int getSurfaceY() {
        if (!mob->isInWater()) return (int) (mob->bb.y0 + 0.5);

        int surface = (int) (mob->bb.y0);
        int tileId = level->getTile(Mth::floor(mob->x), surface, Mth::floor(mob->z));
        int steps = 0;
        while (tileId == Tile::water->id || tileId == Tile::calmWater->id) {
            ++surface;
            tileId = level->getTile(Mth::floor(mob->x), surface, Mth::floor(mob->z));
            if (++steps > 16) return (int) (mob->bb.y0);
        }
        return surface;
    }

    bool canUpdatePath() {
        return mob->onGround || isInLiquid();
    }

    bool isInLiquid() {
        return mob->isInWater() || mob->isInLava();
    }

    void trimPathFromSun() {
        if (level->canSeeSky(Mth::floor(mob->x), (int) (mob->bb.y0 + 0.5), Mth::floor(mob->z)))
			return;

		Path& path = *this->path;

        for (int i = 0; i < path.getSize(); ++i) {
            Node* n = path.get(i);
            if (level->canSeeSky(n->x, n->y, n->z)) {
                path.setSize(i - 1);
                return;
            }
        }
    }

    bool canMoveDirectly(const Vec3& startPos, const Vec3& stopPos, int sx, int sy, int sz) {

        int gridPosX = Mth::floor(startPos.x);
        int gridPosZ = Mth::floor(startPos.z);

        float dirX = stopPos.x - startPos.x;
        float dirZ = stopPos.z - startPos.z;
        float distSqr = dirX * dirX + dirZ * dirZ;
        if (distSqr < 0.00001f) return false;

        float nf = Mth::invSqrt(distSqr);
        dirX *= nf;
        dirZ *= nf;

        sx += 2;
        sz += 2;
        if (!canWalkOn(gridPosX, (int) startPos.y, gridPosZ, sx, sy, sz, startPos, dirX, dirZ)) return false;
        sx -= 2;
        sz -= 2;

        float deltaX = 1 / Mth::abs(dirX);
        float deltaZ = 1 / Mth::abs(dirZ);

        float maxX = gridPosX * 1 - startPos.x;
        float maxZ = gridPosZ * 1 - startPos.z;
        if (dirX >= 0) maxX += 1;
        if (dirZ >= 0) maxZ += 1;
        maxX /= dirX;
        maxZ /= dirZ;

        int stepX = dirX < 0 ? -1 : 1;
        int stepZ = dirZ < 0 ? -1 : 1;
        int gridGoalX = Mth::floor(stopPos.x);
        int gridGoalZ = Mth::floor(stopPos.z);
        int currentDirX = gridGoalX - gridPosX;
        int currentDirZ = gridGoalZ - gridPosZ;
        while (currentDirX * stepX > 0 || currentDirZ * stepZ > 0) {
            if (maxX < maxZ) {
                maxX += deltaX;
                gridPosX += stepX;
                currentDirX = gridGoalX - gridPosX;
            } else {
                maxZ += deltaZ;
                gridPosZ += stepZ;
                currentDirZ = gridGoalZ - gridPosZ;
            }

            if (!canWalkOn(gridPosX, (int) startPos.y, gridPosZ, sx, sy, sz, startPos, dirX, dirZ)) return false;
        }
        return true;
    }

    bool canWalkOn(int x, int y, int z, int sx, int sy, int sz, const Vec3& startPos, float goalDirX, float goalDirZ) {
        int startX = x - sx / 2;
        int startZ = z - sz / 2;

        if (!canWalkAbove(startX, y, startZ, sx, sy, sz, startPos, goalDirX, goalDirZ)) return false;

        // lava or water or air under
        for (int xx = startX; xx < startX + sx; xx++) {
            for (int zz = startZ; zz < startZ + sz; zz++) {
                float dirX = xx + 0.5f - startPos.x;
                float dirZ = zz + 0.5f - startPos.z;
                if (dirX * goalDirX + dirZ * goalDirZ < 0) continue;
                int tile = level->getTile(xx, y - 1, zz);
                if (tile <= 0) return false;
                const Material* m = Tile::tiles[tile]->material;
                if (m == Material::water && !mob->isInWater()) return false;
                if (m == Material::lava) return false;
            }
        }

        return true;
    }

    bool canWalkAbove(int startX, int startY, int startZ, int sx, int sy, int sz, const Vec3& startPos, float goalDirX, float goalDirZ) {
        for (int xx = startX; xx < startX + sx; xx++) {
            for (int yy = startY; yy < startY + sy; yy++) {
                for (int zz = startZ; zz < startZ + sz; zz++) {

                    float dirX = xx + 0.5f - startPos.x;
                    float dirZ = zz + 0.5f - startPos.z;
                    if (dirX * goalDirX + dirZ * goalDirZ < 0) continue;
                    int tile = level->getTile(xx, yy, zz);
                    if (tile <= 0) continue;
					if (Tile::tiles[tile]->material->blocksMotion()) return false;
                    //if (!Tile::tiles[tile]->isPathfindable(level, xx, yy, zz)) return false; //@todo
                }
            }
        }
        return true;
    }
private:
    Mob* mob;
    Level* level;
	Path* path;
	bool deletePath;
    Ref<Path>* refPath;
    float speed;
    float maxDist;
    int _tick;
    int lastStuckCheck;
    Vec3 lastStuckCheckPos;
public:
    bool avoidWater;
    bool avoidSun;
    bool canOpenDoors;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI__PathNavigation_H__*/
