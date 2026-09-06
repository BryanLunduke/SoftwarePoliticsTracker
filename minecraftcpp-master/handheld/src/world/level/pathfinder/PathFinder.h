#ifndef NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__PathFinder_H__
#define NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__PathFinder_H__

//package net.minecraft.world.level.pathfinder;

#include "../LevelSource.h"
#include "../material/Material.h"
#include "../tile/DoorTile.h"
#include "../../entity/Entity.h"
#include "../../../util/Mth.h"
#include <map>

#include "BinaryHeap.h"
#include "Node.h"
#include "Path.h"

static int __created;
static int __maxCreated = 0;
static const int MAX_NODES = 2048;

class FreeCache {
public:
	FreeCache()
	:	cache(w * w * h)
	{}
	void setCenterPos(int x, int y, int z) {
		bx = x - w/2;
		by = y - h/2;
		bz = z - w/2;
	}
	int getValue(int x, int y, int z) {
		return cache.get(_index(x, y, z));
	}
	void setValue(int x, int y, int z, int value) {
		cache.set(_index(x, y, z), value);
	}
	void clear() {
		cache.setAll(0);
	}
	__inline int _index(int x, int y, int z) {
		return (x-bz) | ((z-bz) << ShiftZ) | ((y-by) << ShiftY);
	}
private:
	int bx, by, bz;
	DataLayer cache;

	static const int w = 64, h = 32;
	static const int ShiftY = 10, ShiftZ = 5;
};

class PathFinder
{
	typedef std::map<int, TNode> NodeMap;
public:
	PathFinder()
	:	level(NULL),
		canOpenDoors(false),
		avoidWater(false)
	{
	}
    PathFinder(LevelSource* level)
	:	canOpenDoors(false),
		avoidWater(false)
	{
        setLevelSource(level);
    }

	void setLevelSource(LevelSource* level) {
		this->level = level;
	}

    bool findPath(Path* path, Entity* from, Entity* to, float maxDist) {
        return findPath(*path, from, to->x, to->bb.y0, to->z, maxDist);
    }

    bool findPath(Path* path, Entity* from, int x, int y, int z, float maxDist) {
        return findPath(*path, from, x + 0.5f, y + 0.5f, z + 0.5f, maxDist);
    }

private:
    bool findPath(Path& path, Entity* e, float xt, float yt, float zt, float maxDist) {
		//openSet.clear();
		//LOGI("<--------------------->\n");
		static Stopwatch w;
		w.start();

		// @attn @fix: this is danger!
		nodes.clear();
		_nodeIndex = 0;

		// Calculate the From node
		bool resetAvoidWater = avoidWater;
		int startY;
		if (e->isInWater()) {
			startY = (int) (e->bb.y0);
			int tileId = level->getTile(Mth::floor(e->x), startY, Mth::floor(e->z));
			while (tileId == Tile::water->id || tileId == Tile::calmWater->id) {
				++startY;
				tileId = level->getTile(Mth::floor(e->x), startY, Mth::floor(e->z));
			}
			resetAvoidWater = avoidWater;
			avoidWater = false;
		} else startY = Mth::floor(e->bb.y0 + 0.5f);

		Node* from = getNode(Mth::floor(e->bb.x0), startY, Mth::floor(e->bb.z0));

		// Try finding a To node that doesn't have air below
		const int xx0 = Mth::floor(xt - e->bbWidth / 2);
		const int yy0 = Mth::floor(yt);
		const int zz0 = Mth::floor(zt - e->bbWidth / 2);

		Node* to = NULL;
		if (level->getTile(xx0, yy0-1, zz0)) {
			to = getNode(xx0, yy0, zz0);
		} else {
			const int xx1 = Mth::floor(xt + e->bbWidth /2);
			const int zz1 = Mth::floor(zt + e->bbWidth /2);
			for (int xx = xx0; xx <= xx1; ++xx)
			for (int zz = zz0; zz <= zz1; ++zz) {
				if (level->getTile(xx, yy0-1, zz) != 0) {
					to = getNode(xx, yy0, zz);
					break;
				}
			}
			if (!to) { // Find the first non-air tile below
				int yy = yy0;
				while(!level->getTile(xx0, yy-1, zz0) && yy > 0)
					--yy;
				to = getNode(xx0, yy, zz0);
			}
		}

		Node size(Mth::floor((e->bbWidth + 1)), Mth::floor((e->bbHeight + 1)), Mth::floor((e->bbWidth + 1)));
        bool out = findPath(path, e, from, to, &size, maxDist);
		w.stop();
		//w.printEvery(1, "Pathfinder");

		// Clear excessive Nodes that was created this round
		if (_nodeIndex >= MAX_NODES) {
			for (unsigned int i = 0; i < _pending.size(); ++i)
				delete _pending[i];
			_pending.clear();
		}

		return out;
    }

