#ifndef NET_MINECRAFT_CLIENT_MODEL__Vertex_H__
#define NET_MINECRAFT_CLIENT_MODEL__Vertex_H__

//package net.minecraft.client.model;

#include "../../../world/phys/Vec3.h"

class VertexPT
{
public:
    Vec3 pos;

    float u, v;

	VertexPT() {}

    VertexPT(float x, float y, float z, float u_, float v_)
		:	pos(x, y, z),
			u(u_),
			v(v_)
	{}

    VertexPT remap(float u, float v) {
        return VertexPT(*this, u, v);
    }

    VertexPT(const VertexPT& vertex, float u_, float v_) {
        pos = vertex.pos;
        u = u_;
        v = v_;
    }

    VertexPT(const Vec3& pos_, float u_, float v_) {
        pos = pos_;
        u = u_;
        v = v_;
    }
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__Vertex_H__*/
