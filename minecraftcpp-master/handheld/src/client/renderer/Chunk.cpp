#include "Chunk.h"
#include "Tesselator.h"
#include "TileRenderer.h"
#include "culling/Culler.h"
#include "../../world/entity/Entity.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/level/Region.h"
#include "../../world/level/chunk/LevelChunk.h"
#include "../../util/Mth.h"
//#include "../../platform/time.h"

/*static*/ int Chunk::updates = 0;
//static Stopwatch swRebuild;
//int* _layerChunks[3] = {0, 0, 0}; //Chunk::NumLayers];
//int _layerChunkCount[3] = {0, 0, 0};

Chunk::Chunk( Level* level_, int x, int y, int z, int size, int lists_, GLuint* ptrBuf/*= NULL*/)
:	level(level_),
	visible(false),
	compiled(false),
    _empty(true),
	xs(size), ys(size), zs(size),
	dirty(false),
	occlusion_visible(true),
	occlusion_querying(false),
	lists(lists_),
	vboBuffers(ptrBuf),
	bb(0,0,0,1,1,1),
	t(Tesselator::instance)
{
	for (int l = 0; l < NumLayers; l++) {
		empty[l] = false;
	}

	radius = Mth::sqrt((float)(xs * xs + ys * ys + zs * zs)) * 0.5f;

	this->x = -999;
	setPos(x, y, z);
}

void Chunk::setPos( int x, int y, int z )
{
	if (x == this->x && y == this->y && z == this->z) return;

	reset();
	this->x = x;
	this->y = y;
	this->z = z;
	xm = x + xs / 2;
	ym = y + ys / 2;
	zm = z + zs / 2;

	const float xzg = 1.0f;
	const float yp = 2.0f;
	const float yn = 0.0f;
	bb.set(x-xzg, y-yn, z-xzg, x + xs+xzg, y + ys+yp, z + zs+xzg);

	//glNewList(lists + 2, GL_COMPILE);
	//ItemRenderer.renderFlat(AABB.newTemp(xRenderOffs - g, yRenderOffs - g, zRenderOffs - g, xRenderOffs + xs + g, yRenderOffs + ys + g, zRenderOffs + zs + g));
	//glEndList();
	setDirty();
}

void Chunk::translateToPos()
{
	glTranslatef2((float)x, (float)y, (float)z);
}

void Chunk::rebuild()
{
	if (!dirty) return;
	//if (!visible) return;
	updates++;

    //if (!_layerChunks[0]) {
    //    for (int i = 0; i < NumLayers; ++i)
    //        _layerChunks[i] = new int[xs * ys * zs];
    //}
    //for (int i = 0; i < NumLayers; ++i)
    //    _layerChunkCount[i] = 0;
    
    //Stopwatch& sw = swRebuild;
    //sw.start();
    
	int x0 = x;
	int y0 = y;
	int z0 = z;
	int x1 = x + xs;
	int y1 = y + ys;
	int z1 = z + zs;
	for (int l = 0; l < NumLayers; l++) {
		empty[l] = true;
	}
    _empty = true;

	LevelChunk::touchedSky = false;

	int r = 1;
	Region region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r);
	TileRenderer tileRenderer(&region);

    bool doRenderLayer[NumLayers] = {true, false, false};
	for (int l = 0; l < NumLayers; l++) {
        if (!doRenderLayer[l]) continue;
		bool renderNextLayer = false;
		bool rendered = false;

		bool started = false;
        int cindex = -1;

		for (int y = y0; y < y1; y++) {
			for (int z = z0; z < z1; z++) {
				for (int x = x0; x < x1; x++) {
                    ++cindex;
                    //if (l > 0 && cindex != _layerChunks[_layerChunkCount[l]])
					int tileId = region.getTile(x, y, z);
					if (tileId > 0) {
						if (!started) {
							started = true;

#ifndef USE_VBO
							glNewList(lists + l, GL_COMPILE);
							glPushMatrix2();
							translateToPos();
							float ss = 1.000001f;
							glTranslatef2(-zs / 2.0f, -ys / 2.0f, -zs / 2.0f);
							glScalef2(ss, ss, ss);
							glTranslatef2(zs / 2.0f, ys / 2.0f, zs / 2.0f);
#endif
							t.begin();
							//printf(".");
							//printf("Tesselator::offset : %d, %d, %d\n", this->x, this->y, this->z);
							t.offset((float)(-this->x), (float)(-this->y), (float)(-this->z));
							//printf("Tesselator::offset : %f, %f, %f\n", this->x, this->y, this->z);
						}

						Tile* tile = Tile::tiles[tileId];
						int renderLayer = tile->getRenderLayer();

//                        if (renderLayer == l)
//                            rendered |= tileRenderer.tesselateInWorld(tile, x, y, z);
//                        else {
//                            _layerChunks[_layerChunkCount[renderLayer]++] = cindex;
//                        }
                        
						if (renderLayer > l) {
							renderNextLayer = true;
                            doRenderLayer[renderLayer] = true;
						} else if (renderLayer == l) {
							rendered |= tileRenderer.tesselateInWorld(tile, x, y, z);
						}
					}
				}
			}
		}

		if (started) {

#ifdef USE_VBO
			renderChunk[l] = t.end(true, vboBuffers[l]);
			renderChunk[l].pos.x = (float)this->x;
			renderChunk[l].pos.y = (float)this->y;
			renderChunk[l].pos.z = (float)this->z;
#else
			t.end(false, -1);
			glPopMatrix2();
			glEndList();
#endif
			t.offset(0, 0, 0);
		} else {
			rendered = false;
		}
		if (rendered) {
            empty[l] = false;
            _empty = false;
        }
		if (!renderNextLayer) break;
	}

    //sw.stop();
    //sw.printEvery(1, "rebuild-");
	skyLit = LevelChunk::touchedSky;
	compiled = true;
	return;
}

float Chunk::distanceToSqr( const Entity* player ) const
{
	float xd = (float) (player->x - xm);
	float yd = (float) (player->y - ym);
	float zd = (float) (player->z - zm);
	return xd * xd + yd * yd + zd * zd;
}

float Chunk::squishedDistanceToSqr( const Entity* player ) const
{
	float xd = (float) (player->x - xm);
	float yd = (float) (player->y - ym) * 2;
	float zd = (float) (player->z - zm);
	return xd * xd + yd * yd + zd * zd;
}

void Chunk::reset()
{
	for (int i = 0; i < NumLayers; i++) {
		empty[i] = true;
	}
	visible = false;
	compiled = false;
    _empty = true;
}

int Chunk::getList( int layer )
{
	if (!visible) return -1;
	if (!empty[layer]) return lists + layer;
	return -1;
}

RenderChunk& Chunk::getRenderChunk( int layer )
{
	return renderChunk[layer];
}

int Chunk::getAllLists( int displayLists[], int p, int layer )
{
	if (!visible) return p;
	if (!empty[layer]) displayLists[p++] = (lists + layer);
	return p;
}

void Chunk::cull( Culler* culler )
{
	visible = culler->isVisible(bb);
}

void Chunk::renderBB()
{
	//glCallList(lists + 2);
}

bool Chunk::isEmpty()
{
	return compiled && _empty;//empty[0] && empty[1] && empty[2];
//	if (!compiled) return false;
//	return empty[0] && empty[1];
}

void Chunk::setDirty()
{
	dirty = true;
}

void Chunk::setClean()
{
	dirty = false;
}

bool Chunk::isDirty()
{
	return dirty;
}

void Chunk::resetUpdates()
{
	updates = 0;
	//swRebuild.reset();
}