    // function A*(start,goal)
    bool findPath(Path& path, Entity* e, Node* from, Node* to, const Node* size, float maxDist) {
		//static int _x = 0;
		__created = 0;

        from->g = 0;
        from->h = from->distanceTo(to);
        from->f = from->h;
		
        openSet.clear();
        openSet.insert(from);

        Node* closest = from;

        while (!openSet.isEmpty()) {
			//LOGI("size1: %d\n", openSet.size());
            Node* x = openSet.pop();
			//LOGI("size2: %d\n", openSet.size());

			//if (x->x == to->x && x->y == to->y && x->z == to->z) {
            if (*x == *to) {
				//LOGI(">>> %p, %p : %d, %d\n", x, to, x->hashCode(), to->hashCode());
				if (__created > __maxCreated) {
					__maxCreated = __created;
					for (int i = 0; i < 1; ++i) LOGI("\tNEW MAX: Created %d nodes\n", __created);
				}
				reconstruct_path(path, from, to); //@fix?
                return true;
            }

            if (x->distanceTo(to) < closest->distanceTo(to)) {
				//LOGI("closer!\n");
                closest = x;
            }
            x->closed = true;

            int neighborCount = getNeighbors(e, x, size, to, maxDist);
            for (int i = 0; i < neighborCount; i++) {
                Node* y = neighbors[i];
				if (y->closed) continue;

                float tentative_g_score = x->g + x->distanceTo(y);
                if (!y->inOpenSet() || tentative_g_score < y->g) {
				//if (!openSet.has(y) || tentative_g_score < y->g) {
                    y->cameFrom = x;
                    y->g = tentative_g_score;
                    y->h = y->distanceTo(to);
                    if (y->inOpenSet()) {
					//if (openSet.has(y)) {
                        openSet.changeCost(y, y->g + y->h);
						//delete y;
                    } else {
                        y->f = y->g + y->h;
                        openSet.insert(y);
                    }
                } //else delete y;
				
				//bool isBetter = false;
				//float tentative_g_score = x->g + x->distanceTo(y);
				//if (!y->inOpenSet()) {
				//	openSet.insert(y);
				//	y->h = y->distanceTo(to);
				//	isBetter = true;
				//} else if (tentative_g_score < y->g) {
				//	isBetter = true;
				//}
				//if (isBetter) {
				//	//y->f = y->g + y->h;
				//	y->cameFrom = x;
				//	y->g = tentative_g_score;
				//	openSet.changeCost(y, y->g + y->h);
				//}
            }
        }

		if (__created > __maxCreated) {
			__maxCreated = __created;
			for (int i = 0; i < 1; ++i) LOGI("\tNEW MAX: Created %d nodes\n", __created);
		}

        if (closest == from)
			return false;
		reconstruct_path(path, from, closest); //@fix?
		return true;
    }

    int getNeighbors(Entity* entity, Node* pos, const Node* size, Node* target, float maxDist) {
        int p = 0;

		//LOGI("Getting neighbours for: (%d, %d, %d)\n", pos->x, pos->y, pos->z);

        int jumpSize = 0;
        if (isFree(entity, pos->x, pos->y + 1, pos->z, size) == TYPE_OPEN) jumpSize = 1;

        Node* n = getNode(entity, pos->x, pos->y, pos->z + 1, size, jumpSize);
        Node* w = getNode(entity, pos->x - 1, pos->y, pos->z, size, jumpSize);
        Node* e = getNode(entity, pos->x + 1, pos->y, pos->z, size, jumpSize);
        Node* s = getNode(entity, pos->x, pos->y, pos->z - 1, size, jumpSize);

        if (n != NULL && !n->closed && n->distanceTo(target) < maxDist) neighbors[p++] = n;
        if (w != NULL && !w->closed && w->distanceTo(target) < maxDist) neighbors[p++] = w;
        if (e != NULL && !e->closed && e->distanceTo(target) < maxDist) neighbors[p++] = e;
        if (s != NULL && !s->closed && s->distanceTo(target) < maxDist) neighbors[p++] = s;
        return p;
    }

