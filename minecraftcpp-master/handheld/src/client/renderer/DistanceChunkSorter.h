#ifndef NET_MINECRAFT_CLIENT_RENDERER__DistanceChunkSorter_H__
#define NET_MINECRAFT_CLIENT_RENDERER__DistanceChunkSorter_H__

//package net.minecraft.client.renderer;

#include "../../world/entity/Entity.h"
#include "Chunk.h"

class DistanceChunkSorter
{
    Entity* player;

public:
    DistanceChunkSorter(Entity* player) {
        this->player = player;
    }

    bool operator() (const Chunk* c0, const Chunk* c1) {
		float d0 = c0->distanceToSqr(player);
		float d1 = c1->distanceToSqr(player);
		if (d0 > 32 * 32 && c0->y < 64) d0 *= 10.0f;
		if (d1 > 32 * 32 && c1->y < 64) d1 *= 10.0f;
        return d0 < d1;//? -1 : 1;
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__DistanceChunkSorter_H__*/
