#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ArrowRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ArrowRenderer_H__
#include "EntityRenderer.h"

class ArrowRenderer : public EntityRenderer {
	void render(Entity* arrow, float x, float y, float z, float rot, float a);
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ArrowRenderer_H__ */