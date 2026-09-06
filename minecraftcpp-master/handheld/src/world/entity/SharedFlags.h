#ifndef NET_MINECRAFT_WORLD_ENTITY__SharedFlags_H__
#define NET_MINECRAFT_WORLD_ENTITY__SharedFlags_H__

class SharedFlagsInformation {
public:
	enum SharedFlags {
		FLAG_ONFIRE = 0,
		FLAG_SNEAKING = 1,
		FLAG_RIDING = 2,
		FLAG_SPRINTING = 3,
		FLAG_USINGITEM = 4
	};
	typedef char SharedFlagsInformationType;
	
	static const unsigned int DATA_SHARED_FLAGS_ID = 0;
};
#endif /* NET_MINECRAFT_WORLD_ENTITY__SharedFlags_H__ */