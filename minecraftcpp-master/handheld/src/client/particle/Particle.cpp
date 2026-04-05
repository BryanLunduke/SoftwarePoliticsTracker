#include "Particle.h"

float
	Particle::xOff = 0,
	Particle::yOff = 0,
	Particle::zOff = 0;

Particle::Particle( Level* level, float x, float y, float z, float xa, float ya, float za )
:	super(level),
	age(0),
	gravity(0),
	rCol(1), gCol(1), bCol(1),
	tex(0)
{
	setSize(0.2f, 0.2f);
	heightOffset = bbHeight / 2.0f;
	setPos(x, y, z);

	xd = xa + (float) (Mth::random() * 2 - 1) * 0.4f;
	yd = ya + (float) (Mth::random() * 2 - 1) * 0.4f;
	zd = za + (float) (Mth::random() * 2 - 1) * 0.4f;
	float speed = (float) (Mth::random() + Mth::random() + 1) * 0.15f;

	float dd = (float) (Mth::sqrt(xd * xd + yd * yd + zd * zd));
	const float mul = 0.4f * speed / dd;
	xd = xd * mul;
	yd = yd * mul + 0.1f;
	zd = zd * mul;

	uo = sharedRandom.nextFloat() * 3;
	vo = sharedRandom.nextFloat() * 3;

	size = (sharedRandom.nextFloat() * 0.5f + 0.5f) * 2;

	lifetime = (int) (4.0f / (sharedRandom.nextFloat() * 0.9f + 0.1f));
	makeStepSound = false;
}

Particle* Particle::setPower( float power )
{
	xd *= power;
	yd = (yd - 0.1f) * power + 0.1f;
	zd *= power;
	return this;
}

Particle* Particle::scale( float scale )
{
	setSize(0.2f * scale, 0.2f * scale);
	size *= scale;
	return this;
}

void Particle::tick()
{
	xo = x;
	yo = y;
	zo = z;

	if (age++ >= lifetime) remove();

	yd -= 0.04f * gravity;
	move(xd, yd, zd);
	xd *= 0.98f;
	yd *= 0.98f;
	zd *= 0.98f;

	if (onGround) {
		xd *= 0.7f;
		zd *= 0.7f;
	}
}

void Particle::render( Tesselator& t, float a, float xa, float ya, float za, float xa2, float za2 )
{
	float u0 = (tex % 16) / 16.0f;
	float u1 = u0 + 0.999f / 16.0f;
	float v0 = (tex / 16) / 16.0f;
	float v1 = v0 + 0.999f / 16.0f;
	float r = 0.1f * size;

	float x = (float) (xo + (this->x - xo) * a - xOff);
	float y = (float) (yo + (this->y - yo) * a - yOff);
	float z = (float) (zo + (this->z - zo) * a - zOff);

	float br = getBrightness(a);
	t.color(rCol * br, gCol * br, bCol * br);

	t.vertexUV(x - xa * r - xa2 * r, y - ya * r, z - za * r - za2 * r, u1, v1);
	t.vertexUV(x - xa * r + xa2 * r, y + ya * r, z - za * r + za2 * r, u1, v0);
	t.vertexUV(x + xa * r + xa2 * r, y + ya * r, z + za * r + za2 * r, u0, v0);
	t.vertexUV(x + xa * r - xa2 * r, y - ya * r, z + za * r - za2 * r, u0, v1);
	//printf("uv: %f, %f, %f, %f\n", u0, v0, u1, v1);
}

int Particle::getParticleTexture()
{
	return ParticleEngine::MISC_TEXTURE;
}
