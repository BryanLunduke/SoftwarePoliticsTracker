#ifndef NET_MINECRAFT_CLIENT_MODEL__ZombieModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__ZombieModel_H__

//package net.minecraft.client.model;

#include "HumanoidModel.h"

class ZombieModel: public HumanoidModel
{
    typedef HumanoidModel super;
public:
    void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) {
		super::setupAnim(time, r, bob, yRot, xRot, scale);
		if(!holdingLeftHand && !holdingRightHand) {
			float attack2 = (float) Mth::sin(attackTime * Mth::PI);
			float attack = (float) Mth::sin((1 - (1 - attackTime) * (1 - attackTime)) * Mth::PI);
			arm0.zRot = 0;// -attack;
			arm1.zRot = 0;// +attack;
			arm0.yRot = -(0.1f - attack2 * 0.6f);
			arm1.yRot = +(0.1f - attack2 * 0.6f);
			arm0.xRot = -Mth::PI / 2.0f;
			arm1.xRot = -Mth::PI / 2.0f;
			arm0.xRot -= attack2 * 1.2f - attack * 0.4f;
			arm1.xRot -= attack2 * 1.2f - attack * 0.4f;

	//        body.yRot = attack;
			const float zBob = Mth::cos(bob * 0.09f) * 0.05f + 0.05f;
			const float xBob = Mth::sin(bob * 0.067f) * 0.05f;
			arm0.zRot += zBob;
			arm1.zRot -= zBob;
			arm0.xRot += xBob;
			arm1.xRot -= xBob;
		}
    }
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__ZombieModel_H__*/
