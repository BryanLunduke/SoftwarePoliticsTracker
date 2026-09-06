#include "EntityRenderDispatcher.h"

#include "../../Options.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/item/Item.h"
#include "../../../world/level/Level.h"
#include "../../../world/level/tile/Tile.h"
#include "../../model/ModelInclude.h"

#include "ItemSpriteRenderer.h"
#include "FallingTileRenderer.h"

#include "HumanoidMobRenderer.h"
#include "ItemRenderer.h"
#include "TntRenderer.h"
#include "TripodCameraRenderer.h"
#include "PigRenderer.h"
#include "MobRenderer.h"
#include "PlayerRenderer.h"

#include "CreeperRenderer.h"
#include "SpiderRenderer.h"

#include "ChickenRenderer.h"
#include "SheepRenderer.h"
#include "ArrowRenderer.h"
#include "PaintingRenderer.h"


/*static*/
EntityRenderDispatcher* EntityRenderDispatcher::instance = NULL;

/*static*/
float	EntityRenderDispatcher::xOff = 0,
		EntityRenderDispatcher::yOff = 0,
		EntityRenderDispatcher::zOff = 0;

EntityRenderDispatcher::EntityRenderDispatcher()
:	itemInHandRenderer(NULL)
{
	//@note: The Models (model/armor) will be deleted by resp. MobRenderer
	assign( ER_ITEM_RENDERER,       new ItemRenderer());
	assign(	ER_HUMANOID_RENDERER,	new HumanoidMobRenderer(new HumanoidModel(), 0));
	assign(	ER_PIG_RENDERER,		new PigRenderer(new PigModel(), NULL/*new PigModel(0.5f)*/, 0));
	assign(	ER_COW_RENDERER,		new MobRenderer(new CowModel(), 0));
	assign(	ER_CHICKEN_RENDERER,	new ChickenRenderer( new ChickenModel(), 0));
	assign(	ER_SHEEP_RENDERER,	    new SheepRenderer(new SheepModel(), new SheepFurModel(), 0));
	assign(	ER_SKELETON_RENDERER,	new HumanoidMobRenderer(new SkeletonModel(), 0.5f));
	assign(	ER_ZOMBIE_RENDERER,		new HumanoidMobRenderer(new ZombieModel(), 0.5f));
	assign(	ER_CREEPER_RENDERER,	new CreeperRenderer());
	assign(	ER_SPIDER_RENDERER,		new SpiderRenderer());
	assign(	ER_TNT_RENDERER,		new TntRenderer());
	assign(	ER_ARROW_RENDERER,		new ArrowRenderer());
	assign( ER_PLAYER_RENDERER,		new PlayerRenderer(new HumanoidModel(), 0));
	assign( ER_THROWNEGG_RENDERER,  new ItemSpriteRenderer(Item::egg->getIcon(0)));
	assign( ER_SNOWBALL_RENDERER,   new ItemSpriteRenderer(Item::snowBall->getIcon(0)));
	assign( ER_PAINTING_RENDERER,   new PaintingRenderer());
	assign( ER_FALLINGTILE_RENDERER,new FallingTileRenderer());

	for (RendererIterator it = _renderers.begin(); it != _renderers.end(); ++it) {
		it->second->init(this);
	}
}

void EntityRenderDispatcher::destroy() 
{
	if (instance) {
		delete instance;
		instance = NULL;
	}
}

EntityRenderDispatcher* EntityRenderDispatcher::getInstance()
{
	if (!instance)
		instance = new EntityRenderDispatcher();

	return instance;
}

EntityRenderDispatcher::~EntityRenderDispatcher()
{
	std::set<EntityRenderer*> destroyed;
	for (RendererCIterator cit = _renderers.begin(); cit != _renderers.end(); ++cit) {
		if (destroyed.find(cit->second) == destroyed.end()) {
			destroyed.insert(cit->second);
			delete cit->second;
		}
	}
}

void EntityRenderDispatcher::prepare( Level* level, Font* font, Mob* player, Options* options, float a )
{
	this->level = level;
	this->options = options;
	this->cameraEntity = player;
	this->_font = font;
	if(player->isSleeping()) {
		int t = level->getTile(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
		if (t == Tile::bed->id) {
			int data = level->getData(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));

			int direction = data & 3;
			playerRotY = float(direction * 90 + 180);
			playerRotX = 0;
		}
	}
	else {
		playerRotY = player->yRotO + (player->yRot - player->yRotO) * a;
		playerRotX = player->xRotO + (player->xRot - player->xRotO) * a;
	}
	
	xPlayer = player->xOld + (player->x - player->xOld) * a;
	yPlayer = player->yOld + (player->y - player->yOld) * a;
	zPlayer = player->zOld + (player->z - player->zOld) * a;
}

void EntityRenderDispatcher::render( Entity* entity, float a )
{
	float x = entity->xOld + (entity->x - entity->xOld) * a;
	float y = entity->yOld + (entity->y - entity->yOld) * a;
	float z = entity->zOld + (entity->z - entity->zOld) * a;
	float r = entity->yRotO + (entity->yRot - entity->yRotO) * a;

	float br = entity->getBrightness(a);
	glColor4f2(br, br, br, 1);

	render(entity, x - xOff, y - yOff, z - zOff, r, a);
}

void EntityRenderDispatcher::render( Entity* entity, float x, float y, float z, float rot, float a )
{
	EntityRenderer* renderer = getRenderer(entity);
	if (renderer != NULL) {
		renderer->render(entity, x, y, z, rot, a);
		//renderer->postRender(entity, x, y, z, rot, a);
	}
}

EntityRenderer* EntityRenderDispatcher::getRenderer( Entity* entity )
{
	EntityRendererId rendererId = entity->entityRendererId;

	if (rendererId == ER_QUERY_RENDERER)
		rendererId = entity->queryEntityRenderer();

	return getRenderer(rendererId);
}

EntityRenderer* EntityRenderDispatcher::getRenderer( EntityRendererId rendererId )
{

	EntityRenderer* renderer = NULL;
	RendererCIterator cit = _renderers.find(rendererId);
	if (cit != _renderers.end()) {
		renderer = cit->second;
	}
	return renderer;
}

void EntityRenderDispatcher::setLevel( Level* level )
{
	this->level = level;
}

void EntityRenderDispatcher::setMinecraft( Minecraft* minecraft )
{
	this->minecraft = minecraft;
}

float EntityRenderDispatcher::distanceToSqr( float x, float y, float z )
{
	float xd = x - xPlayer;
	float yd = y - yPlayer;
	float zd = z - zPlayer;
	return xd * xd + yd * yd + zd * zd;
}

Font* EntityRenderDispatcher::getFont()
{
	return _font;
}

void EntityRenderDispatcher::onGraphicsReset()
{
	for (RendererIterator it = _renderers.begin(); it != _renderers.end(); ++it) {
		it->second->onGraphicsReset();
	}
}

void EntityRenderDispatcher::assign( EntityRendererId id, EntityRenderer* renderer )
{
	_renderers.insert(std::make_pair(id, renderer));
}
