#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"

#include "../Level.h"
#include "Tile.h"

/*abstract*/
class LiquidTile: public Tile
{
public: 
    static float getHeight(int d) {
        //        if (d == 0) d++; // NM
        if (d >= 8) d = 0;
        float h = (d + 1) / 9.0f;
        return h;
    }

    int getTexture(int face) {
        if (face == 0 || face == 1) {
            return tex;
        } else {
            return tex + 1;
        }
    }

    bool isCubeShaped() {
        return false;
    }

    bool isSolidRender() {
        return false;
    }

    bool mayPick(int data, bool liquid) {
        return liquid && data == 0;
    }

    bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
        const Material* m = level->getMaterial(x, y, z);
        if (m == this->material) return false;
        if (m == Material::ice) return false;
        if (face == 1) return true;
        return Tile::shouldRenderFace(level, x, y, z, face);
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
        return NULL;
    }

    int getRenderShape() {
        return Tile::SHAPE_WATER;
    }

    int getResource(int data, Random* random) {
        return 0;
    }

    int getResourceCount(Random* random) {
        return 0;
    }

	int getColor(LevelSource* level, int x, int y, int z) {
        return 0x999999ff;
    }

	void handleEntityInside(Level* level, int x, int y, int z, Entity* e, Vec3& current) {
        Vec3 flow = getFlow(level, x, y, z);
        current.x += flow.x * .5f;
        current.y += flow.y * .5f;
        current.z += flow.z * .5f;
    }

    int getTickDelay() {
        if (material == Material::water) return 5;
        if (material == Material::lava) return 30;
        return 0;
    }

    float getBrightness(LevelSource* level, int x, int y, int z) {
        float a = level->getBrightness(x, y, z);
        float b = level->getBrightness(x, y + 1, z);
        return a > b ? a : b;
    }

    virtual void tick(Level* level, int x, int y, int z, Random* random) {
        Tile::tick(level, x, y, z, random);
    }

    int getRenderLayer() {
        return (material == Material::water)? Tile::RENDERLAYER_BLEND : Tile::RENDERLAYER_OPAQUE;
    }

    void animateTick(Level* level, int x, int y, int z, Random* random) {
        if (material == Material::water && random->nextInt(64) == 0) {
            int d = level->getData(x, y, z);
            if (d > 0 && d < 8) {
                //level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "liquid.water", random.nextFloat() * 0.25f + 0.75f, random.nextFloat() * 1.0f + 0.5f);
            }
        }
        if (material == Material::lava) {
            if (level->getMaterial(x, y + 1, z) == Material::air && !level->isSolidRenderTile(x, y + 1, z)) {
                if (random->nextInt(100) == 0) {
                    float xx = x + random->nextFloat();
                    float yy = y + yy1;
                    float zz = z + random->nextFloat();
                    level->addParticle(PARTICLETYPE(lava), xx, yy, zz, 0, 0, 0);
                }
            }
        }
    }

    static float getSlopeAngle(LevelSource* level, int x, int y, int z, const Material* m) {
        Vec3 flow;
        if (m == Material::water) flow = ((LiquidTile*) Tile::water)->getFlow(level, x, y, z);
        if (m == Material::lava) flow = ((LiquidTile*) Tile::lava)->getFlow(level, x, y, z);
        if (flow.x == 0 && flow.z == 0) return -1000;
        return atan2(flow.z, flow.x) - Mth::PI * 0.5f;
    }

    virtual void onPlace(Level* level, int x, int y, int z) {
        updateLiquid(level, x, y, z);
    }

    virtual void neighborChanged(Level* level, int x, int y, int z, int type) {
        updateLiquid(level, x, y, z);
    }

