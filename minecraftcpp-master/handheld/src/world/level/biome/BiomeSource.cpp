#include "BiomeSource.h"
#include "Biome.h"
#include "../Level.h"
#include "../ChunkPos.h"

const float BiomeSource::zoom = 2 * 1;

const float BiomeSource::tempScale = zoom / 80.0f;
const float BiomeSource::downfallScale = zoom / 40.0f;
const float BiomeSource::noiseScale = 1 / 4.0f;

BiomeSource::BiomeSource()
:	temperatureMap(NULL),
	downfallMap(NULL),
	noiseMap(NULL),
	lenTemperatures(0),
	lenDownfalls(0),
	lenNoises(0),
	lenBiomes(0),
	temperatures(NULL),
	downfalls(NULL),
	noises(NULL),
	biomes(NULL)
{
	biomes = new Biome*[16*16];
}

BiomeSource::BiomeSource( Level* level )
:	rndTemperature(level->getSeed() * 9871),
	rndDownfall(level->getSeed() * 39811),
	rndNoise(level->getSeed() * 543321),
	lenTemperatures(0),
	lenDownfalls(0),
	lenNoises(0),
	lenBiomes(0),
	temperatures(NULL),
	downfalls(NULL),
	noises(NULL),
	biomes(NULL)
{
	temperatureMap = new PerlinSimplexNoise(&rndTemperature, 4);
	downfallMap = new PerlinSimplexNoise(&rndDownfall, 4);
	noiseMap = new PerlinSimplexNoise(&rndNoise, 2);

	biomes = new Biome*[16*16];
	temperatures = new float[16*16];
}

BiomeSource::~BiomeSource() {
	LOGI("Deleting biome maps...\n");
	delete temperatureMap;
	delete downfallMap;
	delete noiseMap;

	LOGI("Deleting biome data arrays...\n");
	delete[] temperatures;
	delete[] downfalls;
	delete[] noises;
	delete[] biomes;
}

Biome* BiomeSource::getBiome( const ChunkPos& chunk )
{
	return getBiome(chunk.x << 4, chunk.z << 4);
}

Biome* BiomeSource::getBiome( int x, int z )
{
	return getBiomeBlock(x, z, 1, 1)[0];
}

//float BiomeSource::getTemperature( int x, int z )
//{
//	temperatures = temperatureMap->getRegion(temperatures, x, z, 1, 1, tempScale, tempScale, 0.5f);
//	return temperatures[0];
//}

Biome** BiomeSource::getBiomeBlock( int x, int z, int w, int h )
{
    biomes = getBiomeBlock(biomes, x, z, w, h);
    return biomes;
}

Biome** BiomeSource::getBiomeBlock( Biome** biomes__, int x, int z, int w, int h )
{
	//for (int i = 0; i < w*h; ++i) biomes__[i] = Biome::tundra;
	//const int size = w * h;
	//if (lenBiomes < size) {
	//	//printf("changing to size: %d (was %d). %d, %d (%d, %d)\n", size, lenBiomes, x, z, w, h);
	//	if (biomes) delete[] biomes;
	//	biomes = new Biome*[size];
	//	lenBiomes = size;
	//}

	temperatures = temperatureMap->getRegion(temperatures, x, z, w, w, tempScale, tempScale, 0.25f);
	downfalls = downfallMap->getRegion(downfalls, x, z, w, w, downfallScale, downfallScale, 0.3333f);
	noises = noiseMap->getRegion(noises, x, z, w, w, noiseScale, noiseScale, 0.588f);

	int pp = 0;
	for (int yy = 0; yy < w; yy++) {
		for (int xx = 0; xx < h; xx++) {
			float noise = (noises[pp] * 1.1f + 0.5f);

			float split2 = 0.01f;
			float split1 = 1 - split2;
			float temperature = (temperatures[pp] * 0.15f + 0.7f) * split1 + noise * split2;
			split2 = 0.002f;
			split1 = 1 - split2;
			float downfall = (downfalls[pp] * 0.15f + 0.5f) * split1 + noise * split2;
			temperature = 1 - ((1 - temperature) * (1 - temperature));
			if (temperature < 0) temperature = 0;
			if (downfall < 0) downfall = 0;
			if (temperature > 1) temperature = 1;
			if (downfall > 1) downfall = 1;

			temperatures[pp] = temperature;
			downfalls[pp] = downfall;
			// System.out.println(downfall);
			biomes[pp++] = Biome::getBiome(temperature, downfall);
		}
	}

	return biomes;
}

float* BiomeSource::getTemperatureBlock( /*float* temperatures__, */int x, int z, int w, int h )
{
	//LOGI("gTempBlock: 1\n");
	//const int size = w * h;
	//if (lenTemperatures < size) {
	//	if (temperatures) delete[] temperatures;
	//	temperatures = new float[size];
	//	lenTemperatures = size;
	//}

	float * ot = temperatures;
	temperatures = temperatureMap->getRegion(temperatures, x, z, w, h, tempScale, tempScale, 0.25f);
	noises = noiseMap->getRegion(noises, x, z, w, h, noiseScale, noiseScale, 0.588f);

	if (ot != temperatures) {
		LOGI("tmp ptr changed\n");
	}

	int pp = 0;
	for (int yy = 0; yy < w; yy++) {
		for (int xx = 0; xx < h; xx++) {
			float noise = (noises[pp] * 1.1f + 0.5f);

			float split2 = 0.01f;
			float split1 = 1 - split2;
			float temperature = (temperatures[pp] * 0.15f + 0.7f) * split1 + noise * split2;
			temperature = 1 - ((1 - temperature) * (1 - temperature));

			if (temperature < 0) temperature = 0;
			if (temperature > 1) temperature = 1;

			temperatures[pp] = temperature;
			pp++;
		}
	}

	// System.out.println(min+", "+max);

	return temperatures;
}

//float* BiomeSource::getDownfallBlock( /*float* downfalls__,*/ int x, int z, int w, int h )
//{
//	//const int size = w * h;
//	//if (lenDownfalls < size) {
//	//	delete[] downfalls;
//	//	downfalls = new float[size];
//	//	lenDownfalls = size;
//	//}
//
//	downfalls = downfallMap->getRegion(downfalls, x, z, w, w, downfallScale, downfallScale, 0.5f);
//	return downfalls;
//}
