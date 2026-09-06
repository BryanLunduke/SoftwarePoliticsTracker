#include "Level.h"
#include "LightUpdate.h"
#include "tile/Tile.h"

//LightUpdate::LightUpdate()
//:
//	layer(&LightLayer::Sky),
//	x0(0),
//	y0(0),
//	z0(0),
//	x1(1),
//	y1(1),
//	z1(1)
//{} // for using std::set

LightUpdate::LightUpdate(const LightUpdate* t)
:
	layer(t->layer),
	x0(t->x0),
	y0(t->y0),
	z0(t->z0),
	x1(t->x1),
	y1(t->y1),
	z1(t->z1)
{

}

LightUpdate::LightUpdate(const LightUpdate& t)
:
	layer(t.layer),
	x0(t.x0),
	y0(t.y0),
	z0(t.z0),
	x1(t.x1),
	y1(t.y1),
	z1(t.z1)
{

}

LightUpdate::LightUpdate(const LightLayer& _layer, int _x0, int _y0, int _z0, int _x1, int _y1, int _z1)
:
    layer(&_layer),
    x0(_x0),
    y0(_y0),
    z0(_z0),
    x1(_x1),
    y1(_y1),
    z1(_z1)
{
}

void LightUpdate::operator=(const LightUpdate* t)
{
	layer = t->layer;
	x0 = t->x0;
	y0 = t->y0;
	z0 = t->z0;
	x1 = t->x1;
	y1 = t->y1;
	z1 = t->z1;
}

void LightUpdate::update(Level* level)
{
    int xd = (x1 - x0) + 1;
    int yd = (y1 - y0) + 1;
    int zd = (z1 - z0) + 1;
    int size = xd * yd * zd;
    if (size > 16 * 16 * 128) {
        //System.out.println("Light too large, skipping!");
        return;
    }


    int lastxc = 0;
    int lastzc = 0;
    bool hasLast = false;
    bool lastOk = false;

    for (int x = x0; x <= x1; x++)
        for (int z = z0; z <= z1; z++) {
            int xc = x >> 4;
            int zc = z >> 4;
            bool ok = false;

            if (hasLast && xc == lastxc && zc == lastzc) {
                ok = lastOk;
            } else {
                ok = level->hasChunksAt(x, 0, z, 1);
                if (ok) {
                    LevelChunk* lc = level->getChunk(x >> 4, z >> 4);
                    if (lc->isEmpty()) ok = false;
                }
                lastOk = ok;
                lastxc = xc;
                lastzc = zc;
            }

            if (ok) {

                if (y0 < 0) y0 = 0;
                if (y1 >= Level::DEPTH) y1 = Level::DEPTH - 1;

                for (int y = y0; y <= y1; y++) {
                    int old = level->getBrightness(*layer, x, y, z);

                    int target = 0;
                    int tile = level->getTile(x, y, z);
                    int block = Tile::lightBlock[tile];
                    if (block == 0) block = 1;
                    int emit = 0;
                    if (layer == &LightLayer::Sky) {
                        if (level->isSkyLit(x, y, z)) emit = 15;
                    } else if (layer == &LightLayer::Block) {
                        emit = Tile::lightEmission[tile];
                    }

                    if (block >= 15 && emit == 0) {
                        target = 0;
                    } else {

                        int d0 = level->getBrightness(*layer, x - 1, y, z);
                        int d1 = level->getBrightness(*layer, x + 1, y, z);
                        int d2 = level->getBrightness(*layer, x, y - 1, z);
                        int d3 = level->getBrightness(*layer, x, y + 1, z);
                        int d4 = level->getBrightness(*layer, x, y, z - 1);
                        int d5 = level->getBrightness(*layer, x, y, z + 1);

                        target = d0;
                        if (d1 > target) target = d1;
                        if (d2 > target) target = d2;
                        if (d3 > target) target = d3;
                        if (d4 > target) target = d4;
                        if (d5 > target) target = d5;
                        target -= block;
                        if (target < 0) target = 0;

                        if (emit > target) target = emit;
                    }


                    if (old != target) {
                        level->setBrightness(*layer, x, y, z, target);
                        int t = target - 1;
                        if (t < 0) t = 0;
                        level->updateLightIfOtherThan(*layer, x - 1, y, z, t);
                        level->updateLightIfOtherThan(*layer, x, y - 1, z, t);
                        level->updateLightIfOtherThan(*layer, x, y, z - 1, t);

                        if (x + 1 >= x1) level->updateLightIfOtherThan(*layer, x + 1, y, z, t);
                        if (y + 1 >= y1) level->updateLightIfOtherThan(*layer, x, y + 1, z, t);
                        if (z + 1 >= z1) level->updateLightIfOtherThan(*layer, x, y, z + 1, t);
                    }
                }
            }
        }
}

bool LightUpdate::expandToContain(int _x0, int _y0, int _z0, int _x1, int _y1, int _z1) {
    if (_x0 >= x0 && _y0 >= y0 && _z0 >= z0 && _x1 <= x1 && _y1 <= y1 && _z1 <= z1) return true;

    int r = 1;
    if (_x0 >= x0 - r && _y0 >= y0 - r && _z0 >= z0 - r && _x1 <= x1 + r && _y1 <= y1 + r && _z1 <= z1 + r) {
        int xs = x1 - x0;
        int ys = y1 - y0;
        int zs = z1 - z0;

        if (_x0 > x0) _x0 = x0;
        if (_y0 > y0) _y0 = y0;
        if (_z0 > z0) _z0 = z0;
        if (_x1 < x1) _x1 = x1;
        if (_y1 < y1) _y1 = y1;
        if (_z1 < z1) _z1 = z1;

        int _xs = _x1 - _x0;
        int _ys = _y1 - _y0;
        int _zs = _z1 - _z0;

        int os = xs * ys * zs;
        int ns = _xs * _ys * _zs;
        if (ns - os <= 2) {
            x0 = _x0;
            y0 = _y0;
            z0 = _z0;
            x1 = _x1;
            y1 = _y1;
            z1 = _z1;
            return true;
        }
    }
    return false;
}