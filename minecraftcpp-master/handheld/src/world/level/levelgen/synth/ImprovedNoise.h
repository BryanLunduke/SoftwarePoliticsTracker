#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__

//package net.minecraft.world.level.levelgen.synth;

#include "Synth.h"
class Random;

class ImprovedNoise: public Synth
{
public:
    ImprovedNoise();

    ImprovedNoise(Random* random);
	
	void init(Random* random);

    float noise(float _x, float _y, float _z);

    const float lerp(float t, float a, float b);

    const float grad2(int hash, float x, float z);
    const float grad(int hash, float x, float y, float z);

    float getValue(float x, float y);
    float getValue(float x, float y, float z);

    void add(float* buffer, float _x, float _y, float _z, int xSize, int ySize, int zSize, float xs, float ys, float zs, float pow);

    int hashCode();

	float scale;
	float xo, yo, zo;
private:
	int p[512];
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__*/
