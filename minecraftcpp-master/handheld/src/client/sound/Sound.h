#ifndef NET_MINECRAFT_CLIENT_SOUND__Sound_H__
#define NET_MINECRAFT_CLIENT_SOUND__Sound_H__

//package net.minecraft.client.sound;

#include <string>

class SoundDesc
{
public:
	SoundDesc()
	:	buffer(0)
	{}

    SoundDesc(char* data, int size, int channels, int width, int rate)
    :   buffer(data),
        frames(data),
        size(size),
        channels(channels),
        byteWidth(width),
        frameRate(rate)
    {
        numFrames = size / (channels * byteWidth);
    }

	SoundDesc(char* data)
	:	buffer(data)
	{
		// header [INT][Channels, BytePerSample, FrameRate, NumFrames]
		channels  = *((int*)&data[0]);
		byteWidth = *((int*)&data[4]);
		frameRate = *((int*)&data[8]);
		numFrames = *((int*)&data[12]);

		size = channels * byteWidth * numFrames;
		frames = buffer + 16;
	}

	bool isValid() const { return buffer != 0; }
    float length() const {
        return ((float)numFrames) / frameRate;
    }

    void destroy() const {
        if (isValid()) {
            delete buffer;
            buffer = 0;
        }
    }
    
	char* frames;
    int size;

	int channels;
	int byteWidth;
	int frameRate;
	int numFrames;

    std::string name;
private:
	mutable char* buffer;
};

#if !defined(PRE_ANDROID23) && !defined(__APPLE__) && !defined(RPI)

extern SoundDesc SA_cloth1;
extern SoundDesc SA_cloth2;
extern SoundDesc SA_cloth3;
extern SoundDesc SA_cloth4;
extern SoundDesc SA_grass1;
extern SoundDesc SA_grass2;
extern SoundDesc SA_grass3;
extern SoundDesc SA_grass4;
extern SoundDesc SA_gravel1;
extern SoundDesc SA_gravel2;
extern SoundDesc SA_gravel3;
extern SoundDesc SA_gravel4;
extern SoundDesc SA_sand1;
extern SoundDesc SA_sand2;
extern SoundDesc SA_sand3;
extern SoundDesc SA_sand4;
extern SoundDesc SA_stone1;
extern SoundDesc SA_stone2;
extern SoundDesc SA_stone3;
extern SoundDesc SA_stone4;
extern SoundDesc SA_wood1;
extern SoundDesc SA_wood2;
extern SoundDesc SA_wood3;
extern SoundDesc SA_wood4;

extern SoundDesc SA_click;
extern SoundDesc SA_explode;
extern SoundDesc SA_splash;

extern SoundDesc SA_door_open;
extern SoundDesc SA_door_close;
extern SoundDesc SA_pop;
extern SoundDesc SA_pop2;
extern SoundDesc SA_hurt;
extern SoundDesc SA_glass1;
extern SoundDesc SA_glass2;
extern SoundDesc SA_glass3;

extern SoundDesc SA_sheep1;
extern SoundDesc SA_sheep2;
extern SoundDesc SA_sheep3;
extern SoundDesc SA_pig1;
extern SoundDesc SA_pig2;
extern SoundDesc SA_pig3;
extern SoundDesc SA_pigdeath;

//extern SoundDesc SA_chicken1;
extern SoundDesc SA_chicken2;
extern SoundDesc SA_chicken3;
extern SoundDesc SA_chickenhurt1;
extern SoundDesc SA_chickenhurt2;

extern SoundDesc SA_cow1;
extern SoundDesc SA_cow2;
extern SoundDesc SA_cow3;
extern SoundDesc SA_cow4;
extern SoundDesc SA_cowhurt1;
extern SoundDesc SA_cowhurt2;
extern SoundDesc SA_cowhurt3;

extern SoundDesc SA_zombie1;
extern SoundDesc SA_zombie2;
extern SoundDesc SA_zombie3;
extern SoundDesc SA_zombiedeath;
extern SoundDesc SA_zombiehurt1;
extern SoundDesc SA_zombiehurt2;

extern SoundDesc SA_zpig1;
extern SoundDesc SA_zpig2;
extern SoundDesc SA_zpig3;
extern SoundDesc SA_zpig4;
extern SoundDesc SA_zpigangry1;
extern SoundDesc SA_zpigangry2;
extern SoundDesc SA_zpigangry3;
extern SoundDesc SA_zpigangry4;
extern SoundDesc SA_zpigdeath;
extern SoundDesc SA_zpighurt1;
extern SoundDesc SA_zpighurt2;

extern SoundDesc SA_bow;
extern SoundDesc SA_bowhit1;
extern SoundDesc SA_bowhit2;
extern SoundDesc SA_bowhit3;
extern SoundDesc SA_bowhit4;
extern SoundDesc SA_fallbig1;
extern SoundDesc SA_fallbig2;
extern SoundDesc SA_fallsmall;
extern SoundDesc SA_skeleton1;
extern SoundDesc SA_skeleton2;
extern SoundDesc SA_skeleton3;
extern SoundDesc SA_skeletondeath;
extern SoundDesc SA_skeletonhurt1;
extern SoundDesc SA_skeletonhurt2;
extern SoundDesc SA_skeletonhurt3;
extern SoundDesc SA_skeletonhurt4;
extern SoundDesc SA_spider1;
extern SoundDesc SA_spider2;
extern SoundDesc SA_spider3;
extern SoundDesc SA_spider4;
extern SoundDesc SA_spiderdeath;

extern SoundDesc SA_creeper1;
extern SoundDesc SA_creeper2;
extern SoundDesc SA_creeper3;
extern SoundDesc SA_creeper4;
extern SoundDesc SA_creeperdeath;
extern SoundDesc SA_eat1;
extern SoundDesc SA_eat2;
extern SoundDesc SA_eat3;
extern SoundDesc SA_fuse;

#endif /*!PRE_ANDROID23 && !__APPLE__*/


#endif /*NET_MINECRAFT_CLIENT_SOUND__Sound_H__*/
