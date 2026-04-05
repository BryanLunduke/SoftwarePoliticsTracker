#include "EntityTileRenderer.h"

EntityTileRenderer* EntityTileRenderer::instance = new EntityTileRenderer();

void EntityTileRenderer::render( Tile* tile, int data, float brightness )
{
	TileEntityRenderDispatcher::getInstance()->render(&chest, 0, 0, 0, 0);
}
