#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_UTIL__RandomPos_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_UTIL__RandomPos_H__

//package net.minecraft.world.entity.ai.util;

#include "../../PathfinderMob.h"
#include "../../../phys/Vec3.h"

class RandomPos
{
public:
    static bool getPos(Vec3& outPos, PathfinderMob* mob, int xzDist, int yDist) {
        return generateRandomPos(outPos, mob, xzDist, yDist, NULL);
    }

    static bool getPosTowards(Vec3& outPos, PathfinderMob* mob, int xzDist, int yDist, Vec3* towardsPos) {
		Vec3 tempDir(towardsPos->x, towardsPos->y, towardsPos->z);
		tempDir.subSelf(mob->x, mob->y, mob->z);
        return generateRandomPos(outPos, mob, xzDist, yDist, &tempDir);
    }

    static bool getPosAvoid(Vec3& outPos, PathfinderMob* mob, int xzDist, int yDist, Vec3* avoidPos) {
		Vec3 tempDir(mob->x, mob->y, mob->z);
		tempDir.subSelf(avoidPos->x, avoidPos->y, avoidPos->z);
        return generateRandomPos(outPos, mob, xzDist, yDist, &tempDir);
    }

private:
    static bool generateRandomPos(Vec3& outPos, PathfinderMob* mob, int xzDist, int yDist, Vec3* dir) {
        Random& random = mob->random;
        bool hasBest = false;
        int xBest = 0, yBest = 0, zBest = 0;
        float best = -99999;

//         bool restrict;
//         if (mob->hasRestriction()) {
//             float restDist = mob->getRestrictCenter().dist((int) Math.floor(mob->x), (int) Math.floor(mob->y), (int) Math.floor(mob->z)) + 4;
//             restrict = restDist < mob->getRestrictRadius() + xzDist;
//         } else restrict = false;

        for (int i = 0; i < 10; i++) {
            int xt = random.nextInt(2 * xzDist) - xzDist;
            int yt = random.nextInt(2 * yDist) - yDist;
            int zt = random.nextInt(2 * xzDist) - xzDist;

            if (dir != NULL && xt * dir->x + zt * dir->z < 0) continue;

            xt += Mth::floor(mob->x);
            yt += Mth::floor(mob->y);
            zt += Mth::floor(mob->z);

            //if (restrict && !mob->isWithinRestriction(xt, yt, zt)) continue;
            float value = mob->getWalkTargetValue(xt, yt, zt);
            if (value > best) {
                best = value;
                xBest = xt;
                yBest = yt;
                zBest = zt;
                hasBest = true;
            }
        }
        if (hasBest) {
            outPos.set((float)xBest, (float)yBest, (float)zBest);
			return true;
        }

        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_UTIL__RandomPos_H__*/