protected:
	LiquidTile(int id, const Material* material)
		: Tile(id, (material == Material::lava ? 14 : 12) * 16 + 13, material)
	{
        float yo = 0;
        float e = 0;

		setShape(0 + e, 0 + yo, 0 + e, 1 + e, 1 + yo, 1 + e);
        setTicking(true);
    }

	void fizz(Level* level, int x, int y, int z) {
        //level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.fizz", 0.5f, 2.6f + (level->random.nextFloat() - level->random.nextFloat()) * 0.8f);
        for (int i = 0; i < 8; i++) {
            level->addParticle(PARTICLETYPE(largesmoke), (float)x + Mth::random(), (float)y + 1.2f, (float)z + Mth::random(), 0, 0, 0);
        }
    }
    int getDepth(Level* level, int x, int y, int z) {
        if (level->getMaterial(x, y, z) != material) return -1;
        else return level->getData(x, y, z);
    }

    int getRenderedDepth(LevelSource* level, int x, int y, int z) {
        if (level->getMaterial(x, y, z) != material) return -1;
        int d = level->getData(x, y, z);
        if (d >= 8) d = 0;
        return d;
    }

private:
    void updateLiquid(Level* level, int x, int y, int z) {
        if (level->getTile(x, y, z) != id) return;
        if (material == Material::lava) {
            bool water = false;
            if (water || level->getMaterial(x, y, z - 1) == Material::water) water = true;
            if (water || level->getMaterial(x, y, z + 1) == Material::water) water = true;
            if (water || level->getMaterial(x - 1, y, z) == Material::water) water = true;
            if (water || level->getMaterial(x + 1, y, z) == Material::water) water = true;
            //            if (water || level->getMaterial(x, y - 1, z) == Material::water) water = true; // NM
            if (water || level->getMaterial(x, y + 1, z) == Material::water) water = true;
            if (water) {
                int data = level->getData(x, y, z);
                if (data == 0) {
                    level->setTile(x, y, z, Tile::obsidian->id);
                } else if (data <= 4) {
                    level->setTile(x, y, z, Tile::stoneBrick->id);
                }
                fizz(level, x, y, z);
            }
        }
    }
   
	Vec3 getFlow(LevelSource* level, int x, int y, int z) {
        Vec3 flow(0,0,0);
        int mid = getRenderedDepth(level, x, y, z);
        for (int d = 0; d < 4; d++) {

            int xt = x;
            int yt = y;
            int zt = z;

            if (d == 0) xt--;
            if (d == 1) zt--;
            if (d == 2) xt++;
            if (d == 3) zt++;

            int t = getRenderedDepth(level, xt, yt, zt);
            if (t < 0) {
                if (!level->getMaterial(xt, yt, zt)->blocksMotion()) {
                    t = getRenderedDepth(level, xt, yt - 1, zt);
                    if (t >= 0) {
                        int dir = t - (mid - 8);
                        flow = flow.add((float)((xt - x) * dir), (float)((yt - y) * dir), (float)((zt - z) * dir));
                    }
                }
            } else {
                if (t >= 0) {
                    int dir = t - mid;
                    flow = flow.add((float)((xt - x) * dir), (float)((yt - y) * dir), (float)((zt - z) * dir));
                }
            }
        }
        if (level->getData(x, y, z) >= 8) {
            bool ok = false;
            if (ok || shouldRenderFace(level, x, y, z - 1, 2)) ok = true;
            if (ok || shouldRenderFace(level, x, y, z + 1, 3)) ok = true;
            if (ok || shouldRenderFace(level, x - 1, y, z, 4)) ok = true;
            if (ok || shouldRenderFace(level, x + 1, y, z, 5)) ok = true;
            if (ok || shouldRenderFace(level, x, y + 1, z - 1, 2)) ok = true;
            if (ok || shouldRenderFace(level, x, y + 1, z + 1, 3)) ok = true;
            if (ok || shouldRenderFace(level, x - 1, y + 1, z, 4)) ok = true;
            if (ok || shouldRenderFace(level, x + 1, y + 1, z, 5)) ok = true;
            if (ok) flow = flow.normalized().add(0, -6, 0);
        }
        flow = flow.normalized();
        return flow;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTile_H__*/
