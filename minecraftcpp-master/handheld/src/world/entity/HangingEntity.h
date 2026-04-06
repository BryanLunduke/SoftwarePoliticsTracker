#ifndef NET_MINECRAFT_WORLD_ENTITY__HangingEntity_H__
#define NET_MINECRAFT_WORLD_ENTITY__HangingEntity_H__

#include "Entity.h"
class HangingEntity : public Entity {
	typedef Entity super;
public:
	HangingEntity(Level* level);
	HangingEntity(Level* level, int xTile, int yTile, int zTile, int dir);
	void init();
	void setDir(int dir);
	void setPosition(int x, int y, int z);
	virtual void tick();
	virtual bool survives();
	bool isPickable();
	bool interact(Player* player);
	void move(float xa, float ya, float za);
	void push(float xa, float ya, float za);
	virtual void addAdditonalSaveData(CompoundTag* tag);
	virtual void readAdditionalSaveData(CompoundTag* tag);
	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	virtual void dropItem() = 0;
	virtual bool isHangingEntity();
	virtual float getBrightness(float a);
	virtual bool hurt(Entity* source, int damage);
private:
	float offs(int w);
public:
	int dir;
	int xTile, yTile, zTile;
private:
	int checkInterval;
};

#endif /* NET_MINECRAFT_WORLD_ENTITY__HangingEntity_H__ */