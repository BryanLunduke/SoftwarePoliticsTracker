#include "SandTile.h"
#include "../../entity/item/FallingTile.h"

bool SandTile::instaFall = false;

/*private*/
void SandTile::checkSlide(Level* level, int x, int y, int z)
{
    int x2 = x;
    int y2 = y;
    int z2 = z;
    if (isFree(level, x2, y2 - 1, z2) && y2 >= 0) {
        int r = 32;
        if (instaFall || !level->hasChunksAt(x - r, y - r, z - r, x + r, y + r, z + r)) {
            level->setTile(x, y, z, 0);
            while (isFree(level, x, y - 1, z) && y > 0)
                y--;
            if (y > 0) {
                level->setTile(x, y, z, id);
            }
        } else {
            FallingTile* e = new FallingTile(level, x + 0.5f, y + 0.5f, z + 0.5f, id);
            level->addEntity(e);
        }
    }
}
