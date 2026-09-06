#ifndef NET_MINECRAFT_CLIENT_MODEL__Model_H__
#define NET_MINECRAFT_CLIENT_MODEL__Model_H__

//package net.minecraft.client.model;

#include <vector>
#include "geom/ModelPart.h"

class Mob;
class Entity;

class Model
{
protected:
	Model()
	:	riding(false),
		attackTime(0),
		texWidth(64),
		texHeight(32),
		young(true)
	{}

public:
	virtual ~Model() {}

	virtual void onGraphicsReset() {
		for (unsigned int i = 0; i < cubes.size(); ++i)
			cubes[i]->onGraphicsReset();
	}

    virtual void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale) {}
	virtual void renderHorrible(float time, float r, float bob, float yRot, float xRot, float scale) {}
    virtual void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) {}
    virtual void prepareMobModel(Mob* mob, float time, float r, float a) {}

	float attackTime;
	bool riding;

	int texWidth;
	int texHeight;

	std::vector<ModelPart*> cubes;
	bool young;
private:
	//Map<String, TexOffs> mappedTexOffs = new HashMap<String, TexOffs>();
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__Model_H__*/
