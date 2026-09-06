#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__ThinFenceTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__ThinFenceTile_H__

//package net.minecraft.world.level.tile;

#include "Tile.h"
#include "../material/Material.h"
#include "../../phys/AABB.h"

class ThinFenceTile: public Tile
{
    typedef Tile super;
public:
    ThinFenceTile(int id, int tex, int edgeTex, const Material* material, bool dropsResources)
    :   super(id, tex, material),
        edgeTexture(edgeTex),
        dropsResources(dropsResources)
    {
    }

    /*@Override*/
    int getResource(int data, Random* random/*, int playerBonusLevel*/) {
        if (!dropsResources) {
            return 0;
        }
        return super::getResource(data, random/*, playerBonusLevel*/);
    }

    /*@Override*/
    bool isSolidRender() {
        return false;
    }

    /*@Override*/
    bool isCubeShaped() {
        return false;
    }

	int getRenderLayer() {
		return Tile::RENDERLAYER_ALPHATEST;
	}

    /*@Override*/
    int getRenderShape() {
        return Tile::SHAPE_IRON_FENCE;
    }

    /*@Override*/
    bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
        int id = level->getTile(x, y, z);
        if (id == this->id) return false;
        return super::shouldRenderFace(level, x, y, z, face);
    }

    /*@Override*/
    void addAABBs(Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes ) {
        bool n = attachsTo(level->getTile(x, y, z - 1));
        bool s = attachsTo(level->getTile(x, y, z + 1));
        bool w = attachsTo(level->getTile(x - 1, y, z));
        bool e = attachsTo(level->getTile(x + 1, y, z));

        if ((w && e) || (!w && !e && !n && !s)) {
            setShape(0, 0, 7.0f / 16.0f, 1, 1, 9.0f / 16.0f);
            super::addAABBs(level, x, y, z, box, boxes);
        } else if (w && !e) {
            setShape(0, 0, 7.0f / 16.0f, .5f, 1, 9.0f / 16.0f);
            super::addAABBs(level, x, y, z, box, boxes);
        } else if (!w && e) {
            setShape(.5f, 0, 7.0f / 16.0f, 1, 1, 9.0f / 16.0f);
            super::addAABBs(level, x, y, z, box, boxes);
        }
        if ((n && s) || (!w && !e && !n && !s)) {
            setShape(7.0f / 16.0f, 0, 0, 9.0f / 16.0f, 1, 1);
            super::addAABBs(level, x, y, z, box, boxes);
        } else if (n && !s) {
            setShape(7.0f / 16.0f, 0, 0, 9.0f / 16.0f, 1, .5f);
            super::addAABBs(level, x, y, z, box, boxes);
        } else if (!n && s) {
            setShape(7.0f / 16.0f, 0, .5f, 9.0f / 16.0f, 1, 1);
            super::addAABBs(level, x, y, z, box, boxes);
        }
    }

    /*@Override*/
    void updateDefaultShape() {
        setShape(0, 0, 0, 1, 1, 1);
    }

    /*@Override*/
    void updateShape(LevelSource* level, int x, int y, int z) {
        float minX = 7.0f / 16.0f;
        float maxX = 9.0f / 16.0f;
        float minZ = 7.0f / 16.0f;
        float maxZ = 9.0f / 16.0f;

        bool n = attachsTo(level->getTile(x, y, z - 1));
        bool s = attachsTo(level->getTile(x, y, z + 1));
        bool w = attachsTo(level->getTile(x - 1, y, z));
        bool e = attachsTo(level->getTile(x + 1, y, z));

        if ((w && e) || (!w && !e && !n && !s)) {
            minX = 0;
            maxX = 1;
        } else if (w && !e) {
            minX = 0;
        } else if (!w && e) {
            maxX = 1;
        }
        if ((n && s) || (!w && !e && !n && !s)) {
            minZ = 0;
            maxZ = 1;
        } else if (n && !s) {
            minZ = 0;
        } else if (!n && s) {
            maxZ = 1;
        }

        setShape(minX, 0, minZ, maxX, 1, maxZ);
    }

    int getEdgeTexture() {
        return edgeTexture;
    }

    const bool attachsTo(int tile) {
        return Tile::solid[tile] || tile == id || tile == Tile::glass->id;
    }
private:
    int edgeTexture;
    const bool dropsResources;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__ThinFenceTile_H__*/
