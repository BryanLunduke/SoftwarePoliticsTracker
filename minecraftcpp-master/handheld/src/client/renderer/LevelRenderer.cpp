#include "LevelRenderer.h"

#include "DirtyChunkSorter.h"
#include "DistanceChunkSorter.h"
#include "Chunk.h"
#include "TileRenderer.h"
#include "../Minecraft.h"
#include "../../util/Mth.h"
#include "../../world/entity/player/Player.h"
#include "../../world/level/tile/LevelEvent.h"
#include "../../world/level/tile/LeafTile.h"
#include "../../client/particle/ParticleEngine.h"
#include "../../client/particle/ParticleInclude.h"
#include "../sound/SoundEngine.h"
#include "culling/Culler.h"
#include "entity/EntityRenderDispatcher.h"
#include "../model/HumanoidModel.h"

#include "GameRenderer.h"
#include "../../AppPlatform.h"
#include "../../util/PerfTimer.h"
#include "Textures.h"
#include "tileentity/TileEntityRenderDispatcher.h"
#include "../particle/BreakingItemParticle.h"

#include "../../client/player/LocalPlayer.h"

#ifdef GFX_SMALLER_CHUNKS
/* static */ const int LevelRenderer::CHUNK_SIZE = 8;
#else
/* static */ const int LevelRenderer::CHUNK_SIZE = 16;
#endif

LevelRenderer::LevelRenderer( Minecraft* mc)
:	mc(mc),
	textures(mc->textures),
	level(NULL),
	cullStep(0),

	chunkLists(0),
	xChunks(0), yChunks(0), zChunks(0),

	chunks(NULL),
	sortedChunks(NULL),

	xMinChunk(0), yMinChunk(0), zMinChunk(0),
	xMaxChunk(0), yMaxChunk(0), zMaxChunk(0),

	lastViewDistance(-1),

	noEntityRenderFrames(2),
	totalEntities(0),
	renderedEntities(0),
	culledEntities(0),

	occlusionCheck(false),
	totalChunks(0), offscreenChunks(0), renderedChunks(0), occludedChunks(0), emptyChunks(0),

	chunkFixOffs(0),
	xOld(-9999), yOld(-9999), zOld(-9999),

	ticks(0),
	skyList(0), starList(0), darkList(0),
	tileRenderer(NULL),
	destroyProgress(0)
{
#ifdef OPENGL_ES
	int maxChunksWidth = 2 * LEVEL_WIDTH / CHUNK_SIZE + 1;
	numListsOrBuffers = maxChunksWidth * maxChunksWidth * (128/CHUNK_SIZE) * 3;
	chunkBuffers = new GLuint[numListsOrBuffers];
	glGenBuffers2(numListsOrBuffers, chunkBuffers);
	LOGI("numBuffers: %d\n", numListsOrBuffers);
	//for (int i = 0; i < numListsOrBuffers; ++i) printf("bufId %d: %d\t", i, chunkBuffers[i]);

	glGenBuffers2(1, &skyBuffer);
	generateSky();
#else
	int maxChunksWidth = 1024 / CHUNK_SIZE;
	numListsOrBuffers = maxChunksWidth * maxChunksWidth * maxChunksWidth * 3;
	chunkLists = glGenLists(numListsOrBuffers);
#endif
}

LevelRenderer::~LevelRenderer()
{
	delete tileRenderer;
	tileRenderer = NULL;

	deleteChunks();

#ifdef OPENGL_ES
	glDeleteBuffers(numListsOrBuffers, chunkBuffers);
	glDeleteBuffers(1, &skyBuffer);
	delete[] chunkBuffers;
#else
	glDeleteLists(numListsOrBuffers, chunkLists);
#endif
}

void LevelRenderer::generateSky() {
	Tesselator& t = Tesselator::instance;
	float yy;
	int s = 128;
	int d = (256 / s) + 2;
	yy = (float) (16);
	t.begin();

	skyVertexCount = 0;
	for (int xx = -s * d; xx <= s * d; xx += s) {
		for (int zz = -s * d; zz <= s * d; zz += s) {
			t.vertex((float) xx + 0,  yy, (float) zz + 0);
			t.vertex((float)(xx + s), yy, (float) zz + 0);
			t.vertex((float)(xx + s), yy, (float)(zz + s));
			t.vertex((float) xx + 0,  yy, (float)(zz + s));
			//LOGI("x, z: %d, %d\n", xx, zz);
			skyVertexCount += 4;
		}
	}

	t.end(true, skyBuffer);
	//LOGI("skyvertexcount: %d\n", skyVertexCount);
	//glEndList();
}

void LevelRenderer::setLevel( Level* level )
{
	if (this->level != NULL) {
		this->level->removeListener(this);
	}

	xOld = -9999;
	yOld = -9999;
	zOld = -9999;

	EntityRenderDispatcher::getInstance()->setLevel(level);
	EntityRenderDispatcher::getInstance()->setMinecraft(mc);
	this->level = level;

	delete tileRenderer;
	tileRenderer = new TileRenderer(level);

	if (level != NULL) {
		level->addListener(this);
		allChanged();
	}
}

