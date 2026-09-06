#ifndef NET_MINECRAFT_CLIENT_PARTICLE__HugeExplosionSeedParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__HugeExplosionSeedParticle_H__

//package net.minecraft.client.particle;

#include "Particle.h"

class HugeExplosionSeedParticle: public Particle
{
	typedef Particle super;
public:
	HugeExplosionSeedParticle(Level* level, float x, float y, float z, float xa, float ya, float za)
	:	super(level, x, y, z, 0, 0, 0),
		life(0),
		lifeTime(8)
	{
	}

	void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {}

    void tick() {
		for (int i = 0; i < 6; i++) {
			float xx = x + (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 4;
			float yy = y + (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 4;
			float zz = z + (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 4;
			level->addParticle(PARTICLETYPE(largeexplode), xx, yy, zz, life / (float) lifeTime, 0, 0);
		}
		life++;
		if (life == lifeTime) remove();
	}

	int getParticleTexture() {
		return ParticleEngine::TERRAIN_TEXTURE;
	}
private:
	int life;
	int lifeTime;
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__HugeExplosionSeedParticle_H__*/
