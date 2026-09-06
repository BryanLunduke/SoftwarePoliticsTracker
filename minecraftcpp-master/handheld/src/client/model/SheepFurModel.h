#ifndef NET_MINECRAFT_CLIENT_MODEL__SheepFurModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__SheepFurModel_H__

//package net.minecraft.client.model;

#include "QuadrupedModel.h"

class SheepFurModel: public QuadrupedModel
{
    typedef QuadrupedModel super;

public:
    SheepFurModel();

    /*@Override*/
    void prepareMobModel(Mob* mob, float time, float r, float a);

    /*@Override*/
    void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale);
private:
    float headXRot;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__SheepFurModel_H__*/
