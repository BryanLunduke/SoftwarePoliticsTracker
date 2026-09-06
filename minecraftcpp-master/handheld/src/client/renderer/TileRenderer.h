#ifndef NET_MINECRAFT_CLIENT_RENDERER__TileRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER__TileRenderer_H__

//package net.minecraft.client.renderer;

class Tile;
class FenceTile;
class FenceGateTile;
class ThinFenceTile;
class StairTile;
class LevelSource;
class Material;

class TileRenderer
{
public:
    TileRenderer(LevelSource* level = 0);

    void tesselateInWorld(Tile* tile, int x, int y, int z, int fixedTexture);
    bool tesselateInWorld(Tile* tt, int x, int y, int z);
	void tesselateInWorldNoCulling(Tile* tile, int x, int y, int z);

	bool tesselateTorchInWorld(Tile* tt, int x, int y, int z);
    bool tesselateLadderInWorld(Tile* tt, int x, int y, int z);
    bool tesselateCactusInWorld(Tile* tt, int x, int y, int z);
    bool tesselateCactusInWorld(Tile* tt, int x, int y, int z, float r, float g, float b);
	bool tesselateCrossInWorld(Tile* tt, int x, int y, int z);
	bool tesselateStemInWorld(Tile* _tt, int x, int y, int z);
	bool tesselateWaterInWorld(Tile* tt, int x, int y, int z);
	bool tesselateStairsInWorld(StairTile* tt, int x, int y, int z);
	bool tesselateDoorInWorld(Tile* tt, int x, int y, int z);
	bool tesselateFenceInWorld(FenceTile* tt, int x, int y, int z);
	bool tesselateThinFenceInWorld(ThinFenceTile* tt, int x, int y, int z);
	bool tesselateFenceGateInWorld(FenceGateTile* tt, int x, int y, int z);
	bool tesselateBedInWorld(Tile *tt, int x, int y, int z);
	bool tesselateRowInWorld(Tile* tt, int x, int y, int z);

    void tesselateTorch(Tile* tt, float x, float y, float z, float xxa, float zza);
	void tesselateCrossTexture(Tile* tt, int data, float x, float y, float z);
	void tesselateStemTexture(Tile* tt, int data, float h, float x, float y, float z);
	void tesselateStemDirTexture(Tile* tt, int data, int dir, float h, float x, float y, float z);
	void tesselateRowTexture(Tile* tt, int data, float x, float y, float z);

    void renderBlock(Tile* tt, LevelSource* level, int x, int y, int z);

    /*public*/
	bool tesselateBlockInWorld(Tile* tt, int x, int y, int z);
    bool tesselateBlockInWorld(Tile* tt, int x, int y, int z, float r, float g, float b);
	bool tesselateBlockInWorldWithAmbienceOcclusion(Tile* tt, int pX, int pY, int pZ, float pBaseRed, float pBaseGreen, float pBaseBlue);

    void renderFaceDown(Tile* tt, float x, float y, float z, int tex);
    void renderFaceUp(Tile* tt, float x, float y, float z, int tex);
    void renderNorth(Tile* tt, float x, float y, float z, int tex);
    void renderSouth(Tile* tt, float x, float y, float z, int tex);
    void renderWest(Tile* tt, float x, float y, float z, int tex);
    void renderEast(Tile* tt, float x, float y, float z, int tex);

    void renderTile(Tile* tile, int data);
	void renderGuiTile(Tile* tile, int data);

    static bool canRender(int renderShape);
private:
    float getWaterHeight(int x, int y, int z, const Material* m);

    LevelSource* level;
	int fixedTexture;
	bool xFlipTexture;
	bool noCulling;

	bool applyAmbienceOcclusion;
	float ll000, llx00, ll0y0, ll00z, llX00, ll0Y0, ll00Z;
	float llxyz, llxy0, llxyZ, ll0yz, ll0yZ, llXyz, llXy0;
	float llXyZ, llxYz, llxY0, llxYZ, ll0Yz, llXYz, llXY0;
	float ll0YZ, llXYZ, llx0z, llX0z, llx0Z, llX0Z;
	int blsmooth;
	float c1r, c2r, c3r, c4r;
	float c1g, c2g, c3g, c4g;
	float c1b, c2b, c3b, c4b;
	bool llTrans0Yz, llTransXY0, llTransxY0, llTrans0YZ;
	bool llTransx0z, llTransX0Z, llTransx0Z, llTransX0z;
	bool llTrans0yz, llTransXy0, llTransxy0, llTrans0yZ;
};


#endif /*NET_MINECRAFT_CLIENT_RENDERER__TileRenderer_H__*/
