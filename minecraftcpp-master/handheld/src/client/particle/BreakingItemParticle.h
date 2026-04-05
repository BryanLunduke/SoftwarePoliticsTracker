#ifndef NET_MINECRAFT_CLIENT_PARTICLE__BreakingItemParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__BreakingItemParticle_H__

//package net.minecraft.client.particle;

#include "Particle.h"
#include "../renderer/Tesselator.h"
#include "../../world/item/Item.h"
#include "../../world/level/Level.h"
#include "../../world/level/tile/Tile.h"

class BreakingItemParticle: public Particle
{
	typedef Particle super;
public:
    BreakingItemParticle(Level* level, float x, float y, float z, Item* item)
	:	super(level, x, y, z, 0, 0, 0)
	{
		_init(item);
    }

	BreakingItemParticle(Level* level, float x, float y, float z, float xa, float ya, float za, Item* item)
	:	super(level, x, y, z, 0, 0, 0)
	{
		_init(item);

		xd *= 0.1f;
		yd *= 0.1f;
		zd *= 0.1f;
		xd += xa;
		yd += ya;
		zd += za;
	}
	
	void _init(Item* item) {
		tex = item->getIcon(0);
		rCol = gCol = bCol = 1.0f;
		gravity = Tile::snow->gravity;
		size /= 2;
	}

    int getParticleTexture() {
        return ParticleEngine::ITEM_TEXTURE;
    }

    void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
        float u0 = (tex % 16 + uo / 4.0f) / 16.0f;
        float u1 = u0 + 0.999f / 16.0f / 4;
        float v0 = (tex / 16 + vo / 4.0f) / 16.0f;
        float v1 = v0 + 0.999f / 16.0f / 4;
        float r = 0.1f * size;

        float x = (float) (xo + (this->x - xo) * a - xOff);
        float y = (float) (yo + (this->y - yo) * a - yOff);
        float z = (float) (zo + (this->z - zo) * a - zOff);
        float br = getBrightness(a);
        t.color(br * rCol, br * gCol, br * bCol);

        t.vertexUV(x - xa * r - xa2 * r, y - ya * r, z - za * r - za2 * r, u0, v1);
        t.vertexUV(x - xa * r + xa2 * r, y + ya * r, z - za * r + za2 * r, u0, v0);
        t.vertexUV(x + xa * r + xa2 * r, y + ya * r, z + za * r + za2 * r, u1, v0);
        t.vertexUV(x + xa * r - xa2 * r, y - ya * r, z + za * r - za2 * r, u1, v1);
    }
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__BreakingItemParticle_H__*/
