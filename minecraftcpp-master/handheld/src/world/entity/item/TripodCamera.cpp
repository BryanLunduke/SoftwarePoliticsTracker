#include "TripodCamera.h"
#include "../player/Player.h"
#include "../../level/Level.h"


TripodCamera::TripodCamera( Level* level, Player* owner, float x, float y, float z )
:	super(level),
	owner(owner),
	life(80),
	activated(false)
{
	entityRendererId = ER_TRIPODCAMERA_RENDERER;

	// Copy rotation from the entity placing the camera 
	xRot = xRotO = owner->xRot;
	yRot = yRotO = owner->yRot;

	blocksBuilding = true;
	setSize(1.0f, 1.5f);
	heightOffset = bbHeight / 2.0f - 0.25f;

	setPos(x, y, z);

	xo = x;
	yo = y;
	zo = z;
}

bool TripodCamera::isPickable()
{
	return !removed;
}

bool TripodCamera::interactPreventDefault()
{
	return true;
}

bool TripodCamera::interact( Player* player )
{
	activated = true;
	return true;
}

void TripodCamera::tick()
{
	xo = x;
	yo = y;
	zo = z;

	yd -= 0.04f;
	move(xd, yd, zd);
	xd *= 0.98f;
	yd *= 0.98f;
	zd *= 0.98f;

	if (onGround) {
		xd *= 0.7f;
		zd *= 0.7f;
		yd *= -0.5f;
	}

	if (activated) {
		--life;

		if (life == 0) {
			remove();
		} else if (life == 8) {
			level->takePicture(this, owner);
			level->addParticle(PARTICLETYPE(explode), x, y + 0.6f, z, 0, 0, 0);
			level->addParticle(PARTICLETYPE(explode), x, y + 0.8f, z, 0, 0, 0);
			level->addParticle(PARTICLETYPE(explode), x, y + 1.0f, z, 0, 0, 0);
		} else if (life > 8) {
			level->addParticle(PARTICLETYPE(smoke), x, y + 1.0f, z, 0, 0, 0);
		}
	}
}

float TripodCamera::getShadowHeightOffs()
{
	return 0;
}

bool TripodCamera::isPushable()
{
	return false;
}

