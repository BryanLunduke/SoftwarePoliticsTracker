#ifndef NET_MINECRAFT_CLIENT_RENDERER__DirtyChunkSorter_H__
#define NET_MINECRAFT_CLIENT_RENDERER__DirtyChunkSorter_H__

//package net.minecraft.client.renderer;

#include "../../world/entity/Entity.h"
#include "Chunk.h"

class DirtyChunkSorter
{
    Entity* cameraEntity;

public:
    DirtyChunkSorter(Entity* cameraEntity) {
        this->cameraEntity = cameraEntity;
    }

	// @attn: c0 > c1
    bool operator() (Chunk* c0, Chunk* c1) {
        bool i0 = c0->visible;
        bool i1 = c1->visible;
        if (i0 && !i1) return false; //  1;
        if (i1 && !i0) return true;  // -1;

        float d0 = c0->distanceToSqr(cameraEntity);
        float d1 = c1->distanceToSqr(cameraEntity);

        if (d0 < d1) return false;  //  1;
        if (d0 > d1) return true;   // -1;

        return c0->id > c1->id;// ? -1 : 1;
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__DirtyChunkSorter_H__*/
