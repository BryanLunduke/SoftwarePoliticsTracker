#ifndef SoundSystemSL_H__
#define SoundSystemSL_H__

#include "SoundSystem.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <vector>
#include <list>

#ifdef ANDROID
#include <pthread.h>
class Mutex {
	pthread_mutex_t _mutex;
public:
	Mutex()			{ pthread_mutex_init(&_mutex, NULL); }
	~Mutex()		{ pthread_mutex_destroy(&_mutex); }
	void lock()		{ pthread_mutex_lock(&_mutex); }
	void unlock()	{ pthread_mutex_unlock(&_mutex); }
};
#else
class Mutex {
public:
	Mutex() {}
	void lock() {}
	void unlock() {};
};
#endif


class SoundSystemSL: public SoundSystem
{
	typedef std::list<SLObjectItf> SoundList;
public:
    SoundSystemSL();
	~SoundSystemSL();

	virtual void init();
	virtual void destroy();

    virtual void setListenerPos(float x, float y, float z);
	virtual void setListenerAngle(float deg);

    virtual void load(const std::string& name){}
    virtual void play(const std::string& name){}
    virtual void pause(const std::string& name){}
    virtual void stop(const std::string& name){}
	virtual void playAt(const SoundDesc& sound, float x, float y, float z, float volume, float pitch);

private:
	bool checkErr( SLresult res );

	void removeStoppedSounds();

	SoundList playingBuffers;

	static SLObjectItf objEngine;
	SLEngineItf engEngine;

	SLObjectItf	objListener;
	SL3DLocationItf listener;

	SLObjectItf objOutput;

	Vec3 listenerPos;
	int numBuffersPlaying;
	
	bool available;
	std::vector<SLObjectItf> toRemoveCopy;

	static Mutex toRemoveMutex;
	static std::vector<SLObjectItf> toRemove;
	static const int MAX_BUFFERS_PLAYING = 4;

public:
	static void removePlayer(SLAndroidSimpleBufferQueueItf bq, void *context);
};

#endif /*SoundSystemSL_H__ */
