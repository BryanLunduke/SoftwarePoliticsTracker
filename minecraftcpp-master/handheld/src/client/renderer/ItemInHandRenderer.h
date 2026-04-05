#ifndef NET_MINECRAFT_CLIENT_RENDERER__ItemInHandRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER__ItemInHandRenderer_H__

//package net.minecraft.client.renderer;

#include "TileRenderer.h"
#include "../renderer/RenderChunk.h"
#include "../../world/item/ItemInstance.h"

class Minecraft;

typedef struct RenderCall {
	int itemId;
	RenderChunk chunk;
	std::string	texture;
	bool		isFlat;
} RenderCall;


class ItemInHandRenderer
{
public:
    ItemInHandRenderer(Minecraft* mc);

	void tick();

	void render(float a);
    void renderItem(Mob* player, ItemInstance* item);
    void renderScreenEffect(float a);

	void itemPlaced();
	void itemUsed();

	void onGraphicsReset();

private:
	void renderTex(float a, int tex);
    void renderWater(float a);
    void renderFire(float a);

private:
	int lastIconRendered;
	int lastItemRendered;
	int lastSlot;
	ItemInstance item;

	Minecraft* mc;
	//ItemInstance* selectedItem;
	float height;
	float oHeight;
	TileRenderer tileRenderer;

	static const int MaxNumRenderObjects = 512;
	RenderCall renderObjects[MaxNumRenderObjects];
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__ItemInHandRenderer_H__*/
