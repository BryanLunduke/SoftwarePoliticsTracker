#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SheepRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SheepRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "MobRenderer.h"
class Mob;

class SheepRenderer: public MobRenderer
{
    typedef MobRenderer super;

public:
    SheepRenderer(Model* model, Model* armor, float shadow);
	~SheepRenderer();
protected:
	int prepareArmor(Mob* sheep, int layer, float a);
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SheepRenderer_H__*/
