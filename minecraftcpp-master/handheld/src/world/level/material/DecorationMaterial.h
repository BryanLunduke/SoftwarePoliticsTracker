#ifndef NET_MINECRAFT_WORLD_LEVEL_MATERIAL__DecorationMaterial_H__
#define NET_MINECRAFT_WORLD_LEVEL_MATERIAL__DecorationMaterial_H__

//package net.minecraft.world.level.material;
#include "Material.h"

class DecorationMaterial: public Material
{
public:
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

#endif /*NET_MINECRAFT_WORLD_LEVEL_MATERIAL__DecorationMaterial_H__*/
