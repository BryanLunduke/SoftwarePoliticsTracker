#ifndef NET_MINECRAFT_CLIENT_PARTICLE__CritParticle2_H__
#define NET_MINECRAFT_CLIENT_PARTICLE__CritParticle2_H__

//package net.minecraft.client.particle;

#include "Particle.h"
#include "../../util/Mth.h"
#include "../../world/level/Level.h"
#include "../../world/level/material/Material.h"

class CritParticle2: public Particle
{
	typedef Particle super;
public:
	CritParticle2(Level* level, float x, float y, float z, float xa, float ya, float za, float scale = 1.0f)
	:	super(level, x, y, z, 0, 0, 0),
		visible(true)
	{
		xd *= 0.1f;
		yd *= 0.1f;
		zd *= 0.1f;
		xd += xa * 0.4f;
		yd += ya * 0.4f;
		zd += za * 0.4f;

		rCol = gCol = bCol = (float) (Mth::random() * 0.3f + 0.6f);
		size *= 0.75f;
		size *= scale;
		oSize = size;

		lifetime = (int)(6 / (Mth::random() * 0.8f + 0.6f) * scale);
		noPhysics = false;

		tex = 16 * 4 + 1;
		tick();
	}

	void render(Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2) {
		if (!visible) return;
		float l = ((age + a) / lifetime) * 32;
		if (l < 0) l = 0;
		if (l > 1) l = 1;

		size = oSize * l;
		super::render(t, a, xa, ya, za, xa2, za2);
	}

	void tick() {
		xo = x;
		yo = y;
		zo = z;

		if (age++ >= lifetime) remove();

		move(xd, yd, zd);
		gCol *= 0.96f;
		bCol *= 0.9f;

		xd *= 0.70f;
		yd *= 0.70f;
		zd *= 0.70f;
		yd -= 0.02f;

 		if (onGround) {
			xd *= 0.7f;
			zd *= 0.7f;
		}
	}
protected:
	bool visible;
	float oSize;
};

#endif /*NET_MINECRAFT_CLIENT_PARTICLE__CritParticle2_H__*/
