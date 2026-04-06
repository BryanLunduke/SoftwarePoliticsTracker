#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__AllowAllCuller_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__AllowAllCuller_H__

//package net.minecraft.client.renderer.culling;

#include "Culler.h"

class AABB;

class AllowAllCuller: public Culler
{
public:
    bool isVisible(const AABB& bb) {
        return true;
    }

    bool cubeFullyInFrustum(float x1, float y1, float z1, float x2, float y2, float z2) {
        return true;
    }

    bool cubeInFrustum(float x1, float y1, float z1, float x2, float y2, float z2) {
        return true;
    }

    void prepare(float xOff, float yOff, float zOff) {
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_CULLING__AllowAllCuller_H__*/
