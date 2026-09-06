#ifndef NET_MINECRAFT_WORLD_LEVEL_BIOME__TaigaBiome_H__
#define NET_MINECRAFT_WORLD_LEVEL_BIOME__TaigaBiome_H__

//package net.minecraft.world.level.biome;

#include "Biome.h"
#include "../../../util/Random.h"
#include "../levelgen/feature/PineFeature.h"
#include "../levelgen/feature/SpruceFeature.h"

class TaigaBiome: public Biome
{
public:
    Feature* getTreeFeature(Random* random) {
        if (random->nextInt(3) == 0) {
            return new PineFeature();
        }
        return new SpruceFeature();
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_BIOME__TaigaBiome_H__*/