void LevelRenderer::allChanged()
{
	deleteChunks();

	Tile::leaves->setFancy(mc->options.fancyGraphics);
	Tile::leaves_carried->setFancy(mc->options.fancyGraphics);
	lastViewDistance = mc->options.viewDistance;

	int dist = (512 >> 3) << (3 - lastViewDistance);
	if (lastViewDistance <= 2 && mc->isPowerVR())
		dist = (int)((float)dist * 0.8f);
	LOGI("last: %d, power: %d\n", lastViewDistance, mc->isPowerVR());

	#if defined(RPI)
		dist *= 0.6f;
	#endif

	if (dist > 400) dist = 400;
	/*
	* if (Minecraft.FLYBY_MODE) { dist = 512 - CHUNK_SIZE * 2; }
	*/
	xChunks = (dist / LevelRenderer::CHUNK_SIZE) + 1;
	yChunks = (128 /  LevelRenderer::CHUNK_SIZE);
	zChunks = (dist / LevelRenderer::CHUNK_SIZE) + 1;
	chunksLength = xChunks * yChunks * zChunks;
	LOGI("chunksLength: %d. Distance: %d\n", chunksLength, dist);

	chunks = new Chunk*[chunksLength];
	sortedChunks = new Chunk*[chunksLength];

	int id = 0;
	int count = 0;

	xMinChunk = 0;
	yMinChunk = 0;
	zMinChunk = 0;
	xMaxChunk = xChunks;
	yMaxChunk = yChunks;
	zMaxChunk = zChunks;
	dirtyChunks.clear();
	//renderableTileEntities.clear();

	for (int x = 0; x < xChunks; x++) {
		for (int y = 0; y < yChunks; y++) {
			for (int z = 0; z < zChunks; z++) {
				const int c = getLinearCoord(x, y, z);
				Chunk* chunk = new Chunk(level, x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE, CHUNK_SIZE, chunkLists + id, &chunkBuffers[id]);

				if (occlusionCheck) {
					chunk->occlusion_id = 0;//occlusionCheckIds.get(count);
				}
				chunk->occlusion_querying = false;
				chunk->occlusion_visible = true;
				chunk->visible = true;
				chunk->id = count++;
				chunk->setDirty();

				chunks[c] = chunk;
				sortedChunks[c] = chunk;
				dirtyChunks.push_back(chunk);

				id += 3;
			}
		}
	}

	if (level != NULL) {
		Entity* player = mc->cameraTargetPlayer;
		if (player != NULL) {
			this->resortChunks(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
			DistanceChunkSorter distanceSorter(player);
			std::sort(sortedChunks, sortedChunks + chunksLength, distanceSorter);
		}
	}
	noEntityRenderFrames = 2;
}

void LevelRenderer::deleteChunks()
{
	for (int z = 0; z < zChunks; ++z)
	for (int y = 0; y < yChunks; ++y)
	for (int x = 0; x < xChunks; ++x) {
		int c = getLinearCoord(x, y, z);
		delete chunks[c];
	}

	delete[] chunks;
	chunks = NULL;

	delete[] sortedChunks;
	sortedChunks = NULL;
}

void LevelRenderer::resortChunks( int xc, int yc, int zc )
{
	xc -= CHUNK_SIZE / 2;
	//yc -= CHUNK_SIZE / 2;
	zc -= CHUNK_SIZE / 2;
	xMinChunk = INT_MAX;
	yMinChunk = INT_MAX;
	zMinChunk = INT_MAX;
	xMaxChunk = INT_MIN;
	yMaxChunk = INT_MIN;
	zMaxChunk = INT_MIN;

	int dirty = 0;

	int s2 = xChunks * CHUNK_SIZE;
	int s1 = s2 / 2;

	for (int x = 0; x < xChunks; x++) {
		int xx = x * CHUNK_SIZE;

		int xOff = (xx + s1 - xc);
		if (xOff < 0) xOff -= (s2 - 1);
		xOff /= s2;
		xx -= xOff * s2;

		if (xx < xMinChunk) xMinChunk = xx;
		if (xx > xMaxChunk) xMaxChunk = xx;

		for (int z = 0; z < zChunks; z++) {
			int zz = z * CHUNK_SIZE;
			int zOff = (zz + s1 - zc);
			if (zOff < 0) zOff -= (s2 - 1);
			zOff /= s2;
			zz -= zOff * s2;

			if (zz < zMinChunk) zMinChunk = zz;
			if (zz > zMaxChunk) zMaxChunk = zz;

			for (int y = 0; y < yChunks; y++) {
				int yy = y * CHUNK_SIZE;
				if (yy < yMinChunk) yMinChunk = yy;
				if (yy > yMaxChunk) yMaxChunk = yy;

				Chunk* chunk = chunks[(z * yChunks + y) * xChunks + x];
				bool wasDirty = chunk->isDirty();
				chunk->setPos(xx, yy, zz);
				if (!wasDirty && chunk->isDirty()) {
					dirtyChunks.push_back(chunk);
					++dirty;
				}
			}
		}
	}
}

int LevelRenderer::render( Mob* player, int layer, float alpha )
{
	if (mc->options.viewDistance != lastViewDistance) {
		allChanged();
	}

	TIMER_PUSH("sortchunks");
	for (int i = 0; i < 10; i++) {
		chunkFixOffs = (chunkFixOffs + 1) % chunksLength;
		Chunk* c = chunks[chunkFixOffs];
		if (c->isDirty() && std::find(dirtyChunks.begin(), dirtyChunks.end(), c) == dirtyChunks.end()) {
			dirtyChunks.push_back(c);
		}
	}

	if (layer == 0) {
		totalChunks = 0;
		offscreenChunks = 0;
		occludedChunks = 0;
		renderedChunks = 0;
		emptyChunks = 0;
	}

	float xOff = player->xOld + (player->x - player->xOld) * alpha;
	float yOff = player->yOld + (player->y - player->yOld) * alpha;
	float zOff = player->zOld + (player->z - player->zOld) * alpha;

	float xd = player->x - xOld;
	float yd = player->y - yOld;
	float zd = player->z - zOld;
	if (xd * xd + yd * yd + zd * zd > 4 * 4) {
		xOld = player->x;
		yOld = player->y;
		zOld = player->z;

		resortChunks(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
		DistanceChunkSorter distanceSorter(player);
		std::sort(sortedChunks, sortedChunks + chunksLength, distanceSorter);
	}

	int count = 0;
	if (occlusionCheck && !mc->options.anaglyph3d && layer == 0) {
		int from = 0;
		int to = 16;
		//checkQueryResults(from, to);
		for (int i = from; i < to; i++) {
			sortedChunks[i]->occlusion_visible = true;
		}

		count += renderChunks(from, to, layer, alpha);

		do {
			from = to;
			to = to * 2;
			if (to > chunksLength) to = chunksLength;

			glDisable2(GL_TEXTURE_2D);
			glDisable2(GL_LIGHTING);
			glDisable2(GL_ALPHA_TEST);
			glDisable2(GL_FOG);

			glColorMask(false, false, false, false);
			glDepthMask(false);
			//checkQueryResults(from, to);
			glPushMatrix2();
			float xo = 0;
			float yo = 0;
			float zo = 0;
			for (int i = from; i < to; i++) {
				if (sortedChunks[i]->isEmpty()) {
					sortedChunks[i]->visible = false;
					continue;
				}
				if (!sortedChunks[i]->visible) {
					sortedChunks[i]->occlusion_visible = true;
				}

				if (sortedChunks[i]->visible && !sortedChunks[i]->occlusion_querying) {
					float dist = Mth::sqrt(sortedChunks[i]->distanceToSqr(player));

					int frequency = (int) (1 + dist / 128);

					if (ticks % frequency == i % frequency) {
						Chunk* chunk = sortedChunks[i];
						float xt = (float) (chunk->x - xOff);
						float yt = (float) (chunk->y - yOff);
						float zt = (float) (chunk->z - zOff);
						float xdd = xt - xo;
						float ydd = yt - yo;
						float zdd = zt - zo;

						if (xdd != 0 || ydd != 0 || zdd != 0) {
							glTranslatef2(xdd, ydd, zdd);
							xo += xdd;
							yo += ydd;
							zo += zdd;
						}

						sortedChunks[i]->renderBB();
						sortedChunks[i]->occlusion_querying = true;
					}
				}
			}
			glPopMatrix2();
			glColorMask(true, true, true, true);
			glDepthMask(true);
			glEnable2(GL_TEXTURE_2D);
			glEnable2(GL_ALPHA_TEST);
			glEnable2(GL_FOG);

			count += renderChunks(from, to, layer, alpha);

		} while (to < chunksLength);

	} else {
		TIMER_POP_PUSH("render");
		count += renderChunks(0, chunksLength, layer, alpha);
	}

	TIMER_POP();
	return count;
}

void LevelRenderer::renderDebug(const AABB& b, float a) const {
	float x0 = b.x0;
	float x1 = b.x1;
	float y0 = b.y0;
	float y1 = b.y1;
	float z0 = b.z0;
	float z1 = b.z1;
	float u0 = 0, v0 = 0;
	float u1 = 1, v1 = 1;

	glEnable2(GL_BLEND);
	glBlendFunc2(GL_DST_COLOR, GL_SRC_COLOR);
	glDisable2(GL_TEXTURE_2D);
	glColor4f2(1, 1, 1, 1);

	textures->loadAndBindTexture("terrain.png");

	Tesselator& t = Tesselator::instance;
	t.begin();
	t.color(255, 255, 255, 255);

	t.offset(((Mob*)mc->player)->getPos(a).negated());

	// up
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x0, y0, z0, u0, v0);
	t.vertexUV(x1, y0, z0, u1, v0);
	t.vertexUV(x1, y0, z1, u1, v1);

	// down
	t.vertexUV(x1, y1, z1, u1, v1);
	t.vertexUV(x1, y1, z0, u1, v0);
	t.vertexUV(x0, y1, z0, u0, v0);
	t.vertexUV(x0, y1, z1, u0, v1);

	// north
	t.vertexUV(x0, y1, z0, u1, v0);
	t.vertexUV(x1, y1, z0, u0, v0);
	t.vertexUV(x1, y0, z0, u0, v1);
	t.vertexUV(x0, y0, z0, u1, v1);

	// south
	t.vertexUV(x0, y1, z1, u0, v0);
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z1, u1, v1);
	t.vertexUV(x1, y1, z1, u1, v0);

	// west
	t.vertexUV(x0, y1, z1, u1, v0);
	t.vertexUV(x0, y1, z0, u0, v0);
	t.vertexUV(x0, y0, z0, u0, v1);
	t.vertexUV(x0, y0, z1, u1, v1);

	// east
	t.vertexUV(x1, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z0, u1, v1);
	t.vertexUV(x1, y1, z0, u1, v0);
	t.vertexUV(x1, y1, z1, u0, v0);

	t.offset(0, 0, 0);
	t.draw();

	glEnable2(GL_TEXTURE_2D);
	glDisable2(GL_BLEND);
}

