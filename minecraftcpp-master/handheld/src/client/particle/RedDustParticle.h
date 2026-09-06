#ifndef NET_MINECRAFT_CLIENT_PARTICLE__RedDustParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__RedDustParticle_H__

//package net.minecraft.client.particle;

#include "../renderer/Tesselator.h"
#include "../../world/level/Level.h"


class RedDustParticle: public Particle
{
	typedef Particle super;
public:
    float oSize;

    RedDustParticle(Level* level, float x, float y, float z, float rCol, float gCol, float bCol, float scale = 1.0f)
    :	super(level, x, y, z, 0, 0, 0)
	{
        xd *= 0.1f;
        yd *= 0.1f;
        zd *= 0.1f;

        if (rCol == 0) {
            rCol = 1;
        }
        float brr = (float) Mth::random() * 0.4f + 0.6f;
        this->rCol = ((float) (Mth::random() * 0.2f) + 0.8f) * rCol * brr;
        this->gCol = ((float) (Mth::random() * 0.2f) + 0.8f) * gCol * brr;
        this->bCol = ((float) (Mth::random() * 0.2f) + 0.8f) * bCol * brr;
        size *= 0.75f;
        size *= scale;
        oSize = size;

        lifetime = (int) (8 / (Mth::random() * 0.8 + 0.2));
        lifetime = (int)(lifetime * scale);
        noPhysics = false;
    }

    void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
        float l = ((age + a) / lifetime) * 32;
        if (l < 0) l = 0;
        if (l > 1) l = 1;

        size = oSize * l;
        super::render(t, a, xa, ya, za, xa2, za2);
    }

    void tick() {
        xo = x;
        yo = y;
        zo = z;

        if (age++ >= lifetime) remove();

        tex = 7 - age * 8 / lifetime;

        move(xd, yd, zd);
        if (y == yo) {
            xd *= 1.1f;
            zd *= 1.1f;
        }
        xd *= 0.96f;
        yd *= 0.96f;
        zd *= 0.96f;

        if (onGround) {
            xd *= 0.7f;
            zd *= 0.7f;
        }
    }
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__RedDustParticle_H__*/
