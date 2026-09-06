#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__

//package net.minecraft.client.renderer.culling;

#include "FrustumData.h"
#include "Frustum.h"

class FrustumCuller: public Culler {
    
private:
    FrustumData frustum;
    float xOff, yOff, zOff;

public:
    FrustumCuller() {
        frustum = Frustum::getFrustum();
    }

	void prepare(float xOff, float yOff, float zOff) {
        this->xOff = xOff;
        this->yOff = yOff;
        this->zOff = zOff;
    }

    bool cubeFullyInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeFullyInFrustum(x0 - xOff, y0 - yOff, z0 - zOff, x1 - xOff, y1 - yOff, z1 - zOff);
    }

    bool cubeInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeInFrustum(x0 - xOff, y0 - yOff, z0 - zOff, x1 - xOff, y1 - yOff, z1 - zOff);
    }

    bool isVisible(const AABB& bb) {
        return cubeInFrustum(bb.x0, bb.y0, bb.z0, bb.x1, bb.y1, bb.z1);
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__*/
