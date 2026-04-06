#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__Tile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__Tile_H__

//package net.minecraft.world.level.tile;

#include <string>
#include "../../phys/AABB.h"

class Entity;
class Mob;
class Player;
class Level;
class LevelSource;
class Material;
class Random;
class ItemInstance;

class Bush;
class GrassTile;
class LeafTile;
class FireTile;

// @Note: Got a memory leak in initTiles? You probably didn't call
//        Tile::init after constructing the tile!
class Tile
{
    static const std::string TILE_DESCRIPTION_PREFIX;

public:
    class SoundType
	{
	public:
        //const std::string name;
        const float volume;
        const float pitch;
		const std::string breakSound;
		const std::string stepSound;

        SoundType(const std::string& name, float volume, float pitch)
		:	volume(volume),
			pitch(pitch),
			breakSound("step." + name),
			stepSound("step." + name)
		{}
		SoundType(const std::string& name, const std::string& breakSound, float volume, float pitch)
		:	volume(volume),
			pitch(pitch),
			stepSound("step." + name),
			breakSound(breakSound)
		{}

        float getVolume() const {
            return volume;
        }

        float getPitch() const {
            return pitch;
        }

        const std::string& getBreakSound() const {
            return breakSound;
        }

        const std::string& getStepSound() const {
            return stepSound;
        }
	};

	static const SoundType SOUND_NORMAL;
	static const SoundType SOUND_WOOD;
	static const SoundType SOUND_GRAVEL;
	static const SoundType SOUND_GRASS;
	static const SoundType SOUND_STONE;
	static const SoundType SOUND_METAL;
	static const SoundType SOUND_GLASS;
	static const SoundType SOUND_CLOTH;
	static const SoundType SOUND_SAND;
	static const SoundType SOUND_SILENT;

    static const int SHAPE_INVISIBLE = -1;
    static const int SHAPE_BLOCK = 0;
    static const int SHAPE_CROSS_TEXTURE = 1;
    static const int SHAPE_TORCH = 2;
    static const int SHAPE_FIRE = 3;
    static const int SHAPE_WATER = 4;
    static const int SHAPE_RED_DUST = 5;
    static const int SHAPE_ROWS = 6;
    static const int SHAPE_DOOR = 7;
    static const int SHAPE_LADDER = 8;
    static const int SHAPE_RAIL = 9;
    static const int SHAPE_STAIRS = 10;
    static const int SHAPE_FENCE = 11;
    static const int SHAPE_LEVER = 12;
    static const int SHAPE_CACTUS = 13;
    static const int SHAPE_BED = 14;
    static const int SHAPE_DIODE = 15;
    static const int SHAPE_IRON_FENCE = 18;
	static const int SHAPE_STEM = 19;
    static const int SHAPE_FENCE_GATE = 21;
	static const int SHAPE_ENTITYTILE_ANIMATED = 22;


	static const int NUM_BLOCK_TYPES = 256;

    static Tile* tiles[NUM_BLOCK_TYPES];

	static bool sendTileData[NUM_BLOCK_TYPES];
    static bool shouldTick[NUM_BLOCK_TYPES];
    static bool solid[NUM_BLOCK_TYPES];
    static bool isEntityTile[NUM_BLOCK_TYPES];
    static int lightBlock[NUM_BLOCK_TYPES];
    static bool translucent[NUM_BLOCK_TYPES];
    static int lightEmission[NUM_BLOCK_TYPES];

    static Tile* rock;
	static Tile* grass;
    static Tile* dirt;
    static Tile* stoneBrick;
    static Tile* wood;
    static Tile* sapling;
    static Tile* unbreakable;
    static Tile* water;
    static Tile* calmWater;
    static Tile* lava;
    static Tile* calmLava;
    static Tile* sand;
    static Tile* gravel;
    static Tile* goldOre;
    static Tile* ironOre;
    static Tile* coalOre;
    static Tile* treeTrunk;
	static LeafTile* leaves;
    static Tile* sponge;
	static Tile* web;
    static Tile* glass;
	static Tile* thinGlass;
    static Tile* lapisOre;
    static Tile* lapisBlock;
    static Tile* dispenser;
    static Tile* sandStone;
    static Tile* musicBlock;
    static Tile* bed;
    static Tile* unused_27;
    static Tile* unused_28;
    static Tile* unused_29;
    static Tile* unused_30;
    static Tile* tallgrass;
    static Tile* unused_32;
    static Tile* unused_33;
    static Tile* unused_34;
    static Tile* unused_36;

