#ifndef NET_MINECRAFT_CLIENT_MODEL__CreeperModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__CreeperModel_H__

//package net.minecraft.client.model;

#include "Model.h"

#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"

class CreeperModel: public Model
{
    typedef Model super;
public:
    ModelPart head, /*hair,*/ body, leg0, leg1, leg2, leg3;

    CreeperModel(float g = 0)
    :   head(0, 0),
        //hair(32, 0),
        body(16, 16),
        leg0(0, 16),
        leg1(0, 16),
        leg2(0, 16),
        leg3(0, 16)
    {
        float yo = 4;

    	head.setModel(this);
    	//hair.setModel(this);
    	body.setModel(this);
    	leg0.setModel(this);
    	leg1.setModel(this);
    	leg2.setModel(this);
    	leg3.setModel(this);

        head.addBox(-4, -8, -4, 8, 8, 8, g); // Head
        head.setPos(0, yo, 0);

//         hair.addBox(-4, -8, -4, 8, 8, 8, g + 0.5f); // Head
//         hair.setPos(0, yo, 0);

        body.addBox(-4, 0, -2, 8, 12, 4, g); // Body
        body.setPos(0, yo, 0);

        leg0.addBox(-2, 0, -2, 4, 6, 4, g); // Leg0
        leg0.setPos(-2, 12 + yo, 4);

        leg1.addBox(-2, 0, -2, 4, 6, 4, g); // Leg1
        leg1.setPos(2, 12 + yo, 4);

        leg2.addBox(-2, 0, -2, 4, 6, 4, g); // Leg2
        leg2.setPos(-2, 12 + yo, -4);

        leg3.addBox(-2, 0, -2, 4, 6, 4, g); // Leg3
        leg3.setPos(2, 12 + yo, -4);
    }

    /*@Override*/
    void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
        setupAnim(time, r, bob, yRot, xRot, scale);

        head.render(scale);
        body.render(scale);
        leg0.render(scale);
        leg1.render(scale);
        leg2.render(scale);
        leg3.render(scale);
    }

    void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) {
        head.yRot = yRot / (float) (180 / Mth::PI);
        head.xRot = xRot / (float) (180 / Mth::PI);

		const float pend = (Mth::cos(time * 0.6662f) * 1.4f) * r;
        leg0.xRot = pend;
        leg1.xRot = -pend;
        leg2.xRot = -pend;
        leg3.xRot = pend;
    }
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__CreeperModel_H__*/
