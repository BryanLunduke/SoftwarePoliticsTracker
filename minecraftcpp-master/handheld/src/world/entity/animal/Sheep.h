#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Sheep_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Sheep_H__

//package net.minecraft.world.entity.animal;

#include "Animal.h"

#include "../EntityEvent.h"
#include "../item/ItemEntity.h"
#include "../player/Player.h"
#include "../../item/ItemInstance.h"
#include "../../level/Level.h"
#include "../../../SharedConstants.h"
#include "../../../util/Mth.h"

#include "../../item/Item.h"

#include "../../../nbt/CompoundTag.h"

class Sheep: public Animal
{
	typedef Animal super;

	static const int EAT_ANIMATION_TICKS = SharedConstants::TicksPerSecond * 2;
    static const int DATA_WOOL_ID = 16;

public:
    static const float COLOR[][3];
    static const int NumColors;

    Sheep(Level* level);

    /*@Override*/
    int getMaxHealth();

	/*@Override*/
    void aiStep();

    //*@Override*/
	void handleEntityEvent(char id);

    float getHeadEatPositionScale(float a);
    float getHeadEatAngleScale(float a);

    /*@Override*/
    bool interact(Player* player);

    void addAdditonalSaveData(CompoundTag* tag);
    void readAdditionalSaveData(CompoundTag* tag);

    int getColor() const;
    void setColor(int color);

	static int getSheepColor(Random* random);

	bool isSheared() const;
    void setSheared(bool value);

	int getEntityTypeId() const;
protected:
    /*@Override*/
    void dropDeathLoot(/*bool wasKilledByPlayer, int playerBonusLevel*/);
    /*@Override*/
    int getDeathLoot();

    /*@Override*/
    void jumpFromGround();

    /*@Override*/
    void updateAi();

    /*@Override*/
    bool shouldHoldGround();

    const char* getAmbientSound();
    std::string getHurtSound();
    std::string getDeathSound();

//     /*@Override*/
//     Animal getBreedOffspring(Animal target) {
//         Sheep otherSheep = (Sheep) target;
//         Sheep sheep = /*new*/ Sheep(level);
//         if (random.nextBoolean()) sheep.setColor(getColor());
//         else sheep.setColor(otherSheep.getColor());
//         return sheep;
//     }
private:
    int eatAnimationTick;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Sheep_H__*/
