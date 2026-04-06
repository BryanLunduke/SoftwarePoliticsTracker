#include "RandomLevelSource.h"

#include "feature/FeatureInclude.h"
#include "../Level.h"
#include "../ChunkPos.h"
#include "../MobSpawner.h"
#include "../biome/Biome.h"
#include "../biome/BiomeSource.h"
#include "../chunk/LevelChunk.h"
#include "../material/Material.h"
#include "../tile/Tile.h"
#include "../tile/HeavyTile.h"
#include "../../../util/Random.h"

const float RandomLevelSource::SNOW_CUTOFF = 0.5f;
const float RandomLevelSource::SNOW_SCALE = 0.3f;
static const int MAX_BUFFER_SIZE = 1024;

RandomLevelSource::RandomLevelSource(Level* level, long seed, int version, bool spawnMobs)
:	random(seed),
	level(level),
	lperlinNoise1(&random, 16),
	lperlinNoise2(&random, 16),
	perlinNoise1(&random, 8),
	perlinNoise2(&random, 4),
	perlinNoise3(&random, 4),
	scaleNoise(&random, 10),
	depthNoise(&random, 16),
	forestNoise(&random, 8),
	spawnMobs(spawnMobs),
	pnr(NULL), ar(NULL), br(NULL), sr(NULL), dr(NULL), fi(NULL), fis(NULL)
	//biomes(NULL)
{
	for (int i=0; i<32; ++i)
	for (int j=0; j<32; ++j)
		waterDepths[i][j] = 0;

	buffer = new float[MAX_BUFFER_SIZE];

	Random randomCopy = random;
	printf("random.get : %d\n", randomCopy.nextInt());
}

RandomLevelSource::~RandomLevelSource() {

	// chunks are deleted in the chunk cache instead
	//ChunkMap::iterator it = chunkMap.begin();
	//while (it != chunkMap.end()) {
	//	it->second->deleteBlockData(); //@attn: we delete the block data here, for now
	//	delete it->second;
	//	++it;
	//}

	delete[] buffer;
	delete[] pnr;
	delete[] ar;
	delete[] br;
	delete[] sr;
	delete[] dr;
	delete[] fi;
	delete[] fis;
}

/*public*/
void RandomLevelSource::prepareHeights(int xOffs, int zOffs, unsigned char* blocks, /*Biome*/void* biomes, float* temperatures) {
	
	int xChunks = 16 / CHUNK_WIDTH;
    int waterHeight = Level::DEPTH - 64;

    int xSize = xChunks + 1;
    int ySize = 128 / CHUNK_HEIGHT + 1;
    int zSize = xChunks + 1;
    buffer = getHeights(buffer, xOffs * xChunks, 0, zOffs * xChunks, xSize, ySize, zSize);

    for (int xc = 0; xc < xChunks; xc++) {
        for (int zc = 0; zc < xChunks; zc++) {
            for (int yc = 0; yc < 128 / CHUNK_HEIGHT; yc++) {
                float yStep = 1 / (float) CHUNK_HEIGHT;
                float s0 = buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s1 = buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 0)];
                float s2 = buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s3 = buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 0)];

                float s0a = (buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 1)] - s0) * yStep;
                float s1a = (buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 1)] - s1) * yStep;
                float s2a = (buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 1)] - s2) * yStep;
                float s3a = (buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 1)] - s3) * yStep;

                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    float xStep = 1 / (float) CHUNK_WIDTH;

                    float _s0 = s0;
                    float _s1 = s1;
                    float _s0a = (s2 - s0) * xStep;
                    float _s1a = (s3 - s1) * xStep;

                    for (int x = 0; x < CHUNK_WIDTH; x++) {
                        int offs = (x + xc * CHUNK_WIDTH) << 11 | (0 + zc * CHUNK_WIDTH) << 7 | (yc * CHUNK_HEIGHT + y);
                        int step = 1 << 7;
                        float zStep = 1 / (float) CHUNK_WIDTH;

                        float val = _s0;
                        float vala = (_s1 - _s0) * zStep;
                        for (int z = 0; z < CHUNK_WIDTH; z++) {
// + (zc * CHUNK_WIDTH + z)];
                            float temp = temperatures[(xc * CHUNK_WIDTH + x) * 16 + (zc * CHUNK_WIDTH + z)];
                            int tileId = 0;
                            if (yc * CHUNK_HEIGHT + y < waterHeight) {
                                if (temp < SNOW_CUTOFF && yc * CHUNK_HEIGHT + y >= waterHeight - 1) {
                                    tileId = Tile::ice->id;
                                } else {
                                    tileId = Tile::calmWater->id;
                                }
                            }
                            if (val > 0) {
                                tileId = Tile::rock->id;
                            } else {
                            }

                            blocks[offs] = (unsigned char) tileId;
                            offs += step;
                            val += vala;
                        }
                        _s0 += _s0a;
                        _s1 += _s1a;
                    }

                    s0 += s0a;
                    s1 += s1a;
                    s2 += s2a;
                    s3 += s3a;
                }
            }
        }
    }
}

