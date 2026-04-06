#include "TileEntityRenderDispatcher.h"
#include "ChestRenderer.h"
#include "../Textures.h"
#include "../../gui/Font.h"
#include "../../../SharedConstants.h"
#include "../../../world/entity/Mob.h"
#include "../../../world/level/Level.h"
#include "../../../world/level/tile/entity/TileEntity.h"
#include "SignRenderer.h"

/*static*/
TileEntityRenderDispatcher* TileEntityRenderDispatcher::instance = 0;
/*static*/
float	TileEntityRenderDispatcher::xOff = 0,
		TileEntityRenderDispatcher::yOff = 0,
		TileEntityRenderDispatcher::zOff = 0;

TileEntityRenderDispatcher* TileEntityRenderDispatcher::getInstance()
{
	if (!instance) instance = new TileEntityRenderDispatcher();
	return instance;
}

TileEntityRenderDispatcher::~TileEntityRenderDispatcher()
{
	std::set<TileEntityRenderer*> destroyed;
	for (RendererCIterator cit = _renderers.begin(); cit != _renderers.end(); ++cit) {
		if (destroyed.find(cit->second) == destroyed.end()) {
			destroyed.insert(cit->second);
			delete cit->second;
		}
	}
}

void TileEntityRenderDispatcher::destroy()
{
	if (instance) {
		delete instance;
		instance = NULL;
	}
}

void TileEntityRenderDispatcher::prepare( Level* level, Textures* textures, Font* font, Mob* player, float a )
{
	if (this->level != level)
		setLevel(level);

	this->textures = textures;
	this->cameraEntity = player;
	this->_font = font;

	playerRotY = player->yRotO + (player->yRot - player->yRotO) * a;
	playerRotX = player->xRotO + (player->xRot - player->xRotO) * a;

	xPlayer = player->xOld + (player->x - player->xOld) * a;
	yPlayer = player->yOld + (player->y - player->yOld) * a;
	zPlayer = player->zOld + (player->z - player->zOld) * a;

	//        xPlayer -= xPlayerOffs;
	//        yPlayer -= yPlayerOffs;
	//        zPlayer -= zPlayerOffs;
}

void TileEntityRenderDispatcher::render( TileEntity* e, float a )
{
	if (e->distanceToSqr(xPlayer, yPlayer, zPlayer) < 64 * 64) {
		float br = level->getBrightness(e->x, e->y, e->z);
		glColor4f(br, br, br, 1);
		render(e, e->x - xOff, e->y - yOff, e->z - zOff, a);
	}
}

void TileEntityRenderDispatcher::render( TileEntity* entity, float x, float y, float z, float a )
{
	TileEntityRenderer* renderer = getRenderer(entity);
	if (renderer != NULL) {
		renderer->render(entity, x, y, z, a);
	}
}

void TileEntityRenderDispatcher::setLevel( Level* level )
{
	this->level = level;

	for (RendererCIterator cit = _renderers.begin(); cit != _renderers.end(); ++cit)
		if (cit->second != NULL)
			cit->second->onNewLevel(level);
}

float TileEntityRenderDispatcher::distanceToSqr( float x, float y, float z )
{
	float xd = x - xPlayer;
	float yd = y - yPlayer;
	float zd = z - zPlayer;
	return xd * xd + yd * yd + zd * zd;
}

Font* TileEntityRenderDispatcher::getFont()
{
	return _font;
}

TileEntityRenderer* TileEntityRenderDispatcher::getRenderer( TileEntity* entity )
{
	TileEntityRendererId rendererId = entity->rendererId;

	//if (rendererId == TR_QUERY_RENDERER)
	//	rendererId = entity->queryEntityRenderer();

	return getRenderer(rendererId);
}

TileEntityRenderer* TileEntityRenderDispatcher::getRenderer( TileEntityRendererId rendererId )
{
	TileEntityRenderer* renderer = NULL;
	RendererCIterator cit = _renderers.find(rendererId);
	if (cit != _renderers.end()) {
		renderer = cit->second;
	}
	return renderer;
}

TileEntityRenderDispatcher::TileEntityRenderDispatcher()
{
	_renderers.insert(std::make_pair(TR_CHEST_RENDERER, new ChestRenderer()));
    _renderers.insert(std::make_pair(TR_SIGN_RENDERER, new SignRenderer()));

	for (RendererCIterator cit = _renderers.begin(); cit != _renderers.end(); ++cit)
		cit->second->init(this);
}

void TileEntityRenderDispatcher::onGraphicsReset() {
	for (RendererIterator it = _renderers.begin(); it != _renderers.end(); ++it) {
		it->second->onGraphicsReset();
	}
}
