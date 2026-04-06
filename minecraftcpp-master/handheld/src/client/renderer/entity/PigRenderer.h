#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PigRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PigRenderer_H__

//package net.minecraft.client.renderer.entity;

#include "../../model/Model.h"
#include "../../../world/entity/animal/Pig.h"

class PigRenderer: public MobRenderer
{
	typedef MobRenderer super;
public:
    PigRenderer(Model* model, Model* armor, float shadow)
	:	super(model, shadow)
	{
       // setArmor(armor);
    }

    //void render(Entity* mob, float x, float y, float z, float rot, float a) {
    //    super::render(mob, x, y, z, rot, a);
    //}    
protected:
	//int prepareArmor(Entity* pig, int layer, float a) {
	//	bindTexture("/mob/saddle.png");
	//	return (layer == 0 && pig->hasSaddle()) ? 1 : -1;
	//}
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PigRenderer_H__*/
