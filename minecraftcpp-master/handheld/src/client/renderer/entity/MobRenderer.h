#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MobRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MobRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "EntityRenderer.h"

#include <string>

class Model;
class Entity;
class Mob;

class MobRenderer: public EntityRenderer
{
	static const int MAX_ARMOR_LAYERS = 4;
public:
	// @note: MobRenderer::model will be deleted automagically
	// Armor models (setArmor(Model*)) is to be deleted by derived classes.
    MobRenderer(Model* model, float shadow);
	~MobRenderer();

	virtual int prepareArmor(Mob* mob, int layer, float a);

	virtual void setupPosition(Entity* mob, float x, float y, float z);
    virtual void setupRotations(Entity* mob_, float bob, float bodyRot, float a);

    virtual float getAttackAnim(Mob* mob, float a);
    virtual float getBob(Mob* mob, float a);

    virtual float getFlipDegrees(Mob* mob);

    virtual int getOverlayColor(Mob* mob, float br, float a);

    virtual void scale(Mob* mob, float a);

	virtual void render(Entity* mob_, float x, float y, float z, float rot, float a);
	virtual void renderName(Mob* mob, float x, float y, float z);
	virtual void renderNameTag(Mob* mob, const std::string& name, float x, float y, float z, int maxDist);
	virtual void additionalRendering(Mob* mob, float a);

	virtual void onGraphicsReset();
protected:
	void setArmor(Model* armor);
	Model* getArmor();
private:
	Model* model;
	Model* armor;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__MobRenderer_H__*/
