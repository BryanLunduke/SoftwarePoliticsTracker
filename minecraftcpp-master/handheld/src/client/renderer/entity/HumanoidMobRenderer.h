#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HumanoidMobRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HumanoidMobRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "MobRenderer.h"

class HumanoidModel;
class Mob;

class HumanoidMobRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
    HumanoidMobRenderer(HumanoidModel* humanoidModel, float shadow);

	void renderHand();
	void render(Entity* mob_, float x, float y, float z, float rot, float a);
protected:
    void additionalRendering(Mob* mob, float a);

private:
	HumanoidModel* humanoidModel;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__HumanoidMobRenderer_H__*/
