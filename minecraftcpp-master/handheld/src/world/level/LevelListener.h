#ifndef NET_MINECRAFT_WORLD_LEVEL__LevelListener_H__
#define NET_MINECRAFT_WORLD_LEVEL__LevelListener_H__

//package net.minecraft.world.level;

#include <string>
#include "../entity/EntityTypes.h"

class Entity;
class TripodCamera;
class Player;
class TileEntity;

class LevelListener
{
public:
    virtual ~LevelListener() {}

	virtual void setTilesDirty(int x0, int y0, int z0, int x1, int y1, int z1) {}
	virtual void tileChanged(int x, int y, int z) {}

	virtual void tileBrightnessChanged(int x, int y, int z) { tileChanged(x, y, z); }
	virtual void skyColorChanged() {}
	virtual void allChanged() {}

	virtual void takePicture(TripodCamera* cam, Entity* entity) {}

    virtual void addParticle(const std::string& name, float x, float y, float z, float xa, float ya, float za, int data) {}
	virtual void addParticle(ParticleType::Id name, float x, float y, float z, float xa, float ya, float za, int data) {}

	virtual void playSound(const std::string& name, float x, float y, float z, float volume, float pitch) {}
    virtual void playMusic(const std::string& name, float x, float y, float z, float songOffset) {}
	virtual void playStreamingMusic(const std::string& name, int x, int y, int z) {}

    virtual void entityAdded(Entity* entity) {}
    virtual void entityRemoved(Entity* entity) {}

	virtual void levelEvent(Player* source, int type, int x, int y, int z, int data) {}
	virtual void tileEvent(int x, int y, int z, int b0, int b1) {}
    virtual void tileEntityChanged(int x, int y, int z, TileEntity* te) {}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__LevelListener_H__*/
