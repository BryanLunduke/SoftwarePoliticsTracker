#ifndef NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__TileEntityRenderDispatcher_H__
#define NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__TileEntityRenderDispatcher_H__

//package net.minecraft.client.renderer.tileentity;

#include <map>
#include "../../../world/level/tile/entity/TileEntityRendererId.h"

class TileEntityRenderer;
class Level;
class Textures;
class TileEntity;
class Font;
class Mob;

class TileEntityRenderDispatcher
{
	typedef std::map<TileEntityRendererId, TileEntityRenderer*> RendererMap;
	typedef RendererMap::iterator RendererIterator;
	typedef RendererMap::const_iterator RendererCIterator;

public:
	~TileEntityRenderDispatcher();

	static TileEntityRenderDispatcher* getInstance();
	static void destroy();

	void setLevel(Level* level);
    void prepare(Level* level, Textures* textures, Font* font, Mob* player, float a);

    void render(TileEntity* e, float a);
    void render(TileEntity* entity, float x, float y, float z, float a);

    float distanceToSqr(float x, float y, float z);

    Font* getFont();

    TileEntityRenderer* getRenderer( TileEntity* entity );
    TileEntityRenderer* getRenderer( TileEntityRendererId rendererId );

	void onGraphicsReset();
private:
	TileEntityRenderDispatcher();
public:
    static float xOff, yOff, zOff;

    Textures* textures;
    Level* level;
    Mob* cameraEntity;
    float playerRotY;
    float playerRotX;

    float xPlayer, yPlayer, zPlayer;
private:
	static TileEntityRenderDispatcher* instance;

	Font* _font;
	RendererMap _renderers;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__TileEntityRenderDispatcher_H__*/