void RandomLevelSource::buildSurfaces(int xOffs, int zOffs, unsigned char* blocks, Biome** biomes) {
    int waterHeight = Level::DEPTH - 64;

    float s = 1 / 32.0f;
    perlinNoise2.getRegion(sandBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s, s, 1);
    perlinNoise2.getRegion(gravelBuffer, (float)(xOffs * 16), 109.01340f, (float)(zOffs * 16), 16, 1, 16, s, 1, s);
    perlinNoise3.getRegion(depthBuffer, (float)(xOffs * 16), (float)(zOffs * 16), 0, 16, 16, 1, s * 2, s * 2, s * 2);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
			float temp = 1; // @todo: read temp from BiomeSource
            Biome* b = biomes[x + z * 16];
            bool sand = (sandBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 0;
            bool gravel = (gravelBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 3;
            int runDepth = (int) (depthBuffer[x + z * 16] / 3 + 3 + random.nextFloat() * 0.25f);

            int run = -1;

			char top = b->topMaterial;
            char material = b->material;

            for (int y = 127; y >= 0; y--) {
                int offs = (z * 16 + x) * 128 + y;

                if (y <= 0 + random.nextInt(5)) {
                    blocks[offs] = (char) Tile::unbreakable->id;
                } else {
                    int old = blocks[offs];

                    if (old == 0) {
                        run = -1;
                    } else if (old == Tile::rock->id) {
                        if (run == -1) {
                            if (runDepth <= 0) {
                                top = 0;
                                material = (char) Tile::rock->id;
                            } else if (y >= waterHeight - 4 && y <= waterHeight + 1) {
                                top = b->topMaterial;
								material = b->material;
								
								//@attn: ?
                                if (gravel) {
									top = 0;
									material = (char) Tile::gravel->id;
								}
                                if (sand) {
									top = (char) Tile::sand->id;
									material = (char) Tile::sand->id;
								}
                            }

                            if (y < waterHeight && top == 0) {
								if (temp < 0.15f)
									top = (char) Tile::ice->id;
								else
									top = (char) Tile::calmWater->id;
							}

                            run = runDepth;
                            if (y >= waterHeight - 1) blocks[offs] = top;
                            else blocks[offs] = material;
                        } else if (run > 0) {
                            run--;
                            blocks[offs] = material;

                            // place a few sandstone blocks beneath sand
                            // runs
                            if (run == 0 && material == Tile::sand->id) {
                                run = random.nextInt(4);
                                material = (char) Tile::sandStone->id;
                            }
                        }
                    }
                }
            }
        }
    }
}


