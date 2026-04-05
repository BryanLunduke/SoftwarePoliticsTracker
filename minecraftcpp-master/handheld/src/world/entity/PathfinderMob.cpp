#include "PathfinderMob.h"
#include "../level/Level.h"
#include "../phys/Vec3.h"
#include "../../util/Mth.h"
#include "../../util/PerfTimer.h"
#include "../../SharedConstants.h"

#include "ai/Sensing.h"


PathfinderMob::PathfinderMob( Level* level )
:   super(level),
	//navigation(this, level, 16),
	attackTargetId(0),
	holdGround(false),
	fleeTime(0)//,
	//pathfinderMask(0)
{
	sensing = new Sensing(this);
	navigation = new PathNavigation(this, level, 16);
}

PathfinderMob::~PathfinderMob() {
	delete navigation;
	delete sensing;
}

bool PathfinderMob::canSpawn()
{
	return super::canSpawn() && getWalkTargetValue(Mth::floor(x), Mth::floor(bb.y0), Mth::floor(z)) >= 0;
}

bool PathfinderMob::isPathFinding()
{
	return !path.isEmpty();
}

void PathfinderMob::setPath( Path& path )
{
	this->path = path;
}

Entity* PathfinderMob::getAttackTarget()
{
	if (attackTargetId == 0) return NULL;
	return level->getEntity(attackTargetId);
}

void PathfinderMob::setAttackTarget( Entity* attacker )
{
	attackTargetId = attacker? attacker->entityId : 0;
}

bool PathfinderMob::shouldHoldGround()
{
	return false;
}

void PathfinderMob::updateAi()
{
	TIMER_PUSH("ai");

	if (fleeTime > 0) fleeTime--;
	holdGround = shouldHoldGround();
	float maxDist = 16;

	Entity* attackTarget = NULL;
	if (attackTargetId == 0) {
		attackTarget = findAttackTarget();
		if (attackTarget != NULL) {
			level->findPath(&path, this, attackTarget, maxDist, false, false);//(pathfinderMask&CAN_OPEN_DOORS) != 0, (pathfinderMask&AVOID_WATER) != 0);
			attackTargetId = attackTarget->entityId;
			//LOGI("path.empty: %d\n", path.isEmpty());
		}
	} else {
		attackTarget = level->getEntity(attackTargetId);
		if (!attackTarget || !attackTarget->isAlive()) {
			attackTargetId = 0;
			attackTarget = NULL;
		} else {
			attackTargetId = attackTarget->entityId;
			float d = attackTarget->distanceTo(this);
			if (canSee(attackTarget)) {
				checkHurtTarget(attackTarget, d);
			} else {
				checkCantSeeTarget(attackTarget, d);
			}
		}
	}
	TIMER_POP();

	/*
	* if (holdGround) { xxa = 0; yya = 0; jumping = false; return; }
	*/

	bool doStroll = false;
	if (!holdGround && (attackTarget != NULL && (path.isEmpty() || random.nextInt(20) == 0))) {
		level->findPath(&path, this, attackTarget, maxDist, false, false);//(pathfinderMask&CAN_OPEN_DOORS) != 0, (pathfinderMask&AVOID_WATER) != 0);
	} else if (!holdGround) {
		if (path.isEmpty() && (random.nextInt(180) == 0)) {
			doStroll = true;
		} else {
			if (random.nextInt(120) == 0) doStroll = true;
			else if (fleeTime > 0 && (fleeTime&7) == 1) doStroll = true;
		}
	}
	if (doStroll) {
		if (noActionTime < SharedConstants::TicksPerSecond * 5) {
			findRandomStrollLocation();
		}
	}

	int yFloor = Mth::floor(bb.y0 + .5f);

	bool inWater = isInWater();
	bool inLava = isInLava();
	xRot = 0;
	if (path.isEmpty() || random.nextInt(100) == 0) {
		//super::serverAiStep();
		super::updateAi();
		return;
	}

	TIMER_PUSH("followpath");
	Vec3 target = path.currentPos(this);
	float r = bbWidth * 2;
	bool looping = true;
	while (looping && target.distanceToSqr(x, target.y, z) < r * r) {
		path.next();
		if (path.isDone()) {
			looping = false;
			path.destroy();
		} else target = path.currentPos(this);
	}

	jumping = false;
	if (looping) {
		float xd = target.x - x;
		float zd = target.z - z;
		float yd = target.y - yFloor;
		// float yRotOld = yRot;
		float yRotD = (float) (Mth::atan2(zd, xd) * 180 / Mth::PI) - 90;
		float rotDiff = yRotD - yRot;
		yya = runSpeed;
		while (rotDiff < -180)
			rotDiff += 360;
		while (rotDiff >= 180)
			rotDiff -= 360;
		if (rotDiff > MAX_TURN) {
			rotDiff = MAX_TURN;
			// yya *= 0.2;
		}
		if (rotDiff < -MAX_TURN) {
			rotDiff = -MAX_TURN;
			// yya *= 0.2;
		}
		yRot += rotDiff;

		if (holdGround) {
			if (attackTarget != NULL) {
				float xd2 = attackTarget->x - x;
				float zd2 = attackTarget->z - z;

				float oldyRot = yRot;
				yRot = (float) (Mth::atan2(zd2, xd2) * 180 / Mth::PI) - 90;

				rotDiff = ((oldyRot - yRot) + 90) * Mth::PI / 180;
				xxa = -Mth::sin(rotDiff) * yya * 1.0f;
				yya = Mth::cos(rotDiff) * yya * 1.0f;
			}
		}
		if (yd > 0) {
			jumping = true;
		}
	}

	if (attackTarget != NULL) {
		lookAt(attackTarget, 30, 30);
	}

	if (this->horizontalCollision && !isPathFinding()) jumping = true;
	if (random.nextFloat() < 0.8f && (inWater || inLava)) jumping = true;

	TIMER_POP();
}

