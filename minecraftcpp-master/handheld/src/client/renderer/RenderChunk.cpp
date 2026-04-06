#include "RenderChunk.h"

int RenderChunk::runningId = 0;

RenderChunk::RenderChunk() :
	vboId(-1),
	vertexCount(0)
{
	id = ++runningId;
}

RenderChunk::RenderChunk( GLuint vboId_, int vertexCount_ )
:	vboId(vboId_),
	vertexCount(vertexCount_)
{
	id = ++runningId;
}
