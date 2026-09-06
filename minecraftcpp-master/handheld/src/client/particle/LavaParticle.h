#ifndef NET_MINECRAFT_CLIENT_PARTICLE__LavaParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__LavaParticle_H__

//package net.minecraft.client.particle;

#include "../renderer/Tesselator.h"
#include "../../world/level/Level.h"

class LavaParticle: public Particle
{
	typedef Particle super;
    float oSize;
public:
    LavaParticle(Level* level, float x, float y, float z)
    :	super(level, x, y, z, 0, 0, 0)
	{
        xd *= 0.8f;
        yd *= 0.8f;
        zd *= 0.8f;
        yd = sharedRandom.nextFloat() * 0.4f + 0.05f;

        rCol = gCol = bCol = 1;
        size *= (sharedRandom.nextFloat() * 2 + 0.2f);
        oSize = size;

        lifetime = (int) (16 / (Mth::random() * 0.8 + 0.2));
        noPhysics = false;
        tex = 49;
    }

    float getBrightness(float a) {
        return 1;
    }

    void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
        float s = (age + a) / (float) lifetime;
        size = oSize * (1 - s*s);
        super::render(t, a, xa, ya, za, xa2, za2);
    }

    void tick() {
        xo = x;
        yo = y;
        zo = z;

        if (age++ >= lifetime) remove();
        float odds = age / (float) lifetime;
        if (sharedRandom.nextFloat() > odds) level->addParticle(PARTICLETYPE(smoke), x, y, z, xd, yd, zd);

        yd -= 0.03f;
        move(xd, yd, zd);
        xd *= 0.999f;
        yd *= 0.999f;
        zd *= 0.999f;

        if (onGround) {
            xd *= 0.7f;
            zd *= 0.7f;
        }
    }
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__LavaParticle_H__*/
