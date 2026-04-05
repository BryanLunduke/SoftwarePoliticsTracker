#ifndef NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__Path_H__
#define NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__Path_H__

//package net.minecraft.world.level.pathfinder;

#include "Node.h"
#include "../../phys/Vec3.h"
#include "../../entity/Entity.h"

class Path
{
public:
	Path();
	~Path();

	void  copyNodes(Node** nodes, int length);

	void  destroy();

    void  next();

	void  setSize(int size);
	int   getSize() const;

	bool  isEmpty() const;
    bool  isDone() const;

    Node* last() const;
    Node* get(int i) const;

    int   getIndex() const;
    void  setIndex(int index);

    Vec3  currentPos(Entity* e) const;
	Node* currentPos();

    Vec3  getPos(Entity* e, int index) const;

    bool  sameAs(const Path* path) const;

    bool  endsIn(const Vec3& pos) const;
    bool  endsInXZ(const Vec3& pos) const;

	int id;
private:
	Node** nodes;
	int length;
    int index;

	static int p;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_PATHFINDER__Path_H__*/
