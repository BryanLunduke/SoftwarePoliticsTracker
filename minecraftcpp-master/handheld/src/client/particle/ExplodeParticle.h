#ifndef NET_MINECRAFT_CLIENT_PARTICLE__ExplodeParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__ExplodeParticle_H__

//package net.minecraft.client.particle;

#include "Particle.h"
#include "../renderer/Tesselator.h"
#include "../../world/level/Level.h"

class ExplodeParticle: public Particle
{
	typedef Particle super;
public:
    ExplodeParticle(Level* level, float x, float y, float z, float xa, float ya, float za)
    :   super(level, x, y, z, xa, ya, za)
    {
        xd = xa + (Mth::random()*2.0f-1.0f) * 0.05f;
        yd = ya + (Mth::random()*2.0f-1.0f) * 0.05f;
        zd = za + (Mth::random()*2.0f-1.0f) * 0.05f;

        rCol = gCol = bCol = sharedRandom.nextFloat() * 0.3f + 0.7f;
        size = sharedRandom.nextFloat() * sharedRandom.nextFloat() * 6.0f + 1.0f;

        lifetime = (int)(16.0f/(sharedRandom.nextFloat() * 0.8f + 0.2f)) + 2;
//        noPhysics = true;
    }

    void tick()
    {
        xo = x;
        yo = y;
        zo = z;

        if (age++ >= lifetime) remove();
        
        tex = 7 - age * 8 / lifetime;

        yd += 0.004f;
        move(xd, yd, zd);
        xd *= 0.90f;
        yd *= 0.90f;
        zd *= 0.90f;

        if (onGround) {
            xd *= 0.7f;
            zd *= 0.7f;
        }
    }
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__ExplodeParticle_H__*/
