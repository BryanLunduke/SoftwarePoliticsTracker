#ifndef NET_MINECRAFT_CLIENT_SOUND__SoundEngine_H__
#define NET_MINECRAFT_CLIENT_SOUND__SoundEngine_H__

//package net.minecraft.client.sound;

#if defined(ANDROID) && !defined(PRE_ANDROID23)
	#include "../../platform/audio/SoundSystemSL.h"
#elif defined(__APPLE__)
    #include "../../platform/audio/SoundSystemAL.h"
#else
	#include "../../platform/audio/SoundSystem.h"
#endif
#include "SoundRepository.h"
#include "../../util/Random.h"

class Minecraft;
class Mob;
class Options;

class SoundEngine
{
    static const int SOUND_DISTANCE = 16;

	#if defined(ANDROID) && !defined(PRE_ANDROID23) && !defined(RPI)
		SoundSystemSL soundSystem;
    #elif defined(__APPLE__)
        SoundSystemAL soundSystem;
	#else
	    SoundSystem soundSystem;
	#endif

	Options* options;
	int idCounter;
	//static bool loaded;
	Random random;
	//int noMusicDelay = random.nextInt(20 * 60 * 10);
	float _x;
	float _y;
	float _z;
    float _yRot;
	float _invMaxDistance;

public:
	SoundEngine(float maxDistance);

	~SoundEngine();

    void init(Minecraft* mc, Options* options);
	void destroy();

	void enable(bool status);

	void updateOptions();
    void update(Mob* player, float a);

	void play(const std::string& name, float x, float y, float z, float volume, float pitch);
	void playUI(const std::string& name, float volume, float pitch);

	float _getVolumeMult(float x, float y, float z);
private:
	void loadLibrary() {}
    SoundDesc _pp(const std::string& fn);
    
	SoundRepository sounds;
	Minecraft* mc;
};

#endif /*NET_MINECRAFT_CLIENT_SOUND__SoundEngine_H__*/