	static Tile* cloth;
	static Tile* flower;
	static Tile* rose;
	static Tile* mushroom1;
	static Tile* mushroom2;
    static Tile* goldBlock;
    static Tile* ironBlock;
    static Tile* stoneSlab;
    static Tile* stoneSlabHalf;
    static Tile* redBrick;
    static Tile* tnt;
    static Tile* bookshelf;
    static Tile* mossStone;
    static Tile* obsidian;
    static Tile* torch;
    static FireTile* fire;
    static Tile* mobSpawner;
    static Tile* stairs_wood;
    static Tile* chest;
    static Tile* redStoneDust;
    static Tile* emeraldOre;
    static Tile* emeraldBlock;
    static Tile* workBench;
	static Tile* stonecutterBench;
    static Tile* crops;
    static Tile* farmland;
    static Tile* furnace;
    static Tile* furnace_lit;
    static Tile* sign;
    static Tile* door_wood;
    static Tile* ladder;
    static Tile* rail;
    static Tile* stairs_stone;
    static Tile* wallSign;
    static Tile* lever;
    static Tile* pressurePlate_stone;
    static Tile* door_iron;
    static Tile* pressurePlate_wood;
    static Tile* redStoneOre;
    static Tile* redStoneOre_lit;
    static Tile* notGate_off;
    static Tile* notGate_on;
    static Tile* button;
    static Tile* topSnow;
    static Tile* ice;
    static Tile* snow;
    static Tile* cactus;
    static Tile* clay;
    static Tile* reeds;
    static Tile* recordPlayer;
    static Tile* fence;
	static Tile* stairs_brick;
	static Tile* fenceGate;
    static Tile* pumpkin;
    static Tile* hellRock;
    static Tile* hellSand;
    static Tile* lightGem;
	static Tile* portalTile;
    static Tile* litPumpkin;
    static Tile* cake;
    static Tile* diode_off;
    static Tile* diode_on;
	static Tile* trapdoor;
	static Tile* stoneBrickSmooth;
	static Tile* grass_carried;
	static LeafTile* leaves_carried;
	static Tile* melon;
	static Tile* melonStem;
	static Tile* netherReactor;
	static Tile* glowingObsidian;

	static Tile* stairs_stoneBrickSmooth;
	static Tile* netherBrick;
	static Tile* netherrack;
	static Tile* stairs_netherBricks;
	static Tile* stairs_sandStone;
	static Tile* quartzBlock;
	static Tile* stairs_quartz;

	// invisible bedrock is used to block off empty chunks (i.e. prevent player movement)
	static Tile* invisible_bedrock;
	static Tile* info_updateGame1;
	static Tile* info_updateGame2;
	static Tile* info_reserved6;

	static void initTiles();
	static void teardownTiles();

	static int transformToValidBlockId(int blockId);
	static int transformToValidBlockId(int blockId, int x, int y, int z);

	Tile(int id, const Material* material);
    Tile(int id, int tex, const Material* material);
	virtual ~Tile() {}

    virtual bool isCubeShaped();
    virtual int getRenderShape();
    virtual void setShape(float x0, float y0, float z0, float x1, float y1, float z1);
	virtual void updateShape(LevelSource* level, int x, int y, int z) {}
	virtual void updateDefaultShape() {}

	virtual void addLights(Level* level, int x, int y, int z) {}

	virtual float getBrightness(LevelSource* level, int x, int y, int z);