void LevelRenderer::render(const AABB& b) const
{
	Tesselator& t = Tesselator::instance;

	glColor4f2(1, 1, 1, 1);

	textures->loadAndBindTexture("terrain.png");

	//t.begin();
	t.color(255, 255, 255, 255);

	t.offset(((Mob*)mc->player)->getPos(0).negated());

	t.begin(GL_LINE_STRIP);
	t.vertex(b.x0, b.y0, b.z0);
	t.vertex(b.x1, b.y0, b.z0);
	t.vertex(b.x1, b.y0, b.z1);
	t.vertex(b.x0, b.y0, b.z1);
	t.vertex(b.x0, b.y0, b.z0);
	t.draw();

	t.begin(GL_LINE_STRIP);
	t.vertex(b.x0, b.y1, b.z0);
	t.vertex(b.x1, b.y1, b.z0);
	t.vertex(b.x1, b.y1, b.z1);
	t.vertex(b.x0, b.y1, b.z1);
	t.vertex(b.x0, b.y1, b.z0);
	t.draw();

	t.begin(GL_LINES);
	t.vertex(b.x0, b.y0, b.z0);
	t.vertex(b.x0, b.y1, b.z0);
	t.vertex(b.x1, b.y0, b.z0);
	t.vertex(b.x1, b.y1, b.z0);
	t.vertex(b.x1, b.y0, b.z1);
	t.vertex(b.x1, b.y1, b.z1);
	t.vertex(b.x0, b.y0, b.z1);
	t.vertex(b.x0, b.y1, b.z1);

	t.offset(0, 0, 0);
	t.draw();
}