/*public*/
void RandomLevelSource::postProcess(ChunkSource* parent, int xt, int zt) {

	level->isGeneratingTerrain = true;

    HeavyTile::instaFall = true;
    int xo = xt * 16;
    int zo = zt * 16;

    Biome* biome = level->getBiomeSource()->getBiome(xo + 16, zo + 16);
    //    Biome* biome = Biome::forest;

    random.setSeed(level->getSeed());
    int xScale = random.nextInt() / 2 * 2 + 1;
    int zScale = random.nextInt() / 2 * 2 + 1;
    random.setSeed(((xt * xScale) + (zt * zScale)) ^ level->getSeed());

	// //@todo: hide those chunks if they are aren't visible
//    if (random.nextInt(4) == 0) {
//        int x = xo + random.nextInt(16) + 8;
//        int y = random.nextInt(128);
//        int z = zo + random.nextInt(16) + 8;
//        LakeFeature feature(Tile::calmWater->id);
//		feature.place(level, &random, x, y, z);
//        LOGI("Adding underground lake @ (%d,%d,%d)\n", x, y, z);
//    }

	////@todo: hide those chunks if they are aren't visible
 //   if (random.nextInt(8) == 0) {
 //       int x = xo + random.nextInt(16) + 8;
 //       int y = random.nextInt(random.nextInt(120) + 8);
 //       int z = zo + random.nextInt(16) + 8;
 //       if (y < 64 || random.nextInt(10) == 0) {
	//		LakeFeature feature(Tile::calmLava->id);
	//		feature.place(level, &random, x, y, z);
	//	}
 //   }

	static float totalTime = 0;
	const float st = getTimeS();

    //for (int i = 0; i < 8; i++) {
    //    int x = xo + random.nextInt(16) + 8;
    //    int y = random.nextInt(128);
    //    int z = zo + random.nextInt(16) + 8;
    //    MonsterRoomFeature().place(level, random, x, y, z);
    //}

    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        ClayFeature feature(32);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 20; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::dirt->id, 32);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::gravel->id, 32);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 20; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::coalOre->id, 16);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 20; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(64);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::ironOre->id, 8);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 2; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(32);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::goldOre->id, 8);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 8; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(16);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::redStoneOre->id, 7);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 1; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(16);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::emeraldOre->id, 7);
		feature.place(level, &random, x, y, z);
    }

    // lapis ore
    for (int i = 0; i < 1; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(16) + random.nextInt(16);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::lapisOre->id, 6);
		feature.place(level, &random, x, y, z);
    }

    const float ss = 0.5f;
    int oFor = (int) ((forestNoise.getValue(xo * ss, zo * ss) / 8 + random.nextFloat() * 4 + 4) / 3);
    int forests = 0;//1; (java: 0)
    if (random.nextInt(10) == 0) forests += 1;

    if (biome == Biome::forest) forests += oFor + 2; // + 5
    if (biome == Biome::rainForest) forests += oFor + 2; //+ 5
    if (biome == Biome::seasonalForest) forests += oFor + 1; // 2
    if (biome == Biome::taiga) {
		forests += oFor + 1; // + 5
		//LOGI("Biome is taiga!\n");
	}

    if (biome == Biome::desert) forests -= 20;
    if (biome == Biome::tundra) forests -= 20;
    if (biome == Biome::plains) forests -= 20;

    for (int i = 0; i < forests; i++) {
        int x = xo + random.nextInt(16) + 8;
        int z = zo + random.nextInt(16) + 8;
		int y = level->getHeightmap(x, z);
        Feature* tree = biome->getTreeFeature(&random);
		if (tree) {
	        tree->init(1, 1, 1);
		    tree->place(level, &random, x, y, z);
			delete tree;
		}
		//printf("placing tree at %d, %d, %d\n", x, y, z);
    }

    for (int i = 0; i < 2; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::flower->id);
		feature.place(level, &random, x, y, z);
    }

    if (random.nextInt(2) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
		FlowerFeature feature(Tile::rose->id);
        feature.place(level, &random, x, y, z);
    }

    if (random.nextInt(4) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::mushroom1->id);
		feature.place(level, &random, x, y, z);
    }

    if (random.nextInt(8) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::mushroom2->id);
		feature.place(level, &random, x, y, z);
    }
	/*int grassCount = 1;
	for (int i = 0; i < grassCount; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(Level::genDepth);
		int z = zo + random.nextInt(16) + 8;
		Feature* grassFeature = biome->getGrassFeature(&random);
		if (grassFeature) {
			grassFeature->place(level, &random, x, y, z);
			delete grassFeature;
		}
	}*/
    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        ReedsFeature feature;
		feature.place(level, &random, x, y, z);
    }
	

    //if (random.nextInt(32) == 0) {
    //    int x = xo + random.nextInt(16) + 8;
    //    int y = random.nextInt(128);
    //    int z = zo + random.nextInt(16) + 8;
    //    PumpkinFeature().place(level, random, x, y, z);
    //}

    int cacti = 0;
    if (biome == Biome::desert) cacti += 5;

    for (int i = 0; i < cacti; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        CactusFeature feature;
        //LOGI("Tried creating a cactus at %d, %d, %d\n", x, y, z);
        feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 50; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(random.nextInt(120) + 8);
        int z = zo + random.nextInt(16) + 8;
        SpringFeature feature(Tile::water->id);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 20; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(random.nextInt(random.nextInt(112) + 8) + 8);
        int z = zo + random.nextInt(16) + 8;
        SpringFeature feature(Tile::lava->id);
		feature.place(level, &random, x, y, z);
    }

	if (spawnMobs && !level->isClientSide)
		MobSpawner::postProcessSpawnMobs(level, biome, xo + 8, zo + 8, 16, 16, &random);

	//LOGI("Reading temp: 1\n");
    float* temperatures = level->getBiomeSource()->getTemperatureBlock(/*NULL,*/ xo + 8, zo + 8, 16, 16);
    for (int x = xo + 8; x < xo + 8 + 16; x++)
        for (int z = zo + 8; z < zo + 8 + 16; z++) {
            int xp = x - (xo + 8);
            int zp = z - (zo + 8); 
            int y = level->getTopSolidBlock(x, z);
            float temp = temperatures[xp * 16 + zp] - (y - 64) / 64.0f * SNOW_SCALE;
            if (temp < SNOW_CUTOFF) {
                if (y > 0 && y < 128 && level->isEmptyTile(x, y, z) && level->getMaterial(x, y - 1, z)->blocksMotion()) {
                    if (level->getMaterial(x, y - 1, z) != Material::ice) level->setTile(x, y, z, Tile::topSnow->id);
                }
            }
        }
	//LOGI("Reading temp: 0 END\n");

	const float et = getTimeS();
	totalTime += (et-st);

	//printf("Time to place features: %f. Total %f\n", et - st, totalTime);

    HeavyTile::instaFall = false;

	level->isGeneratingTerrain = false;
}

