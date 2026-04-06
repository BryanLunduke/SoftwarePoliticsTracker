#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__CreeperRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__CreeperRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "MobRenderer.h"

#include "../../model/CreeperModel.h"
#include "../../model/Model.h"
#include "../../../world/entity/monster/Creeper.h"
#include "../../../util/Mth.h"

class CreeperRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
    CreeperRenderer()
	:	super(new CreeperModel(), 0.5f)
	{
    }

protected:
    void scale(Mob* mob, float a) {
        Creeper* creeper = (Creeper*) mob;

        float g = creeper->getSwelling(a);

        float wobble = 1.0f + Mth::sin(g * 100) * g * 0.01f;
        if (g < 0) g = 0;
        if (g > 1) g = 1;
        g = g * g;
        g = g * g;
        float s = (1.0f + g * 0.4f) * wobble;
        float hs = (1.0f + g * 0.1f) / wobble;
        glScalef(s, hs, s);
    }

    int getOverlayColor(Mob* mob, float br, float a) {
        Creeper* creeper = (Creeper*) mob;

        float step = creeper->getSwelling(a);

        if ((int) (step * 10) % 2 == 0) return 0;

        int _a = (int) (step * 0.2f * 255);
        if (_a < 0) _a = 0;
        if (_a > 255) _a = 255;

        int r = 255;
        int g = 255;
        int b = 255;

        return (_a << 24) | (r << 16) | (g << 8) | b;
    }

  //  int prepareArmor(Mob* mob, int layer, float a) {
		//Creeper* creeper = (Creeper*) mob;
  //      if (creeper->isPowered()) {
  //          if (layer == 1) {
  //              float time = mob->tickCount + a;
  //              bindTexture("/armor/power.png");
  //              glMatrixMode(GL_TEXTURE);
  //              glLoadIdentity();
  //              float uo = time * 0.01f;
  //              float vo = time * 0.01f;
  //              glTranslatef(uo, vo, 0);
  //              setArmor(armorModel);
  //              glMatrixMode(GL_MODELVIEW);
  //              glEnable(GL_BLEND);
  //              float br = 0.5f;
  //              glColor4f2(br, br, br, 1);
  //              glDisable(GL_LIGHTING);
  //              glBlendFunc2(GL_ONE, GL_ONE);
  //              return 1;
  //          }
  //          if (layer == 2) {
  //              glMatrixMode(GL_TEXTURE);
  //              glLoadIdentity();
  //              glMatrixMode(GL_MODELVIEW);
  //              glEnable(GL_LIGHTING);
  //              glDisable(GL_BLEND);
  //          }
  //      }
  //      return -1;
  //  }

  //  int prepareArmorOverlay(Mob* mob, int layer, float a) {
  //      return -1;
  //  }
//private:
    //Model* armorModel = /*new*/ CreeperModel(2);
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__CreeperRenderer_H__*/