//void LevelRenderer::checkQueryResults( int from, int to )
//{
//	for (int i = from; i < to; i++) {
//		if (sortedChunks[i]->occlusion_querying) {
//			// I wanna do a fast occusion culler here.
//		}
//	}
//}

int LevelRenderer::renderChunks( int from, int to, int layer, float alpha )
{
	_renderChunks.clear();
	int count = 0;
	for (int i = from; i < to; i++) {
		if (layer == 0) {
			totalChunks++;
			if (sortedChunks[i]->empty[layer]) emptyChunks++;
			else if (!sortedChunks[i]->visible) offscreenChunks++;
			else if (occlusionCheck && !sortedChunks[i]->occlusion_visible) occludedChunks++;
			else renderedChunks++;
		}

		if (!sortedChunks[i]->empty[layer] && sortedChunks[i]->visible && sortedChunks[i]->occlusion_visible) {
			int list = sortedChunks[i]->getList(layer);
			if (list >= 0) {
				_renderChunks.push_back(sortedChunks[i]);
				count++;
			}
		}
	}

	Mob* player = mc->cameraTargetPlayer;
	float xOff = player->xOld + (player->x - player->xOld) * alpha;
	float yOff = player->yOld + (player->y - player->yOld) * alpha;
	float zOff = player->zOld + (player->z - player->zOld) * alpha;

	//int lists = 0;
	renderList.clear();
	renderList.init(xOff, yOff, zOff);

	for (unsigned int i = 0; i < _renderChunks.size(); ++i) {
		Chunk* chunk = _renderChunks[i];
		#ifdef USE_VBO
			renderList.addR(chunk->getRenderChunk(layer));
		#else
			renderList.add(chunk->getList(layer));
		#endif
		renderList.next();
	}

	renderSameAsLast(layer, alpha);

	return count;
}

void LevelRenderer::renderSameAsLast( int layer, float alpha )
{
	renderList.render();
}

void LevelRenderer::tick()
{
	ticks++;
}

bool LevelRenderer::updateDirtyChunks( Mob* player, bool force )
{
	bool slow = false;

	if (slow) {
		DirtyChunkSorter dirtySorter(player);
		std::sort(dirtyChunks.begin(), dirtyChunks.end(), dirtySorter);
		int s = dirtyChunks.size() - 1;
		int amount = dirtyChunks.size();
		for (int i = 0; i < amount; i++) {
			Chunk* chunk = dirtyChunks[s-i];
			if (!force) {
				if (chunk->distanceToSqr(player) > 16 * 16) {
					if (chunk->visible) {
						if (i >= MAX_VISIBLE_REBUILDS_PER_FRAME) return false;
					} else {
						if (i >= MAX_INVISIBLE_REBUILDS_PER_FRAME) return false;
					}
				}
			} else {
				if (!chunk->visible) continue;
			}
			chunk->rebuild();

			dirtyChunks.erase( std::find(dirtyChunks.begin(), dirtyChunks.end(), chunk) ); // @q: s-i?
			chunk->setClean();
		}

		return dirtyChunks.size() == 0;
	} else {
		const int count = 3;

		DirtyChunkSorter dirtyChunkSorter(player);
		Chunk* toAdd[count] = {NULL};
		std::vector<Chunk*>* nearChunks = NULL;

		int pendingChunkSize = dirtyChunks.size();
		int pendingChunkRemoved = 0;

		for (int i = 0; i < pendingChunkSize; i++) {
			Chunk* chunk = dirtyChunks[i];

			if (!force) {
				if (chunk->distanceToSqr(player) > 1024.0f) {
					int index;

					// is this chunk in the closest <count>?
					for (index = 0; index < count; index++) {
						if (toAdd[index] != NULL && dirtyChunkSorter(toAdd[index], chunk) == false) {
							break;
						}
					}

					index--;

					if (index > 0) {
						int x = index;
						while (--x != 0) {
							toAdd[x - 1] = toAdd[x];
						}
						toAdd[index] = chunk;
					}

					continue;
				}
			} else if (!chunk->visible) {
				continue;
			}

			// chunk is very close -- always render

			if (nearChunks == NULL) {
				nearChunks = new std::vector<Chunk*>();
			}

			pendingChunkRemoved++;
			nearChunks->push_back(chunk);
			dirtyChunks[i] = NULL;
		}

		// if there are nearby chunks that need to be prepared for
		// rendering, sort them and then process them
		static const float MaxFrameTime = 1.0f / 100.0f;
		Stopwatch chunkWatch;
		chunkWatch.start();

		if (nearChunks != NULL) {
			if (nearChunks->size() > 1) {
				std::sort(nearChunks->begin(), nearChunks->end(), dirtyChunkSorter);
			}

			for (int i = nearChunks->size() - 1; i >= 0; i--) {
				Chunk* chunk = (*nearChunks)[i];
				chunk->rebuild();
				chunk->setClean();
			}
			delete nearChunks;
		}

		// render the nearest <count> chunks (farther than 1024 units away)
		int secondaryRemoved = 0;

		for (int i = count - 1; i >= 0; i--) {
			Chunk* chunk = toAdd[i];
			if (chunk != NULL) {

				float ttt = chunkWatch.stopContinue();
				if (ttt >= MaxFrameTime) {
					//LOGI("Too much work, I quit2!\n");
					break;
				}

				if (!chunk->visible && i != count - 1) {
					// escape early if chunks aren't ready
					toAdd[i] = NULL;
					toAdd[0] = NULL;
					break;
				}
				toAdd[i]->rebuild();
				toAdd[i]->setClean();
				secondaryRemoved++;
			}
		}

		// compact by removing nulls
		int cursor = 0;
		int target = 0;
		int arraySize = dirtyChunks.size();
		while (cursor != arraySize) {
			Chunk* chunk = dirtyChunks[cursor];
			if (chunk != NULL) {
				bool remove = false;
                for (int i = 0; i < count && !remove; i++)
                    if (chunk == toAdd[i]) {
                        remove = true;
                    }

                if (!remove) {
				//if (chunk == toAdd[0] || chunk == toAdd[1] || chunk == toAdd[2]) {
				//	; // this chunk was rendered and should be removed
				//} else {
					if (target != cursor) {
						dirtyChunks[target] = chunk;
					}
					target++;
				}
			}
			cursor++;
		}

		// trim
		if (cursor > target)
			dirtyChunks.erase(dirtyChunks.begin() + target, dirtyChunks.end());

		return pendingChunkSize == (pendingChunkRemoved + secondaryRemoved);
	}
}

