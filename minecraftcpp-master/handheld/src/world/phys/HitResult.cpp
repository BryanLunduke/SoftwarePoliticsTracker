#include "HitResult.h"
#include "../entity/Entity.h"


HitResult::HitResult()
:	type(NO_HIT),
	entity(NULL),
	indirectHit(false)
{}

HitResult::HitResult(int x, int y, int z, int f, const Vec3& pos)
:	type(TILE),
	x(x),
	y(y),
	z(z),
	f(f),
	pos(pos),
	indirectHit(false),
	entity(NULL)
{
}

HitResult::HitResult(Entity* entity)
:	type(ENTITY),
	entity(entity),
	pos(entity->x, entity->y, entity->z),
	indirectHit(false)
{
}

HitResult::HitResult(const HitResult& hr)
	:	type(hr.type),
	x(hr.x),
	y(hr.y),
	z(hr.z),
	f(hr.f),
	pos(hr.pos),
	indirectHit(hr.indirectHit),
	entity(hr.entity)
{
}


float HitResult::distanceTo(Entity* entity) const {
    float xd = pos.x - entity->x;
    float yd = pos.y - entity->y;
    float zd = pos.z - entity->z;
    return xd * xd + yd * yd + zd * zd;
}