void PathfinderMob::findRandomStrollLocation()
{
	TIMER_PUSH("stroll");
	bool hasBest = false;
	int xBest = -1;
	int yBest = -1;
	int zBest = -1;
	float best = -99999;
	for (int i = 0; i < 10; i++) {
		int xt = Mth::floor(x + random.nextInt(13) - 6);
		int yt = Mth::floor(y + random.nextInt(7) - 3);
		int zt = Mth::floor(z + random.nextInt(13) - 6);
		float value = getWalkTargetValue(xt, yt, zt);
		if (value > best) {
			best = value;
			xBest = xt;
			yBest = yt;
			zBest = zt;
			hasBest = true;
		}

	}
	if (hasBest) {
		//LOGI("Finding a new strolling location! %d, %d, %d (%d, %d, %d) for %p\n", xBest, yBest, zBest, (int)x, (int)y, (int)z, this);
		level->findPath(&path, this, xBest, yBest, zBest, 10, false, false);//(pathfinderMask&CAN_OPEN_DOORS) != 0, (pathfinderMask&AVOID_WATER) != 0);
	}
	TIMER_POP();
}

void PathfinderMob::checkHurtTarget( Entity* target, float d )
{
}

void PathfinderMob::checkCantSeeTarget( Entity* target, float d )
{
	if (d > 32)
		attackTargetId = 0;
}

float PathfinderMob::getWalkTargetValue( int x, int y, int z )
{
	return 0;
}

Entity* PathfinderMob::findAttackTarget()
{
	return NULL;
}

float PathfinderMob::getWalkingSpeedModifier() {
	float speed = super::getWalkingSpeedModifier();
	if (fleeTime > 0) speed *= 2;
	return speed;
}

int PathfinderMob::getNoActionTime() {
	return noActionTime;
}

PathNavigation* PathfinderMob::getNavigation() {
	return navigation;
}
