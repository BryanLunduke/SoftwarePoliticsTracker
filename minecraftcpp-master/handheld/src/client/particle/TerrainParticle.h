#ifndef NET_MINECRAFT_CLIENT_PARTICLE__TerrainParticle_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__TerrainParticle_H__

//package net.minecraft.client.particle;

#include "../renderer/Tesselator.h"
#include "../../world/level/Level.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/level/tile/GrassTile.h"

class TerrainParticle: public Particle
{
	typedef Particle super;

public:
    TerrainParticle(Level* level, float x, float y, float z, float xa, float ya, float za, Tile* tile, int data)
	:	super(level, x, y, z, xa, ya, za),
		tile(tile)
	{
        tex = tile->getTexture(2, data);
        gravity = tile->gravity;
        rCol = gCol = bCol = 0.6f;
        size /= 2;
		//noPhysics = true;
    }

    TerrainParticle* init(int x, int y, int z) {
        if (tile == Tile::grass) return this;
        int col = tile->getColor(level, x, y, z);
        rCol *= ((col >> 16) & 0xff) / 255.0f;
        gCol *= ((col >> 8) & 0xff) / 255.0f;
        bCol *= ((col) & 0xff) / 255.0f;
        return this;
    }

    int getParticleTexture() {
        return ParticleEngine::TERRAIN_TEXTURE;
    }

    void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
        float u0 = ((tex & 15) + uo / 4.0f) / 16.0f;
        float u1 = u0 + 0.999f / 16.0f / 4;
        float v0 = ((tex >> 4) + vo / 4.0f) / 16.0f;
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

private:
	Tile* tile;
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__TerrainParticle_H__*/
