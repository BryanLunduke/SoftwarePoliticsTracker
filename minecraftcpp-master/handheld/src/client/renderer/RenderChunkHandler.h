#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderChunkHandler_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderChunkHandler_H__

#include <vector>
#include "RenderChunk.h"

typedef std::vector<RenderChunk> ChunkList;

class RenderChunkHandler
{
public:
    int vboCount;
    ChunkList chunks;

    RenderChunkHandler() {
        vboCount = Tesselator::getVboCount();
    }

    void render() {
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__RenderChunkHandler_H__*/