void LevelRenderer::renderHit( Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a )
{
	if (mode == 0) {
		if (destroyProgress > 0) {
			Tesselator& t = Tesselator::instance;
			glEnable2(GL_BLEND);
			glBlendFunc2(GL_DST_COLOR, GL_SRC_COLOR);

			textures->loadAndBindTexture("terrain.png");
			glPushMatrix2();

			int tileId = level->getTile(h.x, h.y, h.z);
			Tile* tile = tileId > 0 ? Tile::tiles[tileId] : NULL;
			//glDisable2(GL_ALPHA_TEST);

			glPolygonOffset(-3.0f, -3.0f);
			glEnable2(GL_POLYGON_OFFSET_FILL);
			t.begin();
			t.color(1.0f, 1.0f, 1.0f, 0.5f);
			t.noColor();
			float xo = player->xOld + (player->x - player->xOld) * a;
			float yo = player->yOld + (player->y - player->yOld) * a;
			float zo = player->zOld + (player->z - player->zOld) * a;

			t.offset(-xo, -yo, -zo);
			//t.noColor();

			if (tile == NULL) tile = Tile::rock;
			const int progress = (int) (destroyProgress * 10);
			tileRenderer->tesselateInWorld(tile, h.x, h.y, h.z, 15 * 16 + progress);

			t.draw();
			t.offset(0, 0, 0);
			glPolygonOffset(0.0f, 0.0f);
			glDisable2(GL_POLYGON_OFFSET_FILL);
			//glDisable2(GL_ALPHA_TEST);
			glDisable2(GL_BLEND);

			glDepthMask(true);
			glPopMatrix2();
		}
	}
	//else if (inventoryItem != NULL) {
	//          glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//          float br = ((float) (util.Mth::sin(System.currentTimeMillis() / 100.0f)) * 0.2f + 0.8f);
	//          glColor4f2(br, br, br, ((float) (util.Mth::sin(System.currentTimeMillis() / 200.0f)) * 0.2f + 0.5f));

	//          int id = textures.loadTexture("terrain.png");
	//          glBindTexture2(GL_TEXTURE_2D, id);
	//          int x = h.x;
	//          int y = h.y;
	//          int z = h.z;
	//          if (h.f == 0) y--;
	//          if (h.f == 1) y++;
	//          if (h.f == 2) z--;
	//          if (h.f == 3) z++;
	//          if (h.f == 4) x--;
	//          if (h.f == 5) x++;
	//          /*
	//           * t.begin(); t.noColor(); Tile.tiles[tileType].tesselate(level, x,
	//           * y, z, t); t.end();
	//           */
	//      }
}

void LevelRenderer::renderHitOutline( Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a )
{
	if (mode == 0 && h.type == TILE) {
		glEnable2(GL_BLEND);
		glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f2(0, 0, 0, 0.4f);
		glLineWidth(1.0f);
		glDisable2(GL_TEXTURE_2D);
		glDepthMask(false);
		float ss = 0.002f;
		int tileId = level->getTile(h.x, h.y, h.z);

		if (tileId > 0) {
			Tile::tiles[tileId]->updateShape(level, h.x, h.y, h.z);
			float xo = player->xOld + (player->x - player->xOld) * a;
			float yo = player->yOld + (player->y - player->yOld) * a;
			float zo = player->zOld + (player->z - player->zOld) * a;
			render(Tile::tiles[tileId]->getTileAABB(level, h.x, h.y, h.z).grow(ss, ss, ss).cloneMove(-xo, -yo, -zo));
		}
		glDepthMask(true);
		glEnable2(GL_TEXTURE_2D);
		glDisable2(GL_BLEND);
	}
}

void LevelRenderer::setDirty( int x0, int y0, int z0, int x1, int y1, int z1 )
{
	int _x0 = Mth::intFloorDiv(x0, CHUNK_SIZE);
	int _y0 = Mth::intFloorDiv(y0, CHUNK_SIZE);
	int _z0 = Mth::intFloorDiv(z0, CHUNK_SIZE);
	int _x1 = Mth::intFloorDiv(x1, CHUNK_SIZE);
	int _y1 = Mth::intFloorDiv(y1, CHUNK_SIZE);
	int _z1 = Mth::intFloorDiv(z1, CHUNK_SIZE);

	for (int x = _x0; x <= _x1; x++) {
		int xx = x % xChunks;
		if (xx < 0) xx += xChunks;
		for (int y = _y0; y <= _y1; y++) {
			int yy = y % yChunks;
			if (yy < 0) yy += yChunks;
			for (int z = _z0; z <= _z1; z++) {
				int zz = z % zChunks;
				if (zz < 0) zz += zChunks;

				int p = ((zz) * yChunks + (yy)) * xChunks + (xx);
				Chunk* chunk = chunks[p];
				if (!chunk->isDirty()) {
					dirtyChunks.push_back(chunk);
					chunk->setDirty();
				}
			}
		}
	}
}

void LevelRenderer::tileChanged( int x, int y, int z)
{
	setDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}


void LevelRenderer::setTilesDirty( int x0, int y0, int z0, int x1, int y1, int z1 )
{
	setDirty(x0 - 1, y0 - 1, z0 - 1, x1 + 1, y1 + 1, z1 + 1);
}


