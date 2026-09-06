#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__DungeonFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__DungeonFeature_H__

#if 0

//package net.minecraft.world.level.levelgen;

#include "LargeFeature.h"

class DungeonFeature: public LargeFeature {

    /*protected*/
	void addRoom(int xOffs, int zOffs, unsigned char* blocks, float xRoom, float yRoom, float zRoom);

    /*protected*/
	void addTunnel(int xOffs, int zOffs, unsigned char* blocks, float xCave, float yCave, float zCave, float thickness, float yRot, float xRot, int step, int dist, float yScale);

    /*protected*/
	void addFeature(Level level, int x, int z, int xOffs, int zOffs, unsigned char* blocks);
};

#endif
#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__DungeonFeature_H__*/