LevelChunk* RandomLevelSource::create(int x, int z) {
    return getChunk(x, z);
}

LevelChunk* RandomLevelSource::getChunk(int xOffs, int zOffs) {
	//static int chunkx = 0;
	int hashedPos = ChunkPos::hashCode(xOffs, zOffs);

	ChunkMap::iterator it = chunkMap.find(hashedPos);
	if (it != chunkMap.end())
		return it->second;

    random.setSeed((long)(xOffs * 341872712l + zOffs * 132899541l)); //@fix

    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    LevelChunk* levelChunk = new LevelChunk(level, blocks, xOffs, zOffs);
	chunkMap.insert(std::make_pair(hashedPos, levelChunk));

	Biome** biomes = level->getBiomeSource()->getBiomeBlock(/*biomes, */xOffs * 16, zOffs * 16, 16, 16);
    float* temperatures = level->getBiomeSource()->temperatures;
    prepareHeights(xOffs, zOffs, blocks, 0, temperatures);//biomes, temperatures);
    buildSurfaces(xOffs, zOffs, blocks, biomes);

	//caveFeature.apply(this, level, xOffs, zOffs, blocks, LevelChunk::ChunkBlockCount);
    levelChunk->recalcHeightmap();

    return levelChunk;
}

/*private*/
float* RandomLevelSource::getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize) {
	const int size = xSize * ySize * zSize;
	if (size > MAX_BUFFER_SIZE) {
		LOGI("RandomLevelSource::getHeights: TOO LARGE BUFFER REQUESTED: %d (max %d)\n", size, MAX_BUFFER_SIZE);
	}

    float s = 1 * 684.412f;
    float hs = 1 * 684.412f;

    float* temperatures = level->getBiomeSource()->temperatures;
    float* downfalls = level->getBiomeSource()->downfalls;
    sr = scaleNoise.getRegion(sr, x, z, xSize, zSize, 1.121f, 1.121f, 0.5f);
    dr = depthNoise.getRegion(dr, x, z, xSize, zSize, 200.0f, 200.0f, 0.5f);

    pnr = perlinNoise1.getRegion(pnr, (float)x, (float)y, (float)z, xSize, ySize, zSize, s / 80.0f, hs / 160.0f, s / 80.0f);
    ar = lperlinNoise1.getRegion(ar, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);
    br = lperlinNoise2.getRegion(br, (float)x, (float)y, (float)z, xSize, ySize, zSize, s, hs, s);

    int p = 0;
    int pp = 0;

    int wScale = 16 / xSize;
    for (int xx = 0; xx < xSize; xx++) {
        int xp = xx * wScale + wScale / 2;

        for (int zz = 0; zz < zSize; zz++) {
            int zp = zz * wScale + wScale / 2;
            float temperature = temperatures[xp * 16 + zp];
            float downfall = downfalls[xp * 16 + zp] * temperature;
            float dd = 1 - downfall;
            dd = dd * dd;
            dd = dd * dd;
            dd = 1 - dd;

            float scale = ((sr[pp] + 256.0f) / 512);
            scale *= dd;
            if (scale > 1) scale = 1;


            float depth = (dr[pp] / 8000.0f);
            if (depth < 0) depth = -depth * 0.3f;
            depth = depth * 3.0f - 2.0f;

            if (depth < 0) {
                depth = depth / 2;
                if (depth < -1) depth = -1;
                depth = depth / 1.4f;
                depth /= 2;
                scale = 0;
            } else {
                if (depth > 1) depth = 1;
                depth = depth / 8;
            }

            if (scale < 0) scale = 0;
            scale = (scale) + 0.5f;
            depth = depth * ySize / 16;

            float yCenter = ySize / 2.0f + depth * 4;

            pp++;

            for (int yy = 0; yy < ySize; yy++) {
                float val = 0;

                float yOffs = (yy - (yCenter)) * 12 / scale;
                if (yOffs < 0) yOffs *= 4;

                float bb = ar[p] / 512;
                float cc = br[p] / 512;

                float v = (pnr[p] / 10 + 1) / 2;
                if (v < 0) val = bb;
                else if (v > 1) val = cc;
                else val = bb + (cc - bb) * v;
                val -= yOffs;

                if (yy > ySize - 4) {
                    float slide = (yy - (ySize - 4)) / (4 - 1.0f);
                    val = val * (1 - slide) + -10 * slide;
                }

                buffer[p] = val;
                p++;
            }
        }
    }
    return buffer;
}

