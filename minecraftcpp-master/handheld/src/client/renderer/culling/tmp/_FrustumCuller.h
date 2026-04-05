#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__

//package net.minecraft.client.renderer.culling;

#include "world/phys/AABB.h"

/*public*/ class FrustumCuller: /*implements-interface*/ public Culler {
    
    /*private*/ FrustumData frustum;

    /*public*/ FrustumCuller() {
        frustum = Frustum.getFrustum();
    }

    /*private*/ float xOff, yOff, zOff;

    /*public*/ void prepare(float xOff, float yOff, float zOff) {
        this->xOff = xOff;
        this->yOff = yOff;
        this->zOff = zOff;
    }

    /*public*/ bool cubeFullyInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeFullyInFrustum(x0 - xOff, y0 - yOff, z0 - zOff, x1 - xOff, y1 - yOff, z1 - zOff);
    }

    /*public*/ bool cubeInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeInFrustum(x0 - xOff, y0 - yOff, z0 - zOff, x1 - xOff, y1 - yOff, z1 - zOff);
    }

    /*public*/ bool isVisible(AABB bb) {
        return cubeInFrustum(bb.x0, bb.y0, bb.z0, bb.x1, bb.y1, bb.z1);
    }
}

#endif /*NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__*/
