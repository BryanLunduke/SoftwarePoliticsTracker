#ifndef NET_MINECRAFT_CLIENT_MODEL__Polygon_H__
#define NET_MINECRAFT_CLIENT_MODEL__Polygon_H__

//package net.minecraft.client.model;
#include "Vertex.h"

class Tesselator;

class PolygonQuad
{
public:
	PolygonQuad() {}
    PolygonQuad(VertexPT*,VertexPT*,VertexPT*,VertexPT*);
    PolygonQuad(VertexPT*,VertexPT*,VertexPT*,VertexPT*, int u0, int v0, int u1, int v1);
    PolygonQuad(VertexPT*,VertexPT*,VertexPT*,VertexPT*, float u0, float v0, float u1, float v1);

	void mirror();
	void render(Tesselator& t, float scale, int vboId = -1);
	PolygonQuad* flipNormal();

	VertexPT vertices[4];
	//int vertexCount;

private:
	static const int VERTEX_COUNT = 4;
	bool _flipNormal;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__Polygon_H__*/