/*private*/
void RandomLevelSource::calcWaterDepths(ChunkSource* parent, int xt, int zt) {
    int xo = xt * 16;
    int zo = zt * 16;
    for (int x = 0; x < 16; x++) {
        int y = level->getSeaLevel();
        for (int z = 0; z < 16; z++) {
            int xp = xo + x + 7;
            int zp = zo + z + 7;
            int h = level->getHeightmap(xp, zp);
            if (h <= 0) {
                if (level->getHeightmap(xp - 1, zp) > 0 || level->getHeightmap(xp + 1, zp) > 0 || level->getHeightmap(xp, zp - 1) > 0 || level->getHeightmap(xp, zp + 1) > 0) {
                    bool hadWater = false;
                    if (hadWater || (level->getTile(xp - 1, y, zp) == Tile::calmWater->id && level->getData(xp - 1, y, zp) < 7)) hadWater = true;
                    if (hadWater || (level->getTile(xp + 1, y, zp) == Tile::calmWater->id && level->getData(xp + 1, y, zp) < 7)) hadWater = true;
                    if (hadWater || (level->getTile(xp, y, zp - 1) == Tile::calmWater->id && level->getData(xp, y, zp - 1) < 7)) hadWater = true;
                    if (hadWater || (level->getTile(xp, y, zp + 1) == Tile::calmWater->id && level->getData(xp, y, zp + 1) < 7)) hadWater = true;
                    if (hadWater) {
                        for (int x2 = -5; x2 <= 5; x2++) {
                            for (int z2 = -5; z2 <= 5; z2++) {
                                int d = (x2 > 0 ? x2 : -x2) + (z2 > 0 ? z2 : -z2);

                                if (d <= 5) {
                                    d = 6 - d;
                                    if (level->getTile(xp + x2, y, zp + z2) == Tile::calmWater->id) {
                                        int od = level->getData(xp + x2, y, zp + z2);
                                        if (od < 7 && od < d) {
                                            level->setData(xp + x2, y, zp + z2, d);
                                        }
                                    }
                                }
                            }
                        }
                        if (hadWater) {
                            level->setTileAndDataNoUpdate(xp, y, zp, Tile::calmWater->id, 7);
                            for (int y2 = 0; y2 < y; y2++) {
                                level->setTileAndDataNoUpdate(xp, y2, zp, Tile::calmWater->id, 8);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool RandomLevelSource::hasChunk(int x, int y) {
    //return x >= 0 && x < 16 && y >= 0 && y < 16;
	return true;
}

bool RandomLevelSource::tick() {
	return false;
}

bool RandomLevelSource::shouldSave() {
	return true;
}

std::string RandomLevelSource::gatherStats() {
	return "RandomLevelSource";
}

//bool RandomLevelSource::save(bool force, ProgressListener progressListener) {
//    return true;
//}

Biome::MobList RandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
    BiomeSource* biomeSource = level->getBiomeSource();
    if (biomeSource == NULL) {
        return Biome::MobList();
    }
//    static Stopwatch sw; sw.start();
    Biome* biome = biomeSource->getBiome(x, z);
//    sw.stop();
//    sw.printEvery(10, "getBiome::");
    if (biome == NULL) {
        return Biome::MobList();
    }
    return biome->getMobs(mobCategory);
}


LevelChunk* PerformanceTestChunkSource::create(int x, int z)
{
	unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
	memset(blocks, 0, LevelChunk::ChunkBlockCount);

	for (int y = 0; y < 65; y++)
	{
		if (y < 60)
		{
			for (int x = (y + 1) & 1; x < 16; x += 2)
			{
				for (int z = y & 1; z < 16; z += 2)
				{
					blocks[x << 11 | z << 7 | y] = 3;
				}
			}
		}
		else
		{
			for (int x = 0; x < 16; x += 2)
			{
				for (int z = 0; z < 16; z += 2)
				{
					blocks[x << 11 | z << 7 | y] = 3;
				}
			}

		}
	}

	LevelChunk* levelChunk = new LevelChunk(level, blocks, x, z);

	//caveFeature.apply(this, level, xOffs, zOffs, blocks, LevelChunk::ChunkBlockCount);
	levelChunk->recalcHeightmap();

	return levelChunk;
}
