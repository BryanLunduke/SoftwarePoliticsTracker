#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TripodCameraRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TripodCameraRenderer_H__

#include "EntityRenderer.h"
#include "../TileRenderer.h"
#include "../../model/geom/ModelPart.h"
#include "../../../world/level/tile/Tile.h"


class TripodCamera;

class TripodCameraRenderer: public EntityRenderer
{
    TileRenderer tileRenderer;

public:
    TripodCameraRenderer();

	void render(Entity* cam_, float x, float y, float z, float rot, float a);
	//void render( Tesselator &t );

	static float getFlashTime(const TripodCamera* c, float a);

private:
	Tile tripod;
	ModelPart cameraCube;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TripodCameraRenderer_H__*/
