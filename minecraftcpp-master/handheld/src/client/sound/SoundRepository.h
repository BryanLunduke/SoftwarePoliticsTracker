#ifndef NET_MINECRAFT_CLIENT_SOUND__SoundRepository_H__
#define NET_MINECRAFT_CLIENT_SOUND__SoundRepository_H__

#include <map>
#include <string>
#include "Sound.h"
#include "../../util/Mth.h"
#include "../../platform/log.h"

class SoundRepository
{
	typedef std::vector<SoundDesc> SoundList;
	typedef std::map<std::string, SoundList> SoundMap;

public:
    ~SoundRepository() {
#ifdef __APPLE__
        SoundMap::iterator it = map.begin();
        for (; it != map.end(); ++it) {
            SoundList& list = it->second;
            for (unsigned int j = 0; j < list.size(); ++j)
                list[j].destroy();
        }
#endif
    }
	
    bool get(const std::string& name, SoundDesc& sound) {
		SoundMap::iterator it = map.find(name);
		if (it == map.end()) {
			LOGI("Couldn't find a sound with id: %s\n", name.c_str());
			return false;
		}
		sound = it->second[Mth::random(it->second.size())];
		return true;
	}

	void add(const std::string& name, const SoundDesc& sound) {
		if (!sound.isValid()) return;
        SoundMap::iterator it = map.find(name);
		if (it == map.end()) {
			SoundList list;
			list.push_back( sound );
			map.insert( make_pair(name, list) );
		} else {
			it->second.push_back(sound);
		}
	}

	void add(const std::string& name, SoundDesc& sound) {
        if (!sound.isValid()) return;
        add(name, const_cast<const SoundDesc&>(sound));
        sound.name = name;
	}

private:
	SoundMap map;
};

#endif /*NET_MINECRAFT_CLIENT_SOUND__SoundRepository_H__*/
