#ifndef NET_MINECRAFT_CLIENT_MODEL__ChestModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__ChestModel_H__

//package net.minecraft.client.model;

#include "Model.h"
#include "geom/ModelPart.h"

class ChestModel: public Model
{
public:
    ChestModel()
    :   lid(0, 0),
        lock(0, 0),
        bottom(0, 0)
    {
        lid.setModel(this);
        lid.setTexSize(64, 64);
        lid.addBox(0.0f, -5, -14, 14, 5, 14, 0);
        lid.x = 1;
        lid.y = 7;
        lid.z = 15;

        lock.setModel(this);
        lock.setTexSize(64, 64);
        lock.addBox(-1, -2, -15, 2, 4, 1, 0);
        lock.x = 8;
        lock.y = 7;
        lock.z = 15;

        bottom.setModel(this);
        bottom.setTexSize(64, 64);
        bottom.addBox(0.0f, 0, 0, 14, 10, 14, 0);
        bottom.x = 1;
        bottom.y = 6;
        bottom.z = 1;
    }

    void render() {
        lock.xRot = lid.xRot;

        lid.render(1 / 16.0f);
        lock.render(1 / 16.0f);
        bottom.render(1 / 16.0f);
    }

    ModelPart lid;
    ModelPart bottom;
    ModelPart lock;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__ChestModel_H__*/
