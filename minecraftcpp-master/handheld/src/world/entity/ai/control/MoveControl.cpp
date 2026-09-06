#include "MoveControl.h"
#include "JumpControl.h"

const float MoveControl::MAX_TURN = 30;

const float MoveControl::MIN_SPEED = 0.0005f;
const float MoveControl::MIN_SPEED_SQR = MIN_SPEED * MIN_SPEED;


MoveControl::MoveControl( Mob* mob )
:   mob(mob),
	wantedX(mob->x),
	wantedY(mob->y),
	wantedZ(mob->z),
	_hasWanted(false)
{
}

bool MoveControl::hasWanted() {
	return _hasWanted;
}

float MoveControl::getSpeed() {
	return speed;
}

void MoveControl::setWantedPosition( float x, float y, float z, float speed ) {
	//LOGI("> %f, %f, %f\n", x, y, z);
	wantedX = x;
	wantedY = y;
	wantedZ = z;
	this->speed = speed;
	_hasWanted = true;
}

void MoveControl::tick() {
	mob->setYya(0);
	if (!_hasWanted) return;
	_hasWanted = false;

	int yFloor = Mth::floor(mob->bb.y0 + 0.5f);

	float xd = wantedX - mob->x;
	float zd = wantedZ - mob->z;
	float yd = wantedY - yFloor;
	float dd = xd * xd + yd * yd + zd * zd;
	if (dd < MIN_SPEED_SQR) return;

	float yRotD = (float) (std::atan2(zd, xd) * 180 / Mth::PI) - 90;

	mob->yRot = rotlerp(mob->yRot, yRotD, MAX_TURN);
	mob->setSpeed(speed);

	if (yd > 0 && xd * xd + zd * zd < 1) mob->getJumpControl()->jump();
}

float MoveControl::rotlerp( float a, float b, float max ) {
	float diff = b - a;
	while (diff < -180)
		diff += 360;
	while (diff >= 180)
		diff -= 360;
	if (diff > max) {
		diff = max;
	}
	if (diff < -max) {
		diff = -max;
	}
	return a + diff;
}
