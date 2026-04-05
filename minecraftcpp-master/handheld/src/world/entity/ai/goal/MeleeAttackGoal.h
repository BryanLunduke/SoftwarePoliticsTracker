#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__MeleeAttackGoal_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__MeleeAttackGoal_H__

//package net.minecraft.world.entity.ai.goal;

#include "Goal.h"

#include "../control/Control.h"
#include "../../monster/Monster.h"
#include "../../../level/Level.h"
#include "../../../level/pathfinder/Path.h"
#include "../Sensing.h"

class MeleeAttackGoal: public Goal
{
public:
    MeleeAttackGoal(Monster* mob, float speed, bool trackTarget, int attackType = 0)
	:	mob(mob),
		level(mob->level),
		speed(speed),
		trackTarget(trackTarget),
		attackTime(0),
		attackType(attackType),
		path(NULL)
	{
		setRequiredControlFlags(Control::MoveControlFlag | Control::LookControlFlag);
    }
	~MeleeAttackGoal() {
		if (path) {
			LOGI("mag-deleting %p (%d)\n", path, path->id);
			delete path;
		}
	}

    /*@Override*/
    bool canUse() {
        Mob* bestTarget = mob->getTarget();
        if (bestTarget == NULL) return false;
        if (attackType != 0 && !mob->isPlayer()) return false; //!attackType.isAssignableFrom(bestTarget.getClass())) return false;
        target = bestTarget;
		if (path) {
			LOGI("mag-canuse-deleting %p (%d)\n", path, path->id);
			delete path;
		}
        path = mob->getNavigation()->createPath(target);
        return path != NULL;
    }

    bool canContinueToUse() {
        Mob* bestTarget = mob->getTarget();
        if (bestTarget == NULL) return false;
        if (attackType != 0 && !mob->isPlayer()) return false;//!attackType.isAssignableFrom(bestTarget.getClass())) return false;
        target = bestTarget;
        if (!trackTarget) return !mob->getNavigation()->isDone();
        return true;
    }

    void start() {
        mob->getNavigation()->moveTo(path, speed, false);
    }

    void stop() {
        target = NULL;
        mob->getNavigation()->stop();
    }

    void tick() {
        //mob->getLookControl().setLookAt(target, 30, 30);
        if (trackTarget || mob->sensing->canSee(target)) {
			//LOGI("target: %p @ %f, %f, %f\n", target, target->x, target->y, target->z);
			mob->getNavigation()->moveTo(target, speed);
		}

        attackTime = Mth::Max(attackTime - 1, 0);

        float meleeRadiusSqr = (mob->bbWidth * 2) * (mob->bbWidth * 2);
        if (mob->distanceToSqr(target->x, target->bb.y0, target->z) > meleeRadiusSqr) return;
        if (attackTime > 0) return;
        attackTime = 20;
        mob->doHurtTarget(target);
    }

private:
    Level* level;
    Monster* mob;
    Mob* target;
    int attackTime;
    float speed;
    Path* path;
    int attackType;
	bool trackTarget;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__MeleeAttackGoal_H__*/