void LevelRenderer::cull( Culler* culler, float a )
{
	for (int i = 0; i < chunksLength; i++) {
		if (!chunks[i]->isEmpty()) {
			if (!chunks[i]->visible || ((i + cullStep) & 15) == 0) {
				chunks[i]->cull(culler);
			}
		}
	}
	cullStep++;
}

void LevelRenderer::skyColorChanged()
{
	for (int i = 0; i < chunksLength; i++) {
		if (chunks[i]->skyLit) {
			if (!chunks[i]->isDirty()) {
				dirtyChunks.push_back(chunks[i]);
				chunks[i]->setDirty();
			}
		}
	}
}

bool entityRenderPredicate(const Entity* a, const Entity* b) {
	return a->entityRendererId < b->entityRendererId;
}

void LevelRenderer::renderEntities(Vec3 cam, Culler* culler, float a) {
    if (noEntityRenderFrames > 0) {
        noEntityRenderFrames--;
        return;
    }

	TIMER_PUSH("prepare");
    TileEntityRenderDispatcher::getInstance()->prepare(level, textures, mc->font, mc->cameraTargetPlayer, a);
    EntityRenderDispatcher::getInstance()->prepare(level, mc->font, mc->cameraTargetPlayer, &mc->options, a);

    totalEntities = 0;
    renderedEntities = 0;
    culledEntities = 0;

	Entity* player = mc->cameraTargetPlayer;
    EntityRenderDispatcher::xOff = TileEntityRenderDispatcher::xOff = (player->xOld + (player->x - player->xOld) * a);
    EntityRenderDispatcher::yOff = TileEntityRenderDispatcher::yOff = (player->yOld + (player->y - player->yOld) * a);
    EntityRenderDispatcher::zOff = TileEntityRenderDispatcher::zOff = (player->zOld + (player->z - player->zOld) * a);

	glEnableClientState2(GL_VERTEX_ARRAY);
	glEnableClientState2(GL_TEXTURE_COORD_ARRAY);

	TIMER_POP_PUSH("entities");
	const EntityList& entities = level->getAllEntities();
	totalEntities = entities.size();
	if (totalEntities > 0) {
		Entity** toRender = new Entity*[totalEntities];
		for (int i = 0; i < totalEntities; i++) {
			Entity* entity = entities[i];

			if (entity->shouldRender(cam) && culler->isVisible(entity->bb))
			{
				if (entity == mc->cameraTargetPlayer && mc->options.thirdPersonView == 0 && mc->cameraTargetPlayer->isPlayer() && !((Player*)mc->cameraTargetPlayer)->isSleeping()) continue;
				if (entity == mc->cameraTargetPlayer && !mc->options.thirdPersonView)
					continue;
				if (!level->hasChunkAt(Mth::floor(entity->x), Mth::floor(entity->y), Mth::floor(entity->z)))
					continue;

				toRender[renderedEntities++] = entity;
				//EntityRenderDispatcher::getInstance()->render(entity, a);
			}
		}

		if (renderedEntities > 0) {
			std::sort(&toRender[0], &toRender[renderedEntities], entityRenderPredicate);
			for (int i = 0; i < renderedEntities; ++i) {
				EntityRenderDispatcher* disp = EntityRenderDispatcher::getInstance();
				disp->render(toRender[i], a);
			}
		}

		delete[] toRender;
	}

    TIMER_POP_PUSH("tileentities");
    for (unsigned int i = 0; i < level->tileEntities.size(); i++) {
        TileEntityRenderDispatcher::getInstance()->render(level->tileEntities[i], a);
    }

	glDisableClientState2(GL_VERTEX_ARRAY);
	glDisableClientState2(GL_TEXTURE_COORD_ARRAY);

	TIMER_POP();
}

std::string LevelRenderer::gatherStats1() {
	std::stringstream ss;
	ss << "C: " << renderedChunks << "/" << totalChunks << ". F: " << offscreenChunks << ", O: " << occludedChunks << ", E: " << emptyChunks << "\n";
    return ss.str();
}

//
//    /*public*/ std::string gatherStats2() {
//        return "E: " + renderedEntities + "/" + totalEntities + ". B: " + culledEntities + ", I: " + ((totalEntities - culledEntities) - renderedEntities);
//    }
//
//    int[] toRender = new int[50000];
//    IntBuffer resultBuffer = MemoryTracker.createIntBuffer(64);

void LevelRenderer::renderSky(float alpha) {
    if (mc->level->dimension->foggy) return;

    glDisable2(GL_TEXTURE_2D);
    Vec3 sc = level->getSkyColor(mc->cameraTargetPlayer, alpha);
    float sr = (float) sc.x;
    float sg = (float) sc.y;
    float sb = (float) sc.z;// + 0.5f;

    if (mc->options.anaglyph3d) {
        float srr = (sr * 30.0f + sg * 59.0f + sb * 11.0f) / 100.0f;
        float sgg = (sr * 30.0f + sg * 70.0f) / (100.0f);
        float sbb = (sr * 30.0f + sb * 70.0f) / (100.0f);

        sr = srr;
        sg = sgg;
        sb = sbb;
    }
    glColor4f2(sr, sg, Mth::Min(1.0f, sb), 1);

    //Tesselator& t = Tesselator::instance;

    glEnable2(GL_FOG);
    glColor4f2(sr, sg, sb, 1.0f);

#ifdef OPENGL_ES
	drawArrayVT(skyBuffer, skyVertexCount);
#endif
    glEnable2(GL_TEXTURE_2D);
}

