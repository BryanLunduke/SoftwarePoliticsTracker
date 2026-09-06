#ifndef NET_MINECRAFT_WORLD_LEVEL__TickNextTickData_H__
#define NET_MINECRAFT_WORLD_LEVEL__TickNextTickData_H__

//package net.minecraft.world.level;

class TickNextTickData
{
    static long C;
    long c;

public:
    int x, y, z, tileId;
    long delay;

	TickNextTickData(int x_, int y_, int z_, int tileId_);

	int hashCode() const;

    bool operator==(const TickNextTickData& t) const;
    bool operator<(const TickNextTickData& tnd) const;

	TickNextTickData* setDelay(long l);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__TickNextTickData_H__*/