    static bool isFaceVisible(Level* level, int x, int y, int z, int f);
    virtual bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face);

	virtual int getTexture(int face);
	virtual int getTexture(int face, int data);
    virtual int getTexture(LevelSource* level, int x, int y, int z, int face);

	// @attn Not threadsafe (ADDON: nor safe to _save_ this returned AABB*.
	// Make a copy if you need to save this AABB (rather then using as a temp)
    virtual AABB* getAABB(Level* level, int x, int y, int z);
	virtual void addAABBs(Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes);
	virtual AABB getTileAABB(Level* level, int x, int y, int z);

    virtual bool isSolidRender();

	virtual bool mayPick();
    virtual bool mayPick(int data, bool liquid);
	virtual bool mayPlace(Level* level, int x, int y, int z, unsigned char face);
	virtual bool mayPlace(Level* level, int x, int y, int z);

	virtual int getTickDelay();
    virtual void tick(Level* level, int x, int y, int z, Random* random) {}
    virtual void animateTick(Level* level, int x, int y, int z, Random* random) {}

    virtual void destroy(Level* level, int x, int y, int z, int data);

    virtual void neighborChanged(Level* level, int x, int y, int z, int type) {}

    virtual void onPlace(Level* level, int x, int y, int z) {}
    virtual void onRemove(Level* level, int x, int y, int z) {}

	virtual int getResource(int data, Random* random);
    virtual int getResourceCount(Random* random);

    virtual float getDestroyProgress(Player* player);

    virtual void spawnResources(Level* level, int x, int y, int z, int data);
    virtual void spawnResources(Level* level, int x, int y, int z, int data, float odds);
	virtual bool spawnBurnResources(Level* level, float x, float y, float z);
	void popResource(Level* level, int x, int y, int z, const ItemInstance& itemInstance);

    virtual float getExplosionResistance(Entity* source);

    virtual HitResult clip(Level* level, int xt, int yt, int zt, const Vec3& a, const Vec3& b);

	virtual void wasExploded(Level* level, int x, int y, int z) {}

    virtual int getRenderLayer();

    virtual bool use(Level* level, int x, int y, int z, Player* player);

    virtual void stepOn(Level* level, int x, int y, int z, Entity* entity) {}

	virtual void fallOn( Level* level, int x, int y, int z, Entity* entity, float fallDistance ) {}

	virtual int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue) { return itemValue; }
	virtual void setPlacedBy(Level* level, int x, int y, int z, Mob* by) {}

    virtual void prepareRender(Level* level, int x, int y, int z) {}

    virtual void attack(Level* level, int x, int y, int z, Player* player) {}

    virtual void handleEntityInside(Level* level, int x, int y, int z, Entity* e, Vec3& current) {}

    virtual int getColor(LevelSource* level, int x, int y, int z);

	virtual bool isSignalSource();
    virtual bool getSignal(LevelSource* level, int x, int y, int z);
    virtual bool getSignal(LevelSource* level, int x, int y, int z, int dir);
	virtual bool getDirectSignal(Level* level, int x, int y, int z, int dir);

    virtual void entityInside(Level* level, int x, int y, int z, Entity* entity) {}

    virtual void playerDestroy(Level* level, Player* player, int x, int y, int z, int data);

    virtual bool canSurvive(Level* level, int x, int y, int z);


    virtual std::string getName() const;
    virtual std::string getDescriptionId() const;
	virtual Tile* setDescriptionId(const std::string& id);

    virtual void triggerEvent(Level* level, int x, int y, int z, int b0, int b1) {}

protected:
    virtual Tile* setSoundType(const SoundType& soundType);

    virtual Tile* setLightBlock(int i);
    virtual Tile* setLightEmission(float f);

    virtual Tile* setExplodeable(float explosionResistance);
    virtual Tile* setDestroyTime(float destroySpeed);

    virtual void setTicking(bool tick);

    /*** Returns the item instance's auxValue when a TileItem is spawned from this Tile. */
    virtual int getSpawnResourcesAuxValue(int data);

private:
	Tile* init();
	Tile* setCategory(int category);

	bool containsX(const Vec3& v);
	bool containsY(const Vec3& v);
	bool containsZ(const Vec3& v);
public:
	int tex;
	const int id;

	float xx0, yy0, zz0, xx1, yy1, zz1;
	const SoundType* soundType;

	float gravity;
	const Material* const material;
	float friction;

//protected:
	float destroySpeed;
	float explosionResistance;

	int category;
protected:
	AABB tmpBB;

    static const int RENDERLAYER_OPAQUE;
    static const int RENDERLAYER_ALPHATEST;
    static const int RENDERLAYER_BLEND;
private:
	std::string descriptionId;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__Tile_H__*/
