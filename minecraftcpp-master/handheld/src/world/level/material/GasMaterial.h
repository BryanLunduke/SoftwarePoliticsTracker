#ifndef NET_MINECRAFT_WORLD_LEVEL_MATERIAL__GasMaterial_H__
#define NET_MINECRAFT_WORLD_LEVEL_MATERIAL__GasMaterial_H__

//package net.minecraft.world.level.material;
#include "Material.h"

class GasMaterial: public Material
{
public:
	GasMaterial() {
		replaceable();
	}

	bool isSolid() const {
        return false;
    }

    bool blocksLight() const {
        return false;
    }
    
    bool blocksMotion() const {
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_MATERIAL__GasMaterial_H__*/
