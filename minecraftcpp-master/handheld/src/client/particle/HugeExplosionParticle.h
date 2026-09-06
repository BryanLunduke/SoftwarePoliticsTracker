#ifndef NET_MINECRAFT_CLIENT_PARTICLE__HugeExplosionParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__HugeExplosionParticle_H__

//package net.minecraft.client.particle;

#include "../renderer/Tesselator.h"
#include "../renderer/Textures.h"
#include "../../world/level/Level.h"

class HugeExplosionParticle: public Particle
{
	typedef Particle super;
public:
    HugeExplosionParticle(Textures* textures, Level* level, float x, float y, float z, float xa, float ya, float za)
	:	super(level, x, y, z, 0, 0, 0),
		life(0),
		lifeTime(6 + sharedRandom.nextInt(4)),
		textures(textures),
		size(1 - xa * 0.5f)
	{
        rCol = gCol = bCol = sharedRandom.nextFloat() * 0.6f + 0.4f;
    }

    void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
        int tex = (int) ((life + a) * 15 / lifeTime);
        if (tex > 15) return;
		textures->loadAndBindTexture("misc/explosion.png");

        float u0 = (tex % 4) / 4.0f;
        float u1 = u0 + 0.999f / 4.0f;
        float v0 = (tex / 4) / 4.0f;
        float v1 = v0 + 0.999f / 4.0f;

//        xa2 = 0;
//        za2 = 1;

        float r = size + size;

        float x = xo + (this->x - xo) * a - xOff;
        float y = yo + (this->y - yo) * a - yOff;
        float z = zo + (this->z - zo) * a - zOff;
		const float xar = xa * r, yar = ya * r, zar = za * r;
		const float xa2r = xa2 * r, za2r = za2 * r;
        t.begin();
        t.color(rCol, gCol, bCol, 1.0f);
        t.vertexUV(x - xar - xa2r, y - yar, z - zar - za2r, u1, v1);
        t.vertexUV(x - xar + xa2r, y + yar, z - zar + za2r, u1, v0);
        t.vertexUV(x + xar + xa2r, y + yar, z + zar + za2r, u0, v0);
        t.vertexUV(x + xar - xa2r, y - yar, z + zar - za2r, u0, v1);
        t.draw();
        //glPolygonOffset(0, 0.0f);
    }

    void tick() {
        xo = x;
        yo = y;
        zo = z;
        life++;
        if (life == lifeTime) remove();
    }

    int getParticleTexture() {
        return ParticleEngine::ENTITY_PARTICLE_TEXTURE;
    }
private:
	int life;
	int lifeTime;
	float size;
	Textures* textures;
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__HugeExplosionParticle_H__*/
