#ifndef NET_MINECRAFT_CLIENT_PARTICLE__FlameParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__FlameParticle_H__

//package net.minecraft.client.particle;

#include "Particle.h"
#include "../renderer/Tesselator.h"
#include "../../world/level/Level.h"

class FlameParticle: public Particle
{
	typedef Particle super;
    float oSize;
public:
    FlameParticle(Level* level, float x, float y, float z, float xa, float ya, float za)
	:	super(level, x, y, z, xa, ya, za)
    {
        xd = xd * 0.01f + xa;
        yd = yd * 0.01f + ya;
        zd = zd * 0.01f + za;
        x += (sharedRandom.nextFloat()-sharedRandom.nextFloat()) * 0.05f;
        y += (sharedRandom.nextFloat()-sharedRandom.nextFloat()) * 0.05f;
        z += (sharedRandom.nextFloat()-sharedRandom.nextFloat()) * 0.05f;
        
		//printf("xd, yd, zd: %f, %f, %f\n", xd, yd, zd);

        oSize = size;
        rCol = gCol = bCol = 1.0f;
        
        lifetime = (int)(8.0f/(Mth::random()*0.8f+0.2f))+4;
        noPhysics = true;
        tex = 48;
    }
    
    void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
        float s = (age + a) / (float) lifetime;
        size = oSize * (1 - s*s*0.5f);
        super::render(t, a, xa, ya, za, xa2, za2);
    }    

    float getBrightness(float a) {
		return 1.0f;
/*
        float l = (age+a)/lifetime;
        if (l<0) l = 0;
        if (l>1) l = 1;
        float br = super::getBrightness(a);
        
        return br*l+(1-l);
*/
    }

    void tick()
    {
        xo = x;
        yo = y;
        zo = z;

        if (age++ >= lifetime) remove();

        move(xd, yd, zd);
        xd *= 0.96f;
        yd *= 0.96f;
        zd *= 0.96f;

        if (onGround)
        {
            xd *= 0.7f;
            zd *= 0.7f;
        }
    }
};
#endif /*NET_MINECRAFT_CLIENT_PARTICLE__FlameParticle_H__*/
