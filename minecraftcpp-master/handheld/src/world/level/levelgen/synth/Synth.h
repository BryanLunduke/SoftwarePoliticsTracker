#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__Synth_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__Synth_H__

//package net.minecraft.world.level.levelgen.synth;

class Synth
{
public:
	virtual ~Synth();

	int getDataSize(int width, int height);

	virtual float getValue(float x, float y) = 0;

    void create(int width, int height, float* result);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__Synth_H__*/
