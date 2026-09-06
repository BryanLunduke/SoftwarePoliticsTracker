#ifndef NET_MINECRAFT_CLIENT_MODEL_GEOM__Cube_H__
#define NET_MINECRAFT_CLIENT_MODEL_GEOM__Cube_H__

//package net.minecraft.client.model;

#include <string>
#include "Polygon.h"

class Tesselator;
class ModelPart;

class Cube
{
public:
	Cube(ModelPart* modelPart, int xTexOffs, int yTexOffs, float x0, float y0, float z0, int w, int h, int d, float g);

	void compile(Tesselator& t, float scale);
	Cube* setId(const std::string& id);
private:

	VertexPT vertices[8];
	PolygonQuad polygons[6];
	std::string id;
	const float x0, y0, z0, x1, y1, z1;

	friend class ModelPart;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL_GEOM__Cube_H__*/
