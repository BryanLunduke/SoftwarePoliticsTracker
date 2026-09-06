#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PaintingRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PaintingRenderer_H__
#include "EntityRenderer.h"
class Painting;
class PaintingRenderer : public EntityRenderer {
public:
	void render(Entity* entity, float x, float y, float z, float rot, float a);
private:
	void renderPainting(Painting* painting, int w, int h, int uo, int vo, float a);
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PaintingRenderer_H__ */