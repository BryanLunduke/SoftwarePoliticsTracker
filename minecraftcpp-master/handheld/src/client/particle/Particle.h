#ifndef NET_MINECRAFT_CLIENT_PARTICLE__Particle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__Particle_H__

//package net.minecraft.client.particle;

#include "../renderer/Tesselator.h"
#include "../../world/entity/Entity.h"
#include "../../world/level/Level.h"
#include "ParticleEngine.h"

class CompoundTag;

class Particle: public Entity {
	typedef Entity super;

public:
	static float xOff, yOff, zOff;

    Particle(Level* level, float x, float y, float z, float xa, float ya, float za);

	virtual ~Particle() {}

    Particle* setPower(float power);
    Particle* scale(float scale);

    virtual void tick();
    virtual void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2);

    virtual int getParticleTexture();

    virtual void addAdditonalSaveData(CompoundTag* entityTag) {}
    virtual void readAdditionalSaveData(CompoundTag* tag) {}

	virtual int getEntityTypeId() const { return 0; }

protected:
    int tex;
    float uo, vo;
    int age;
    int lifetime;
    float size;
    float gravity;
    float rCol, gCol, bCol;
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__Particle_H__*/
