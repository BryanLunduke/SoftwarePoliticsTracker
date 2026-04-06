#ifndef NET_MINECRAFT_CLIENT_RENDERER__LevelRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER__LevelRenderer_H__

//package net.minecraft.client.renderer;

#include "../../world/level/LevelListener.h"
#include "../../world/phys/Vec3.h"
#include "RenderList.h"
#include "gles.h"
#include <vector>

class Minecraft;
class Textures;
class Culler;
class Chunk;
class TileRenderer;
class Level;
class Mob;
class Player;
class HitResult;
class AABB;
class TripodCamera;

class LevelRenderer: public LevelListener
{
public:
    static const int CHUNK_SIZE;
    static const int MAX_VISIBLE_REBUILDS_PER_FRAME = 3;
    static const int MAX_INVISIBLE_REBUILDS_PER_FRAME = 1;
    
    float xOld;
    float yOld;
    float zOld;
    float destroyProgress;

	LevelRenderer(Minecraft* mc);
	~LevelRenderer();

	void setLevel(Level* level);
	void allChanged();

    int  render(Mob* player, int layer, float alpha);
	void renderDebug(const AABB& b, float a) const;

	void renderSky(float alpha);
	void renderClouds(float alpha);
	void renderEntities(Vec3 cam, Culler* culler, float a);
    void renderSameAsLast(int layer, float alpha);
	void renderHit(Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a);
    void renderHitOutline(Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a);
	void renderHitSelect(Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a);
	void entityAdded(Entity* entity);

	void tick();
	bool updateDirtyChunks(Mob* player, bool force);
	void setDirty(int x0, int y0, int z0, int x1, int y1, int z1);
    void tileChanged(int x, int y, int z);
    void setTilesDirty(int x0, int y0, int z0, int x1, int y1, int z1);
	void cull(Culler* culler, float a);
    void skyColorChanged();

	//void addParticle(ParticleType::Id name, float x, float y, float z, float xa, float ya, float za, int data);
	void addParticle(const std::string& name, float x, float y, float z, float xa, float ya, float za, int data);
	void playSound(const std::string& name, float x, float y, float z, float volume, float pitch);
	void takePicture(TripodCamera* cam, Entity* entity);

	void levelEvent(Player* source, int type, int x, int y, int z, int data);

	std::string gatherStats1();

	void render(const AABB& b) const;
	void onGraphicsReset();
private:
	void generateSky();

	int  renderChunks(int from, int to, int layer, float alpha);
	void resortChunks(int xc, int yc, int zc);
	void deleteChunks();
	//void checkQueryResults(int from, int to);
	__inline int getLinearCoord(int x, int y, int z) {
		return (z * yChunks + y) * xChunks + x;
	}
	int noEntityRenderFrames;
    int totalEntities;
    int renderedEntities;
    int culledEntities;

	std::vector<Chunk*> _renderChunks;

    int cullStep;
	//static const int renderListsLength = 4;
    RenderList renderList;//[renderListsLength];

	int totalChunks, offscreenChunks, occludedChunks, renderedChunks, emptyChunks;
    int chunkFixOffs;

    int xMinChunk, yMinChunk, zMinChunk;
    int xMaxChunk, yMaxChunk, zMaxChunk;

	Level* level;
	std::vector<Chunk*> dirtyChunks;

	Chunk** chunks;
    Chunk** sortedChunks;
	int chunksLength;
public:
	TileRenderer* tileRenderer;
private:
	int xChunks, yChunks, zChunks;
    int chunkLists;
    Minecraft* mc;

	bool occlusionCheck;
	int lastViewDistance;

	int ticks;
    int starList, skyList, darkList;

	int numListsOrBuffers;
	GLuint* chunkBuffers;
	GLuint skyBuffer;
	int skyVertexCount;

//    /*public*/ std::vector<TileEntity*> renderableTileEntities;
    Textures* textures;
//    /*private*/ TileRenderer tileRenderer;
//    /*private*/ IntBuffer occlusionCheckIds;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__LevelRenderer_H__*/
