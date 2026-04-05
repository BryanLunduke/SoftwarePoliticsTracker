#ifndef NET_MINECRAFT_CLIENT_MODEL__SpiderModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__SpiderModel_H__

//package net.minecraft.client.model;

#include "Model.h"

#include "geom/ModelPart.h"
#include "../../world/entity/Entity.h"
#include "../../util/Mth.h"

class SpiderModel: public Model
{
    typedef Model super;
public:
    ModelPart head, body0, body1, leg0, leg1, leg2, leg3, leg4, leg5, leg6, leg7;

    SpiderModel()
    :   head(32, 4),
        body0(0, 0),
        body1(0, 12),
        leg0(18, 0),
        leg1(18, 0),
        leg2(18, 0),
        leg3(18, 0),
        leg4(18, 0),
        leg5(18, 0),
        leg6(18, 0),
        leg7(18, 0)
    {
        float g = 0;

    	head.setModel(this);
    	body0.setModel(this);
    	body0.setModel(this);
    	leg0.setModel(this);
    	leg1.setModel(this);
    	leg2.setModel(this);
    	leg3.setModel(this);
    	leg4.setModel(this);
    	leg5.setModel(this);
    	leg6.setModel(this);
    	leg7.setModel(this);

        float yo = 18 + 6 - 9;

        head.addBox(-4, -4, -8, 8, 8, 8, g); // Head
        head.setPos(0, 0 + yo, -3);

        body0.addBox(-3, -3, -3, 6, 6, 6, g); // Body
        body0.setPos(0, yo, 0);

        body1.addBox(-5, -4, -6, 10, 8, 12, g); // Body
        body1.setPos(0, 0 + yo, 3 + 6);

        leg0.addBox(-15, -1, -1, 16, 2, 2, g); // Leg0
        leg0.setPos(-4, 0 + yo, 2);

        leg1.addBox(-1, -1, -1, 16, 2, 2, g); // Leg1
        leg1.setPos(4, 0 + yo, 2);

        leg2.addBox(-15, -1, -1, 16, 2, 2, g); // Leg2
        leg2.setPos(-4, 0 + yo, 1);

        leg3.addBox(-1, -1, -1, 16, 2, 2, g); // Leg3
        leg3.setPos(4, 0 + yo, 1);

        leg4.addBox(-15, -1, -1, 16, 2, 2, g); // Leg0
        leg4.setPos(-4, 0 + yo, 0);

        leg5.addBox(-1, -1, -1, 16, 2, 2, g); // Leg1
        leg5.setPos(4, 0 + yo, 0);

        leg6.addBox(-15, -1, -1, 16, 2, 2, g); // Leg2
        leg6.setPos(-4, 0 + yo, -1);

        leg7.addBox(-1, -1, -1, 16, 2, 2, g); // Leg3
        leg7.setPos(4, 0 + yo, -1);
    }

    /*@Override*/
    void render(Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale) {
        setupAnim(time, r, bob, yRot, xRot, scale);

        head.render(scale);
        body0.render(scale);
        body1.render(scale);
        leg0.render(scale);
        leg1.render(scale);
        leg2.render(scale);
        leg3.render(scale);
        leg4.render(scale);
        leg5.render(scale);
        leg6.render(scale);
        leg7.render(scale);
    }

    void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale) {
        head.yRot = yRot / (float) (180 / Mth::PI);
        head.xRot = xRot / (float) (180 / Mth::PI);

        const float sr = (float) Mth::PI / 4.0f;
        leg0.zRot = -sr;
        leg1.zRot = sr;

        leg2.zRot = -sr * 0.74f;
        leg3.zRot = sr * 0.74f;

        leg4.zRot = -sr * 0.74f;
        leg5.zRot = sr * 0.74f;

        leg6.zRot = -sr;
        leg7.zRot = sr;

        //float ro = -(float) Mth::PI / 2.0f * 0;
        const float ur = (float) Mth::PI / 8.0f;
		const float ur2 = sr;

        leg0.yRot =  ur2;
        leg1.yRot = -ur2;
        leg2.yRot =  ur;
        leg3.yRot = -ur;
        leg4.yRot = -ur;
        leg5.yRot = +ur;
        leg6.yRot = -ur2;
        leg7.yRot = +ur2;

		//static Stopwatch w;
		//w.start();

        float c0 = -((float) Mth::cos(time * 0.6662f * 2 + Mth::PI * 2 * 0 / 4.0f) * 0.4f) * r;
        float c1 = -((float) Mth::cos(time * 0.6662f * 2 + Mth::PI * 2 * 2 / 4.0f) * 0.4f) * r;
        float c2 = -((float) Mth::cos(time * 0.6662f * 2 + Mth::PI * 2 * 1 / 4.0f) * 0.4f) * r;
        float c3 = -((float) Mth::cos(time * 0.6662f * 2 + Mth::PI * 2 * 3 / 4.0f) * 0.4f) * r;

		//LOGI("spider: %f, %f, %f, %f\n", c0, c1, c2, c3);

        float s0 = std::abs((float) Mth::sin(time * 0.6662f + Mth::PI * 2 * 0 / 4.0f) * 0.4f) * r;
        float s1 = std::abs((float) Mth::sin(time * 0.6662f + Mth::PI * 2 * 2 / 4.0f) * 0.4f) * r;
        float s2 = std::abs((float) Mth::sin(time * 0.6662f + Mth::PI * 2 * 1 / 4.0f) * 0.4f) * r;
        float s3 = std::abs((float) Mth::sin(time * 0.6662f + Mth::PI * 2 * 3 / 4.0f) * 0.4f) * r;

		//w.stop();
		//w.printEvery(100);

		//LOGI("spiddy: %f, %f, %f, %f\n", s0, s1, s2, s3);

        leg0.yRot += c0;
        leg1.yRot -= c0;
        leg2.yRot += c1;
        leg3.yRot -= c1;
        leg4.yRot += c2;
        leg5.yRot -= c2;
        leg6.yRot += c3;
        leg7.yRot -= c3;

        leg0.zRot += s0;
        leg1.zRot -= s0;
        leg2.zRot += s1;
        leg3.zRot -= s1;
        leg4.zRot += s2;
        leg5.zRot -= s2;
        leg6.zRot += s3;
        leg7.zRot -= s3;
    }
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__SpiderModel_H__*/
