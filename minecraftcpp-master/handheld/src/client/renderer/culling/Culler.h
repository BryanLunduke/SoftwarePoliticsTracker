#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__Culler_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__Culler_H__

//package net.minecraft.client.renderer.culling;

class AABB;

class Culler
{
public:
    virtual ~Culler() {}

	virtual bool isVisible(const AABB& bb) = 0;

	virtual bool cubeInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) = 0;

	virtual bool cubeFullyInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) = 0;

	virtual void prepare(float xOff, float yOff, float zOff) {}
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_CULLING__Culler_H__*/
