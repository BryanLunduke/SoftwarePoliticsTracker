#include "PerlinNoise.h"
#include "ImprovedNoise.h"

void PerlinNoise::init( int levels )
{
	this->levels = levels;
	noiseLevels = new ImprovedNoise* [levels];
	for (int i = 0; i < levels; i++) {
		noiseLevels[i] = new ImprovedNoise(_rndPtr);
	}
}

PerlinNoise::~PerlinNoise()
{
	for (int i = 0; i < levels; ++i)
		delete noiseLevels[i];
	delete[] noiseLevels;
}

PerlinNoise::PerlinNoise( int levels )
{
	_rndPtr = &_random;
	init(levels);
}

PerlinNoise::PerlinNoise( Random* random, int levels )
{
	_rndPtr = random;
	init(levels);
}

float PerlinNoise::getValue( float x, float y )
{
	float value = 0;
	float pow = 1;

	for (int i = 0; i < levels; i++) {
		value += noiseLevels[i]->getValue(x * pow, y * pow) / pow;
		pow /= 2;
	}

	return value;
}

float PerlinNoise::getValue( float x, float y, float z )
{
	float value = 0;
	float pow = 1;

	for (int i = 0; i < levels; i++) {
		value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
		pow /= 2;
	}

	return value;
}

float* PerlinNoise::getRegion( float* buffer, float x, float y, float z, int xSize, int ySize, int zSize, float xScale, float yScale, float zScale )
{
	const int size = xSize * ySize * zSize;
	if (buffer == 0) {
		buffer = new float[size];
	}
	for (int i = 0; i < size; i++)
		buffer[i] = 0;

	float pow = 1;

	for (int i = 0; i < levels; i++) {
		noiseLevels[i]->add(buffer, x, y, z, xSize, ySize, zSize, xScale * pow, yScale * pow, zScale * pow, pow);
		pow /= 2;
	}

	return buffer;
}

float* PerlinNoise::getRegion( float* sr, int x, int z, int xSize, int zSize, float xScale, float zScale, float pow )
{
	return getRegion(sr, (float)x, 10.0f, (float)z, xSize, 1, zSize, xScale, 1, zScale);
}

int PerlinNoise::hashCode() {
    int x = 4711;
    for (int i = 0; i < levels; ++i)
        x *= noiseLevels[i]->hashCode();
    return x;
}

