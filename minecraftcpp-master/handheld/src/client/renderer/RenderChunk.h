#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderChunk_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderChunk_H__

//package net.minecraft.client.renderer;

#include "gles.h"
#include "../../world/phys/Vec3.h"

class RenderChunk
{
public:
	RenderChunk();
	RenderChunk(GLuint vboId_, int vertexCount_);

	GLuint vboId;
	GLsizei vertexCount;
	int id;
	Vec3 pos;

private:
	static int runningId;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__RenderChunk_H__*/
