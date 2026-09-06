#ifndef NET_MINECRAFT_WORLD_LEVEL_DIMENSION__NormalDayCycleDimension_H__
#define NET_MINECRAFT_WORLD_LEVEL_DIMENSION__NormalDayCycleDimension_H__

//package net.minecraft.world.level.dimension;

#include "Dimension.h"
#include "../Level.h"
#include "../../../util/Mth.h"

class NormalDayCycleDimension: public Dimension {
public:
	float getTimeOfDay(long time, float a) {
		int dayStep = (int) (time % Level::TICKS_PER_DAY);
		float td = (dayStep + a) / Level::TICKS_PER_DAY - 0.25f;
		if (td < 0) td += 1;
		if (td > 1) td -= 1;
		float tdo = td;
		td = 1 - (cos(td * Mth::PI) + 1) * 0.5f;
		return tdo + (td - tdo) / 3.0f;
	}

	Vec3 getFogColor( float td, float a ) {
		float br = cos(td * Mth::PI * 2) * 2 + 0.5f;
		if (br < 0) br = 0;
		if (br > 1.f) br = 1.f;

		float r = ((fogColor >> 16) & 0xff) / 255.0f;
		float g = ((fogColor >> 8) & 0xff) / 255.0f;
		float b = ((fogColor) & 0xff) / 255.0f;
		r *= br * 0.94f + 0.06f;
		g *= br * 0.94f + 0.06f;
		b *= br * 0.91f + 0.09f;

		//LOGI("rgb: %f, %f, %f\n", r, g, b);

		return Vec3(r, g, b);
	}
	
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_DIMENSION__NormalDayCycleDimension_H__*/
