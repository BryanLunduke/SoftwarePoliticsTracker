#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumData_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumData_H__

//package net.minecraft.client.renderer.culling;

#include "../../../world/phys/AABB.h"

// We create an enum of the sides so we don't have to call each side 0 or 1.
// This way it makes it more understandable and readable when dealing with frustum sides.
class FrustumData
{
public:
    //enum FrustumSide
    static const int RIGHT = 0; // The RIGHT side of the frustum
    static const int LEFT = 1; // The LEFT    side of the frustum
    static const int BOTTOM = 2; // The BOTTOM side of the frustum
    static const int TOP = 3; // The TOP side of the frustum
    static const int BACK = 4; // The BACK   side of the frustum
    static const int FRONT = 5; // The FRONT side of the frustum

    // Like above, instead of saying a number for the ABC and D of the plane, we
    // want to be more descriptive.
    static const int A = 0; // The X value of the plane's normal
    static const int B = 1; // The Y value of the plane's normal
    static const int C = 2; // The Z value of the plane's normal
    static const int D = 3; // The distance the plane is from the origin

    float m_Frustum[16][16];
    float proj[16];
    float modl[16];
    float clip[16];

	bool pointInFrustum(float x, float y, float z)
    {
        for (int i = 0; i < 6; i++)
        {
            if (m_Frustum[i][A] * x + m_Frustum[i][B] * y + m_Frustum[i][C] * z + m_Frustum[i][D] <= 0)
            {
                return false;
            }
        }
    
        return true;
    }
    bool sphereInFrustum(float x, float y, float z, float radius)
    {
        for (int i = 0; i < 6; i++)
        {
            if (m_Frustum[i][A] * x + m_Frustum[i][B] * y + m_Frustum[i][C] * z + m_Frustum[i][D] <= -radius)
            {
                return false;
            }
        }
    
        return true;
    }
    bool cubeFullyInFrustum(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        for (int i = 0; i < 6; i++)
        {
            if (!(m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0)) return false;
            if (!(m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0)) return false;
            if (!(m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0)) return false;
            if (!(m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0)) return false;
            if (!(m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0)) return false;
            if (!(m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0)) return false;
            if (!(m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0)) return false;
            if (!(m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0)) return false;
        }
    
        return true;
    }
    
    bool cubeInFrustum(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        for (int i = 0; i < 6; i++)
        {
            if (m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0) continue;
            if (m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0) continue;
            if (m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0) continue;
            if (m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z1) + m_Frustum[i][D] > 0) continue;
            if (m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0) continue;
            if (m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y1) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0) continue;
            if (m_Frustum[i][A] * (x1) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0) continue;
            if (m_Frustum[i][A] * (x2) + m_Frustum[i][B] * (y2) + m_Frustum[i][C] * (z2) + m_Frustum[i][D] > 0) continue;
    
            return false;
        }
    
        return true;
    }
    bool isVisible(const AABB& aabb)
    {
        return cubeInFrustum(aabb.x0, aabb.y0, aabb.z0, aabb.x1, aabb.y1, aabb.z1);
    }
};
#endif /*NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumData_H__*/
