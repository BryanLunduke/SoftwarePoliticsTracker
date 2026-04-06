#ifndef NET_MINECRAFT_CLIENT_PARTICLE__BubbleParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__BubbleParticle_H__

//package net.minecraft.client.particle;

#include "../../util/Mth.h"
#include "../../world/level/Level.h"
#include "../../world/level/material/Material.h"
#include "Particle.h"

class BubbleParticle: public Particle
{
	typedef Particle super;

public:
    BubbleParticle(Level* level, float x, float y, float z, float xa, float ya, float za)
	:	super(level, x, y, z, xa, ya, za)
	{
        rCol = 1.0f;
        gCol = 1.0f;
        bCol = 1.0f;
        tex = 32;
        this->setSize(0.02f, 0.02f);
        
        size = size*(sharedRandom.nextFloat()*0.6f+0.2f);
        
        xd = xa*0.2f+(float)(Mth::random()*2-1)*0.02f;
        yd = ya*0.2f+(float)(Mth::random()*2-1)*0.02f;
        zd = za*0.2f+(float)(Mth::random()*2-1)*0.02f;        

        lifetime = (int) (8 / (Mth::random() * 0.8 + 0.2));
    }

    void tick() {
        xo = x;
        yo = y;
        zo = z;

        yd += 0.002f;
        move(xd, yd, zd);
        xd *= 0.85f;
        yd *= 0.85f;
        zd *= 0.85f;

        if (level->getMaterial(Mth::floor(x), Mth::floor(y), Mth::floor(z)) != Material::water) remove();

        if (lifetime-- <= 0) remove();
    }
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__BubbleParticle_H__*/
