#ifndef NET_MINECRAFT_WORLD_ENTITY__EntityPos_H__
#define NET_MINECRAFT_WORLD_ENTITY__EntityPos_H__

//package net.minecraft.world.entity;

#include "Entity.h"

class EntityPos
{
public:
    float x, y, z;
    float yRot, xRot;
    bool rot;
    bool move;

    EntityPos(float x, float y, float z, float yRot, float xRot)
	:	rot(false),
		move(false)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->yRot = yRot;
        this->xRot = xRot;
        rot = true;
        move = true;
    }

    EntityPos(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        move = true;
        rot = false;
    }

    EntityPos(float yRot, float xRot)
    {
        this->yRot = yRot;
        this->xRot = xRot;
        rot = true;
        move = false;
    }

//    EntityPos lerp(Entity* e, float f)
//    {
//        float xd = e->x+(x-e->x)*f;
//        float yd = e->y+(y-e->y)*f;
//        float zd = e->z+(z-e->z)*f;
//        
//        float yrdd = yRot-e->yRot;
//        float xrdd = xRot-e->xRot;
//        
//        while (yrdd>=180) yrdd-=360;
//        while (yrdd<-180) yrdd+=360;
//        while (xrdd>=180) xrdd-=360;
//        while (xrdd<-180) xrdd+=360;
//        
//        float yrd = e->yRot+yrdd*f;
//        float xrd = e->xRot+xrdd*f;
//
//        while (yrd>=180) yrd-=360;
//        while (yrd<-180) yrd+=360;
//        while (xrd>=180) xrd-=360;
//        while (xrd<-180) xrd+=360;
//        
//        if (rot && move)
//        {
//            return /*new*/ EntityPos(xd, yd, zd, yrd, xrd);
//        }
//        if (move)
//        {
//            return /*new*/ EntityPos(xd, yd, zd);
//        }
//        if (rot)
//        {
//            return /*new*/ EntityPos(yrd, xrd);
//        }
//        return NULL;
//    }
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__EntityPos_H__*/
