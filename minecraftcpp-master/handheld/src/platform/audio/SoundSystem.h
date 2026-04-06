#ifndef SoundSystem_H__
#define SoundSystem_H__

#include "../../world/level/tile/Tile.h"
#include <string>

class SoundDesc;

class SoundSystem
{
public:
	SoundSystem() {}
	virtual ~SoundSystem() {}

	virtual bool isAvailable() { return false; }
    virtual void enable(bool status) {}

    virtual void setListenerPos(float x, float y, float z){}
    virtual void setListenerAngle(float deg){}

    virtual void load(const std::string& name){}
    virtual void play(const std::string& name){}
    virtual void pause(const std::string& name){}
    virtual void stop(const std::string& name){}

	virtual void playAt(const SoundDesc& desc, float x, float y, float z, float volume, float pitch){}
};

#endif /*SoundSystem_H__ */
