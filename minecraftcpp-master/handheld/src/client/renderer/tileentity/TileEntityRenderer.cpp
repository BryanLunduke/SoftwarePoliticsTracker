#include "TileEntityRenderer.h"
#include "TileEntityRenderDispatcher.h"
#include "../Textures.h"
#include "../../gui/Font.h"
#include "../../../world/level/Level.h"
#include "../../../world/level/tile/entity/TileEntity.h"

TileEntityRenderer::TileEntityRenderer()
:   tileEntityRenderDispatcher(NULL)
{
}

void TileEntityRenderer::bindTexture( const std::string& resourceName )
{
	Textures* t = tileEntityRenderDispatcher->textures;
	if (t != NULL) t->loadAndBindTexture(resourceName);
}

Level* TileEntityRenderer::getLevel()
{
	return tileEntityRenderDispatcher->level;
}

void TileEntityRenderer::init( TileEntityRenderDispatcher* tileEntityRenderDispatcher )
{
	this->tileEntityRenderDispatcher = tileEntityRenderDispatcher;
}

Font* TileEntityRenderer::getFont()
{
	return tileEntityRenderDispatcher->getFont();
}
