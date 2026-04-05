#ifndef NET_MINECRAFT_WORLD_LEVEL__TilePos_H__
#define NET_MINECRAFT_WORLD_LEVEL__TilePos_H__

//package net.minecraft.world.level;

class TilePos
{
public:
    int x, y, z;

    TilePos(int x_, int y_, int z_)
	:	x(x_),
		y(y_),
		z(z_)
	{
    }

	TilePos(const TilePos& rhs)
	:	x(rhs.x),
		y(rhs.y),
		z(rhs.z)
	{}

	bool operator<(const TilePos& rhs) const {
		return hashCode() < rhs.hashCode();
	}

	TilePos& operator=(const TilePos& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}

	bool operator==(const TilePos& rhs) const {
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}

    int hashCode() const {
        return x * 8976890 + y * 981131 + z;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__TilePos_H__*/
