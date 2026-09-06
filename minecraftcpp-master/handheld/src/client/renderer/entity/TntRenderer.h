#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TntRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TntRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "EntityRenderer.h"
#include "../TileRenderer.h"

class TntRenderer: public EntityRenderer
{
public:
    TntRenderer();
	void render(Entity* tnt_, float x, float y, float z, float rot, float a);

	TileRenderer tileRenderer;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TntRenderer_H__*/
