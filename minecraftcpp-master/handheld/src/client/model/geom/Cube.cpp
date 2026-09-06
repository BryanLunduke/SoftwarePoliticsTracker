#include "Cube.h"
#include "ModelPart.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/gles.h"
#include "../../../util/Mth.h"

Cube::Cube(ModelPart* modelPart, int xTexOffs, int yTexOffs, float x0, float y0, float z0, int w, int h, int d, float g)
:   x0(x0),
    y0(y0),
    z0(z0),
    x1(x0 + w),
    y1(y0 + h),
    z1(z0 + d)
{
    float x1 = this->x1;
    float y1 = this->y1;
    float z1 = this->z1;

    x0 -= g;
    y0 -= g;
    z0 -= g;
    x1 += g;
    y1 += g;
    z1 += g;

    if (modelPart->mirror) {
        float tmp = x1;
        x1 = x0;
        x0 = tmp;
    }

	vertices[0] = VertexPT(x0, y0, z0, 0, 0);
	vertices[1] = VertexPT(x1, y0, z0, 0, 8);
	vertices[2] = VertexPT(x1, y1, z0, 8, 8);
	vertices[3] = VertexPT(x0, y1, z0, 8, 0);

	vertices[4] = VertexPT(x0, y0, z1, 0, 0);
	vertices[5] = VertexPT(x1, y0, z1, 0, 8);
	vertices[6] = VertexPT(x1, y1, z1, 8, 8);
	vertices[7] = VertexPT(x0, y1, z1, 8, 0);

	VertexPT* ptr = vertices - 1;
	VertexPT* u0 = ++ptr;
	VertexPT* u1 = ++ptr;
	VertexPT* u2 = ++ptr;
	VertexPT* u3 = ++ptr;

	VertexPT* l0 = ++ptr;
	VertexPT* l1 = ++ptr;
	VertexPT* l2 = ++ptr;
	VertexPT* l3 = ++ptr;

	polygons[0] = PolygonQuad(l1, u1, u2, l2, xTexOffs + d + w, yTexOffs + d, xTexOffs + d + w + d, yTexOffs + d + h); // Right
	polygons[1] = PolygonQuad(u0, l0, l3, u3, xTexOffs + 0, yTexOffs + d, xTexOffs + d, yTexOffs + d + h); // Left
	polygons[2] = PolygonQuad(l1, l0, u0, u1, xTexOffs + d, yTexOffs + 0, xTexOffs + d + w, yTexOffs + d); // Up
	polygons[3] = PolygonQuad(u2, u3, l3, l2, xTexOffs + d + w, yTexOffs + d, xTexOffs + d + w + w, yTexOffs); // Down
	polygons[4] = PolygonQuad(u1, u0, u3, u2, xTexOffs + d, yTexOffs + d, xTexOffs + d + w, yTexOffs + d + h); // Front
	polygons[5] = PolygonQuad(l0, l1, l2, l3, xTexOffs + d + w + d, yTexOffs + d, xTexOffs + d + w + d + w, yTexOffs + d + h); // Back

    if (modelPart->mirror) {
        for (int i = 0; i < 6; i++)
            polygons[i].mirror();
    }
}

void Cube::compile(Tesselator& t, float scale) {
    for (int i = 0; i < 6; i++) {
        polygons[i].render(t, scale);
    }
}

Cube* Cube::setId(const std::string& id) {
    this->id = id;
    return this;
}