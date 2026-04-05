#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__EntityRenderDispatcher_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__EntityRenderDispatcher_H__

//package net.minecraft.client.renderer.entity;

#include <map>
#include "../../../world/entity/EntityRendererId.h"

class EntityRenderer;
class Level;
class Font;
class Textures;
class ItemInHandRenderer;
class Minecraft;
class Textures;
class Options;
class Entity;
class Mob;

class EntityRenderDispatcher
{
	typedef std::map<EntityRendererId, EntityRenderer*> RendererMap;
	typedef RendererMap::iterator RendererIterator;
	typedef RendererMap::const_iterator RendererCIterator;

public:
	EntityRenderDispatcher();
	~EntityRenderDispatcher();
	static EntityRenderDispatcher* getInstance();

	static void destroy();

    void prepare(Level* level, Font* font, Mob* player, Options* options, float a);

    void render(Entity* entity, float a);
    void render(Entity* entity, float x, float y, float z, float rot, float a);
	EntityRenderer* getRenderer( Entity* entity );
	EntityRenderer* getRenderer( EntityRendererId rendererId );

    void setLevel(Level* level);
	void setMinecraft(Minecraft* minecraft);

    float distanceToSqr(float x, float y, float z);

    Font* getFont();

	void onGraphicsReset();
private:
	void assign(EntityRendererId id, EntityRenderer* renderer);
public:
	ItemInHandRenderer* itemInHandRenderer;

	//TripodCameraRenderer tripodCameraRenderer;

	static float xOff, yOff, zOff;

	Textures* textures;
	Level* level;
	Minecraft* minecraft;
	Mob* cameraEntity;

	float playerRotY;
	float playerRotX;
	Options* options;

	float xPlayer, yPlayer, zPlayer;

private:
	static EntityRenderDispatcher* instance;

	Font* _font;
	// For onGraphicsReset etc
	RendererMap _renderers;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__EntityRenderDispatcher_H__*/
