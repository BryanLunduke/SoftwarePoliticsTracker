#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SpiderRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SpiderRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "MobRenderer.h"

#include "../gles.h"
#include "../../model/SpiderModel.h"
#include "../../../world/entity/monster/Spider.h"

class SpiderRenderer: public MobRenderer
{
    typedef MobRenderer super;
public:
    SpiderRenderer()
    :   super(new SpiderModel(), 1.0f)
    {
        //this->setArmor(/*new*/ SpiderModel());
    }

protected:
    float getFlipDegrees(Mob* spider) {
        return 180;
    }

//    int prepareArmor(Mob* spider, int layer, float a) {
//        if (layer != 0) return -1;
//        bindTexture("/mob/spider_eyes.png");
//        float br = 1.0f;
//        glEnable(GL_BLEND);
//        glDisable(GL_ALPHA_TEST);
////        glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//        glBlendFunc2(GL_ONE, GL_ONE);
//        glColor4f2(1, 1, 1, br);
//        return 1;
//    }

    /*@Override*/
    void scale(Mob* mob, float a) {
        float scale = ((Spider*)mob)->getModelScale();
        glScalef(scale, scale, scale);
    }
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__SpiderRenderer_H__*/
