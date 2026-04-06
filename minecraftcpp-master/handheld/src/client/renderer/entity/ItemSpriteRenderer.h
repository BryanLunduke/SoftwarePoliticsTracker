#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemSpriteRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemSpriteRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "EntityRenderer.h"

class ItemSpriteRenderer: public EntityRenderer
{
public:
    ItemSpriteRenderer(int icon);

	void render(Entity* e, float x, float y, float z, float rot, float a);
private:
	int icon;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemSpriteRenderer_H__*/
