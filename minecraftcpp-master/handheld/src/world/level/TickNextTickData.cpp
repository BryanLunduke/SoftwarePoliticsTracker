#include "TickNextTickData.h"

long TickNextTickData::C = 0;

TickNextTickData::TickNextTickData(int x_, int y_, int z_, int tileId_)
	: x(x_), y(y_), z(z_), tileId(tileId_), c(++C)
{
}

bool TickNextTickData::operator==(const TickNextTickData& t) const {
    return x == t.x && y == t.y && z == t.z && tileId == t.tileId;
}

int TickNextTickData::hashCode() const {
    return (((x * 128 * 1024) + (z * 128) + y) * 256) + tileId;
}

TickNextTickData* TickNextTickData::setDelay(long l) {
    this->delay = l;
    return this;
}

bool TickNextTickData::operator<(const TickNextTickData& tnd) const {
    if (delay < tnd.delay) return true;
    if (delay > tnd.delay) return false;
    if (c < tnd.c) return true;
    if (c > tnd.c) return false;
    return false;
}
