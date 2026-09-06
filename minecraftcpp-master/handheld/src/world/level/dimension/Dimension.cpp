#include "Dimension.h"
#include "NormalDayCycleDimension.h"

//#include "../levelgen/SimpleLevelSource.h"
#include "../levelgen/RandomLevelSource.h"
#include "../Level.h"
#include "../biome/BiomeSource.h"
#include "../chunk/ChunkSource.h"
#include "../tile/Tile.h"
#include "../../../util/Mth.h"


Dimension::Dimension()
:	foggy(false),
	ultraWarm(false),
	hasCeiling(false),
	biomeSource(NULL),
	id(0)
{
}

Dimension::~Dimension()
{
	delete biomeSource;
}

void Dimension::init( Level* level )
{
	this->level = level;
	init();
	updateLightRamp();
}

void Dimension::init()
{
	biomeSource = new BiomeSource(level);
}

/*virtual*/
bool Dimension::isValidSpawn(int x, int z) {
    int topTile = level->getTopTile(x, z);

	if (topTile == Tile::invisible_bedrock->id)
		return false;

    //if (topTile != Tile::sand->id) return false;
	if (!Tile::tiles[topTile]->isSolidRender()) return false;

    return true;
}

float Dimension::getTimeOfDay(long time, float a) {
	return 1;
}

ChunkSource* Dimension::createRandomLevelSource() {
	return new RandomLevelSource(
		level,
		level->getSeed(),
		level->getLevelData()->getGeneratorVersion(),
		!level->isClientSide && level->getLevelData()->getSpawnMobs());
	//return new PerformanceTestChunkSource(level);
}


void Dimension::updateLightRamp()
{
	float ambientLight = 0.05f;
	for (int i = 0; i <= 15; /*Level::MAX_BRIGHTNESS;*/ i++) {
		float v = (1 - i / (float) (16 /*Level::MAX_BRIGHTNESS*/));
		// Boosted ambient lightning by ten times.
		brightnessRamp[i] = ((1 - v) / (v * 3 + 1)) * (1 - ambientLight) + ambientLight * 3;
	}
}

float* Dimension::getSunriseColor( float td, float a )
{
	float span = 0.4f;
	float tt = Mth::cos(td * Mth::PI * 2) - 0.0f;
	float mid = -0.0f;
	if (tt >= mid - span && tt <= mid + span) {
		float aa = ((tt - mid) / span) * 0.5f + 0.5f;
		float mix = 1 - (((1 - Mth::sin(aa * Mth::PI))) * 0.99f);
		mix = mix * mix;
		sunriseCol[0] = (aa * 0.3f + 0.7f);
		sunriseCol[1] = (aa * aa * 0.7f + 0.2f);
		sunriseCol[2] = (aa * aa * 0.0f + 0.2f);
		sunriseCol[3] = mix;
		return sunriseCol;
	}
	return NULL;
}

Vec3 Dimension::getFogColor( float td, float a )
{
	float br = Mth::cos(td * Mth::PI * 2) * 2 + 0.5f;
	if (br < 0.0f) br = 0.0f;
	if (br > 1.0f) br = 1.0f;

	float r = ((fogColor >> 16) & 0xff) / 255.0f;
	float g = ((fogColor >> 8) & 0xff) / 255.0f;
	float b = ((fogColor) & 0xff) / 255.0f;
	r *= br * 0.94f + 0.06f;
	g *= br * 0.94f + 0.06f;
	b *= br * 0.91f + 0.09f;
	return Vec3(r, g, b);
	//return Vec3(0.752941f, 0.847059f, 1);
}

bool Dimension::mayRespawn()
{
	return true;
}

Dimension* Dimension::getNew( int id )
{
	if (id == NORMAL) return new Dimension();
	if (id == NORMAL_DAYCYCLE) return new NormalDayCycleDimension();
	return NULL;
}

//
// DimensionFactory
//
#include "../storage/LevelData.h"
Dimension* DimensionFactory::createDefaultDimension(LevelData* data )
{
	int dimensionId = Dimension::NORMAL;

	switch(data->getGameType()) {
	case GameType::Survival: dimensionId = Dimension::NORMAL_DAYCYCLE;
		break;
	case GameType::Creative:
	default:
		dimensionId = Dimension::NORMAL;
		break;
	}

	return Dimension::getNew(dimensionId);
}
