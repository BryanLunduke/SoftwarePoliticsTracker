#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ChickenRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ChickenRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "MobRenderer.h"

class Mob;

class ChickenRenderer: public MobRenderer
{
    typedef MobRenderer super;
public:
    ChickenRenderer(Model* model, float shadow);

    void render(Entity* mob, float x, float y, float z, float rot, float a);

protected:
    float getBob(Mob* mob_, float a);
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ChickenRenderer_H__*/
