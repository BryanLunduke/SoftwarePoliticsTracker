#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__Goal_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__Goal_H__

//package net.minecraft.world.entity.ai.goal;

class Goal
{
public:
    Goal()
    :   _requiredControlFlags(0)
    {}
    virtual ~Goal() {}

    virtual bool canUse() = 0;

    virtual bool canContinueToUse() {
        return canUse();
    }

    virtual bool canInterrupt() {
        return true;
    }

    virtual void start() {
    }

    virtual void stop() {
    }

    virtual void tick() {
    }

    void setRequiredControlFlags(int requiredControlFlags) {
        _requiredControlFlags = requiredControlFlags;
    }

    int getRequiredControlFlags() {
        return _requiredControlFlags;
    }
private:
    int _requiredControlFlags;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_GOAL__Goal_H__*/
