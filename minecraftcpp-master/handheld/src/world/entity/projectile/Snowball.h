#ifndef NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Snowball_H__
#define NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Snowball_H__

//package net.minecraft.world.entity->projectile;

#include "Throwable.h"
#include "../Mob.h"
#include "../../level/Level.h"
#include "../../phys/HitResult.h"

class Snowball: public Throwable
{
	typedef Throwable super;
public:
    Snowball(Level* level)
	:	super(level)
	{
		entityRendererId = ER_SNOWBALL_RENDERER;
    }

    Snowball(Level* level, Mob* mob)
    :	super(level, mob)
	{
		entityRendererId = ER_SNOWBALL_RENDERER;
	}

    Snowball(Level* level, float x, float y, float z)
    :	super(level, x, y, z)
	{
		entityRendererId = ER_SNOWBALL_RENDERER;
    }

	virtual int getEntityTypeId() const {
		return EntityTypes::IdSnowball;
	}

    /*@Override*/
	void onHit(const HitResult& res) {
        if (res.type == ENTITY)
            res.entity->hurt(this, 0);

		for (int i = 0; i < 6; i++)
            level->addParticle(PARTICLETYPE(snowballpoof), x, y, z, 0, 0, 0);

		if (!level->isClientSide)
            remove();
    }
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Snowball_H__*/
