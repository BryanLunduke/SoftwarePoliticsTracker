#ifndef NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__Node_H__
#define NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__Node_H__

//package net.minecraft.world.level.pathfinder;

#include "../../../util/Mth.h"
#include <string>

class Node
{
public:
    Node(int x = 0, int y = 0, int z = 0)
	:	x(x),
		y(y),
		z(z),
		heapIdx(-1),
		g(0), h(0), f(0),
		cameFrom(NULL),
		closed(false),
		hash(createHash(x,y,z))
	{
    }

    static int createHash(const int x, const int y, const int z) {
        return (y & 0xff) | ((x & 0x7fff) << 8) | ((z & 0x7fff) << 24) | ((x < 0) ? 0x0080000000 : 0) | ((z < 0) ? 0x0000008000 : 0);
    }

    float distanceTo(Node* to) const {
        float xd = (float)(to->x - x);
        float yd = (float)(to->y - y);
        float zd = (float)(to->z - z);
        return Mth::sqrt(xd * xd + yd * yd + zd * zd);
    }

	bool operator==(const Node& rhs) const {
		return hash == rhs.hash && x == rhs.x && y == rhs.y && z == rhs.z;
    }

    int hashCode() const {
        return hash;
    }

    bool inOpenSet() const {
		return heapIdx >= 0;
    }

    std::string toString() const {
        return "Node::toString not implemented";//x + ", " + y + ", " + z;
    }

public:
    int heapIdx;
    float g, h, f;
    Node* cameFrom;

	short x, y, z;
	bool closed;
private:
	int hash;
};

class TNode {
public:
	TNode(Node* node)
	:	node(node)
	{}

	bool operator==(const TNode& rhs) const {
		return node->operator==(*rhs.node);
	}
	bool operator<(const TNode& rhs) const {
		if (node->z != rhs.node->z) return node->z < rhs.node->z;
		if (node->x != rhs.node->x) return node->x < rhs.node->x;
		return node->y < rhs.node->y;
	}
	Node* node;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__Node_H__*/
