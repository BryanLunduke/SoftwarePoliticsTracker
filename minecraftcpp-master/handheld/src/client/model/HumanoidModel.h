#ifndef NET_MINECRAFT_CLIENT_MODEL__HumanoidModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__HumanoidModel_H__

//package net.minecraft.client.model;

#include "Model.h"
#include "geom/ModelPart.h"
class ItemInstance;
class HumanoidModel: public Model
{
public:
    HumanoidModel(float g = 0, float yOffset = 0);

	void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale);

	void render(HumanoidModel* model, float scale);
    void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale);
	void renderHorrible(float time, float r, float bob, float yRot, float xRot, float scale);
	void onGraphicsReset();

	ModelPart head, /*hair,*/ body, arm0, arm1, leg0, leg1;//, ear;
	bool holdingLeftHand;
	bool holdingRightHand;
	bool sneaking;
	bool bowAndArrow;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__HumanoidModel_H__*/