    Node* getNode(Entity* entity, int x, int y, int z, const Node* size, int jumpSize) {
        Node* best = NULL;
		int pathType = isFree(entity, x, y, z, size);
		if (pathType == TYPE_WALKABLE) return getNode(x, y, z);
        if (pathType == TYPE_OPEN) best = getNode(x, y, z);
		if (best == NULL && jumpSize > 0 && pathType != TYPE_FENCE && isFree(entity, x, y + jumpSize, z, size) == TYPE_OPEN) {
			best = getNode(x, y + jumpSize, z);
            y += jumpSize;
        }

        if (best != NULL) {
            int drop = 0;
            int cost = 0;
			while (y > 0) {
				cost = isFree(entity, x, y - 1, z, size);
				if (avoidWater && cost == TYPE_WATER) return NULL;
				if (cost != TYPE_OPEN) break;

                // fell too far?
                if (++drop >= 4) return NULL;
                if (--y > 0) best = getNode(x, y, z);
            }
            // fell into lava?
            if (cost == TYPE_LAVA) return NULL;
        }

        return best;
    }

    Node* getNode(int x, int y, int z) {
        int i = Node::createHash(x, y, z);
		NodeMap::iterator it = nodes.find(i);

        if (it == nodes.end()){
            Node* node = new_Node(x, y, z);
			++__created;
            nodes.insert(std::make_pair(i, TNode(node)));
			return node;
        }
        return it->second.node;
    }

	static const int TYPE_FENCE    = 1;
    static const int TYPE_LAVA     = 2;
    static const int TYPE_WATER    = 3;
    static const int TYPE_BLOCKED  = 4;
    static const int TYPE_OPEN     = 5;
	static const int TYPE_WALKABLE = 6;

    int isFree(Entity* entity, int x, int y, int z, const Node* size) {
		bool walkable = false;
		//LOGI("isfree: [%d, %d, %d]\n", x, y, z);
        for (int xx = x; xx < x + size->x; xx++) {
            for (int yy = y; yy < y + size->y; yy++) {
                for (int zz = z; zz < z + size->z; zz++) {
                    int tileId = level->getTile(xx, yy, zz);
					if (tileId <= 0) continue;
                    if (tileId == Tile::door_iron->id || tileId == Tile::door_wood->id) {
						//LOGI("canOpenDoors? %d : %d\n", canOpenDoors, DoorTile::isOpen(level->getData(xx, yy, zz)));
						if (tileId == Tile::door_wood->id && canOpenDoors)
							continue;
                        int data = level->getData(xx, yy, zz);
						if (!DoorTile::isOpen(entity->level, xx, yy, zz)) {
							return TYPE_BLOCKED;
						}
						else
							continue;
                    }
					else if (tileId == Tile::water->id || tileId == Tile::calmWater->id) {
						if (avoidWater) {
							return TYPE_WATER;
						}
						walkable = true;
					}
					else if (tileId == Tile::fence->id || tileId == Tile::fenceGate->id) {
						return TYPE_FENCE;
					}

					const Material* m = Tile::tiles[tileId]->material;
                    if (m->blocksMotion()) {
						return TYPE_BLOCKED;
					} else walkable = true;
                    if (m == Material::lava) {
                        return TYPE_LAVA;
                    }
                }
            }
        }
        return TYPE_OPEN;
    }

    // function reconstruct_path(came_from,current_node)
    void reconstruct_path(Path& path, Node* from, Node* to) {
        int count = 1;
        Node* n = to;
        while (n->cameFrom != NULL) {
            count++;
            n = n->cameFrom;
        }

		int size = count;
        Node** nodes = new Node*[size]; //@todo: have one long static array for this
        n = to;
        nodes[--count] = n;
        while (n->cameFrom != NULL) {
            n = n->cameFrom;
            nodes[--count] = n;
        }
		//LOGI("Setting %p nodes to path %p\n", nodes, &path);
		path.copyNodes(nodes, size);
		delete[] nodes;
    }

	Node* new_Node(int x, int y, int z) {
		//return new Node(x, y, z);
		if (++_nodeIndex >= MAX_NODES) {
			Node* out = new Node(x, y, z);
			_pending.push_back( out );
			return out;
		}
		Node& n = _nodes[_nodeIndex-1];
		n = Node(x, y, z);
		return &n;
	}

    LevelSource* level;

    BinaryHeap	openSet;
	NodeMap		nodes;

	Node _nodes[MAX_NODES];
	std::vector<Node*> _pending;
	int _nodeIndex;

    Node* neighbors[32];
public:
	bool canOpenDoors;
	bool avoidWater;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__PathFinder_H__*/
