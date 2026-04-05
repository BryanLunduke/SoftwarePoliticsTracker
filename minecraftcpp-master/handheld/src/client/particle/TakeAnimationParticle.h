#ifndef NET_MINECRAFT_CLIENT_PARTICLE__TakeAnimationParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__TakeAnimationParticle_H__

//package net.minecraft.client.particle;

#include "Particle.h"

#include "../renderer/entity/EntityRenderDispatcher.h"
#include "../renderer/Tesselator.h"
#include "../../world/entity/Entity.h"
#include "../../world/level/Level.h"
#include "../../util/Mth.h"
#include "../../world/entity/item/ItemEntity.h"

class TakeAnimationParticle: public Particle
{
    typedef Particle super;

public:
	//@todo:itementity
    TakeAnimationParticle(Level* level, ItemEntity* item, Entity* target, float yOffs)
    :   super(level, item->x, item->y, item->z, item->xd, item->yd, item->zd),
		e(level, item->x, item->y, item->z, item->item),
		//bx(item->x), by(item->y), bz(item->z),
        target(target),
        yOffs(yOffs),
        life(0),
        lifeTime(3)
    {
		e.item.count = 1;
	}

    void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
        float time = (life + a) / lifeTime;
        time = time*time;

        float xo = e.x;
        float yo = e.y;
        float zo = e.z;

        float xt = target->xOld + (target->x - target->xOld) * a;
        float yt = target->yOld + (target->y - target->yOld) * a+yOffs;
        float zt = target->zOld + (target->z - target->zOld) * a;

        float xx = xo + (xt - xo) * time;
        float yy = yo + (yt - yo) * time;
        float zz = zo + (zt - zo) * time;

        int xTile = Mth::floor(xx);
        int yTile = Mth::floor(yy + heightOffset * 0.5f);
        int zTile = Mth::floor(zz);

        float br = level->getBrightness(xTile, yTile, zTile);
        glColor4f2(br, br, br, 1);

        xx -= xOff;
        yy -= yOff;
        zz -= zOff;

        EntityRenderDispatcher::getInstance()->render(&e, xx, yy, zz, e.yRot, a);
    }

    void tick() {
        life++;
        if (life == lifeTime) remove();
    }

    int getParticleTexture() {
        return ParticleEngine::ENTITY_PARTICLE_TEXTURE;
    }

private:
    //Entity* item;
	ItemEntity e;
    Entity* target;
	//float bx, by, bz;
    int life;
    int lifeTime;
    float yOffs;
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__TakeAnimationParticle_H__*/