void LevelRenderer::renderClouds( float alpha ) {
	//if (!mc->level->dimension->isNaturalDimension()) return;
	glEnable2(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	float yOffs = (float) (mc->player->yOld + (mc->player->y - mc->player->yOld) * alpha);
	int s = 32;
	int d = 256 / s;
	Tesselator& t = Tesselator::instance;

	//glBindTexture(GL_TEXTURE_2D, texturesloadTexture("/environment/clouds.png"));
	textures->loadAndBindTexture("environment/clouds.png");
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Vec3 cc = level->getCloudColor(alpha);
	float cr = (float) cc.x;
	float cg = (float) cc.y;
	float cb = (float) cc.z;

	float scale = 1 / 2048.0f;

	float time = (ticks + alpha);
	float xo = mc->player->xo + (mc->player->x - mc->player->xo) * alpha + time * 0.03f;
	float zo = mc->player->zo + (mc->player->z - mc->player->zo) * alpha;
	int xOffs = Mth::floor(xo / 2048);
	int zOffs = Mth::floor(zo / 2048);
	xo -= xOffs * 2048;
	zo -= zOffs * 2048;

	float yy = /*level.dimension.getCloudHeight()*/ 128 - yOffs + 0.33f;//mc->player->y + 1;
	float uo = (float) (xo * scale);
	float vo = (float) (zo * scale);
	t.begin();

	t.color(cr, cg, cb, 0.8f);
	for (int xx = -s * d; xx < +s * d; xx += s) {
		for (int zz = -s * d; zz < +s * d; zz += s) {
			t.vertexUV((float)xx, yy, (float)zz + s, xx * scale + uo, (zz + s) * scale + vo);
			t.vertexUV((float)xx + s, yy, (float)zz + s, (xx + s) * scale + uo, (zz + s) * scale + vo);
			t.vertexUV((float)xx + s, yy, (float)zz, (xx + s) * scale + uo, zz * scale + vo);
			t.vertexUV((float)xx, yy, (float)zz, xx * scale + uo, zz * scale + vo);
		}
	}
	t.endOverrideAndDraw();
	glColor4f(1, 1, 1, 1.0f);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}

void LevelRenderer::playSound(const std::string& name, float x, float y, float z, float volume, float pitch) {
	// @todo: deny sounds here if sound is off (rather than waiting 'til SoundEngine)
	float dd = 16;

    if (volume > 1) dd *= volume;
    if (mc->cameraTargetPlayer->distanceToSqr(x, y, z) < dd * dd) {
        mc->soundEngine->play(name, x, y, z, volume, pitch);
    }
}

void LevelRenderer::addParticle(const std::string& name, float x, float y, float z, float xa, float ya, float za, int data) {

	float xd = mc->cameraTargetPlayer->x - x;
    float yd = mc->cameraTargetPlayer->y - y;
    float zd = mc->cameraTargetPlayer->z - z;
	float distanceSquared = xd * xd + yd * yd + zd * zd;

	//Particle* p = NULL;
	//if (name == "hugeexplosion") p = new HugeExplosionSeedParticle(level, x, y, z, xa, ya, za);
	//else if (name == "largeexplode") p = new HugeExplosionParticle(textures, level, x, y, z, xa, ya, za);

	//if (p) {
	//	if (distanceSquared < 32 * 32) {
	//		mc->particleEngine->add(p);
	//	} else { delete p; }
	//	return;
	//}

    const float particleDistance = 16;
    if (distanceSquared > particleDistance * particleDistance) return;

	//static Stopwatch sw;
	//sw.start();

    if (name == "bubble") mc->particleEngine->add(new BubbleParticle(level, x, y, z, xa, ya, za));
	else if (name == "crit") mc->particleEngine->add(new CritParticle2(level, x, y, z, xa, ya, za));
	else if (name == "smoke") mc->particleEngine->add(new SmokeParticle(level, x, y, z, xa, ya, za));
    //else if (name == "note") mc->particleEngine->add(new NoteParticle(level, x, y, z, xa, ya, za));
    else if (name == "explode") mc->particleEngine->add(new ExplodeParticle(level, x, y, z, xa, ya, za));
    else if (name == "flame") mc->particleEngine->add(new FlameParticle(level, x, y, z, xa, ya, za));
    else if (name == "lava") mc->particleEngine->add(new LavaParticle(level, x, y, z));
    //else if (name == "splash") mc->particleEngine->add(new SplashParticle(level, x, y, z, xa, ya, za));
	else if (name == "largesmoke") mc->particleEngine->add(new SmokeParticle(level, x, y, z, xa, ya, za, 2.5f));
    else if (name == "reddust") mc->particleEngine->add(new RedDustParticle(level, x, y, z, xa, ya, za));
	else if (name == "iconcrack") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, xa, ya, za, Item::items[data]));
	else if (name == "snowballpoof") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, Item::snowBall));
    //else if (name == "snowballpoof") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, Item::snowBall));
    //else if (name == "slime") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, Item::slimeBall));
    //else if (name == "heart") mc->particleEngine->add(new HeartParticle(level, x, y, z, xa, ya, za));

	//sw.stop();
	//sw.printEvery(50, "add-particle-string");
}

