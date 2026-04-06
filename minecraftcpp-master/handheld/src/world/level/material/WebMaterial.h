#ifndef NET_MINECRAFT_WORLD_LEVEL_MATERIAL__WebMaterial_H__
#define NET_MINECRAFT_WORLD_LEVEL_MATERIAL__WebMaterial_H__

//package net.minecraft.world.level.material;
#include "Material.h"

class WebMaterial: public Material
{
public:
    WebMaterial() {
        notAlwaysDestroyable();
    }

    bool blocksMotion() const {
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_MATERIAL__WebMaterial_H__*/
