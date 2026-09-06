#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "EntityRenderer.h"
#include "../../../util/Random.h"

class Font;
class Entity;
class ItemInstance;
class Textures;
class Tesselator;
class TileRenderer;

class ItemRenderer: public EntityRenderer
{
public:
    ItemRenderer();

    void render(Entity* itemEntity_, float x, float y, float z, float rot, float a);
    static void renderGuiItem(Font* font, Textures* textures, const ItemInstance* item, float x, float y, bool fancy);
	static void renderGuiItem(Font* font, Textures* textures, const ItemInstance* item, float x, float y, float w, float h, bool fancy);
	static void renderGuiItemCorrect(Font* font, Textures* textures, const ItemInstance* item, int x, int y);
	//void renderGuiItemDecorations(Font* font, Textures* textures, ItemInstance* item, int x, int y);
	static void renderGuiItemDecorations(const ItemInstance* item, float x, float y);

	static void blit(float x, float y, float sx, float sy, float w, float h);
	static int  getAtlasPos(const ItemInstance* item);

	static void teardown_static();
private:
	static void fillRect(Tesselator& t, float x, float y, float w, float h, int c);
	static TileRenderer* tileRenderer;
	Random random;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__ItemRenderer_H__*/