/*
void LevelRenderer::addParticle(ParticleType::Id name, float x, float y, float z, float xa, float ya, float za, int data) {
	float xd = mc->cameraTargetPlayer->x - x;
	float yd = mc->cameraTargetPlayer->y - y;
	float zd = mc->cameraTargetPlayer->z - z;

	const float particleDistance = 16;
	if (xd * xd + yd * yd + zd * zd > particleDistance * particleDistance) return;

	//static Stopwatch sw;
	//sw.start();

	//Particle* p = NULL;

	if (name == ParticleType::bubble)		mc->particleEngine->add( new BubbleParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::crit)		mc->particleEngine->add(new CritParticle2(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::smoke)		mc->particleEngine->add(new SmokeParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::explode)		mc->particleEngine->add( new ExplodeParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::flame)		mc->particleEngine->add( new FlameParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::lava)		mc->particleEngine->add( new LavaParticle(level, x, y, z) );
	else if (name == ParticleType::largesmoke)	mc->particleEngine->add( new SmokeParticle(level, x, y, z, xa, ya, za, 2.5f) );
	else if (name == ParticleType::reddust)		mc->particleEngine->add( new RedDustParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::iconcrack)	mc->particleEngine->add( new BreakingItemParticle(level, x, y, z, xa, ya, za, Item::items[data]) );

	//switch (name) {
	//	case ParticleType::bubble:		p = new BubbleParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::crit:		p = new CritParticle2(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::smoke:		p = new SmokeParticle(level, x, y, z, xa, ya, za); break;
	//	//case ParticleType::note: p = new NoteParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::explode:		p = new ExplodeParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::flame:		p = new FlameParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::lava:		p = new LavaParticle(level, x, y, z); break;
	//	//case ParticleType::splash: p = new SplashParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::largesmoke:	p = new SmokeParticle(level, x, y, z, xa, ya, za, 2.5f); break;
	//	case ParticleType::reddust:		p = new RedDustParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::iconcrack:	p = new BreakingItemParticle(level, x, y, z, xa, ya, za, Item::items[data]); break;
	//	//case ParticleType::snowballpoof: p = new BreakingItemParticle(level, x, y, z, Item::snowBall); break;
	//	//case ParticleType::slime: p = new BreakingItemParticle(level, x, y, z, Item::slimeBall); break;
	//	//case ParticleType::heart: p = new HeartParticle(level, x, y, z, xa, ya, za); break;
	//	default:
	//		LOGW("Couldn't find particle of type: %d\n", name);
	//		break;
	//}
	//if (p) {
	//	mc->particleEngine->add(p);
	//}

	//sw.stop();
	//sw.printEvery(50, "add-particle-enum");
}
*/

void LevelRenderer::renderHitSelect( Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a )
{
	//if (h.type == TILE) LOGI("type: %s @ (%d, %d, %d)\n", Tile::tiles[level->getTile(h.x, h.y, h.z)]->getDescriptionId().c_str(), h.x, h.y, h.z);

	if (mode == 0) {

		Tesselator& t = Tesselator::instance;
		glEnable2(GL_BLEND);
		glDisable2(GL_TEXTURE_2D);
		glBlendFunc2(GL_SRC_ALPHA, GL_ONE);
		glBlendFunc2(GL_DST_COLOR, GL_SRC_COLOR);
		glEnable2(GL_DEPTH_TEST);

		textures->loadAndBindTexture("terrain.png");
		
		int tileId = level->getTile(h.x, h.y, h.z);
		Tile* tile = tileId > 0 ? Tile::tiles[tileId] : NULL;
		glDisable2(GL_ALPHA_TEST);

		//LOGI("block: %d - %d (%s)\n", tileId, level->getData(h.x, h.y, h.z), tile==NULL?"null" : tile->getDescriptionId().c_str() );

		const float br = 0.65f;
		glColor4f2(br * 1.0f, br * 1.0f, br * 1.0f, br * 1.0f);
		glPushMatrix2();

		//glPolygonOffset(-.3f, -.3f);
		glPolygonOffset(-1.f, -1.f); //Implementation dependent units
		glEnable2(GL_POLYGON_OFFSET_FILL);
		float xo = player->xOld + (player->x - player->xOld) * a;
		float yo = player->yOld + (player->y - player->yOld) * a;
		float zo = player->zOld + (player->z - player->zOld) * a;

		t.begin();
		t.offset(-xo, -yo, -zo);
		t.noColor();

		if (tile == NULL) tile = Tile::rock;
		tileRenderer->tesselateInWorld(tile, h.x, h.y, h.z);

		t.draw();
		t.offset(0, 0, 0);
		glPolygonOffset(0.0f, 0.0f);

		glDisable2(GL_POLYGON_OFFSET_FILL);
		glEnable2(GL_TEXTURE_2D);

		glDepthMask(true);
		glPopMatrix2();

		glEnable2(GL_ALPHA_TEST);
		glDisable2(GL_BLEND);
	}
}

void LevelRenderer::onGraphicsReset()
{
	generateSky();

	// Get new buffers
#ifdef OPENGL_ES
	glGenBuffers2(numListsOrBuffers, chunkBuffers);
#else
	chunkLists = glGenLists(numListsOrBuffers);
#endif

	// Rebuild
	allChanged();
}

void LevelRenderer::entityAdded( Entity* entity )
{
	if (!entity->isPlayer())
		return;

	// Hack to (hopefully) get the players to show
	EntityRenderDispatcher::getInstance()->onGraphicsReset();
}

int _t_keepPic = -1;

void LevelRenderer::takePicture( TripodCamera* cam, Entity* entity )
{
	// Push old values
	Mob* oldCameraEntity = mc->cameraTargetPlayer;
	bool hideGui = mc->options.hideGui;
	bool thirdPerson = mc->options.thirdPersonView;

	// @huge @attn: This is highly illegal, super temp!
	mc->cameraTargetPlayer = (Mob*)cam;
	mc->options.hideGui = true;
	mc->options.thirdPersonView = false;

	mc->gameRenderer->renderLevel(0);

	// Pop values back
	mc->cameraTargetPlayer = oldCameraEntity;
	mc->options.hideGui = hideGui;
	mc->options.thirdPersonView = thirdPerson;

	_t_keepPic = -1;

	// Save image
	static char filename[256];
	sprintf(filename, "%s/games/com.mojang/img_%.4d.jpg", mc->externalStoragePath.c_str(), getTimeMs());

	mc->platform()->saveScreenshot(filename, mc->width, mc->height);
}

void LevelRenderer::levelEvent(Player* player, int type, int x, int y, int z, int data) {
	switch (type) {    
	case LevelEvent::SOUND_OPEN_DOOR:
        if (Mth::random() < 0.5f) {
            level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.door_open", 1, level->random.nextFloat() * 0.1f + 0.9f);
        } else {
            level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.door_close", 1, level->random.nextFloat() * 0.1f + 0.9f);
        }
        break;
	}
}
