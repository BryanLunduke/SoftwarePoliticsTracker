#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__GoalSelector_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__GoalSelector_H__

//package net.minecraft.world.entity.ai.goal;

#include "Goal.h"

#include <vector>

class GoalSelector
{
    class InternalGoal {
    public:
        InternalGoal(short prio, Goal* goal)
        :   prio(prio),
            goal(goal),
			isUsing(false)
        {}

        Goal* goal;
        short prio;
		bool isUsing;
    };
public:
	~GoalSelector() {
		for (unsigned int i = 0; i < goals.size(); ++i) {
			delete goals[i].goal;
		}
	}

    void addGoal(int prio, Goal* goal) {
        goals.push_back(InternalGoal(prio, goal));
    }

    void tick() {
        std::vector<InternalGoal> toStart;

		for (std::vector<InternalGoal>::iterator it = goals.begin(); it != goals.end(); ++it) {
			InternalGoal& ig = *it;
            bool isUsing = ig.isUsing; //usingGoals.contains(ig);

            if (isUsing) {
                if (!canUseInSystem(ig) || !ig.goal->canContinueToUse()) {
                    ig.goal->stop();
					ig.isUsing = false;
                    //usingGoals.remove(ig);
                } else continue;
            }

            if (!canUseInSystem(ig) || !ig.goal->canUse()) {
                continue;
            }

            toStart.push_back(ig);
            //usingGoals.add(ig);
			ig.isUsing = true;
        }

        //bool debug = false;
        //if (debug && toStart.size() > 0) printf("Starting: ");
		for (std::vector<InternalGoal>::iterator it = toStart.begin(); it != toStart.end(); ++it) {
			InternalGoal& ig = *it;
            //if (debug) printf("  %s, ", ig.goal.toString() + ", ");
            ig.goal->start();
        }

        //if (debug && usingGoals.size() > 0) printf("Running: ");
		for (std::vector<InternalGoal>::iterator it = goals.begin(); it != goals.end(); ++it) {
			InternalGoal& ig = *it;
			if (ig.isUsing) {
				//if (debug) printf("  %s\n", ig.goal.toString());
				ig.goal->tick();
			}
        }
    }

    //std::vector<InternalGoal>& getRunningGoals() {
    //    return usingGoals;
    //}
private:
    bool canUseInSystem(InternalGoal& goal) {
		for (std::vector<InternalGoal>::iterator it = goals.begin(); it != goals.end(); ++it) {
			InternalGoal& ig = *it;
            if (ig.goal == goal.goal && ig.prio == goal.prio)
				continue;
            if (goal.prio >= ig.prio) {
                if (ig.isUsing && !canCoExist(goal, ig))
					return false;
            } else if (ig.isUsing && !ig.goal->canInterrupt())
				return false;
        }

        return true;
    }

    bool canCoExist(InternalGoal& goalA, InternalGoal& goalB) {
        return (goalA.goal->getRequiredControlFlags() & goalB.goal->getRequiredControlFlags()) == 0;
    }

    std::vector<InternalGoal> goals;
    //std::vector<InternalGoal> usingGoals;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__GoalSelector_H__*/
