#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK__LightLayer_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK__LightLayer_H__

//package net.minecraft.world.level;

class LightLayer
{
public:
    static const LightLayer Sky; // (15)
	static const LightLayer Block;// (0)

	const int surrounding;

private:
    LightLayer(int surrounding_)
	:	surrounding(surrounding_)
	{}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_CHUNK__LightLayer_H__*/
