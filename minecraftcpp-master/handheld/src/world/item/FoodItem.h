#ifndef NET_MINECRAFT_WORLD_ITEM__FoodItem_H__
#define NET_MINECRAFT_WORLD_ITEM__FoodItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../level/Level.h"
#include "../entity/player/Player.h"
#include "../../SharedConstants.h"
#include "../../network/packet/SetHealthPacket.h"
//#include "../effect/MobEffectInstance.h"
//#include "../food/FoodConstants.h"

class FoodItem: public Item
{
	typedef Item super;
    static const int EAT_DURATION = (int) (20 * 1.6);
public:
    FoodItem(int id, int nutrition, bool isMeat, float saturationMod = 0.6f)
    :   super(id),
        nutrition(nutrition),
        _isMeat(isMeat),
        saturationModifier(saturationMod)
    {
	}

	bool isFood() const {
		return true;
	}

	ItemInstance useTimeDepleted(ItemInstance* instance, Level* level, Player* player) {
		instance->count--;
		player->foodData.eat(this);
		level->playSound(player, "random.burp", 0.5f, level->random.nextFloat() * 0.1f + 0.9f);
		return *instance;
	}

    int getUseDuration(ItemInstance* itemInstance) {
        return EAT_DURATION;
    }

    UseAnim::UseAnimation getUseAnimation() {
        return UseAnim::eat;
    }

	ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
		if (!player->abilities.invulnerable && player->isHurt()) {
			player->startUsingItem(*instance, getUseDuration(instance));
		}
		return instance;
	}

    int getNutrition() {
        return nutrition;
    }

	/*
    float getSaturationModifier() {
        return saturationModifier;
    }
	*/

    bool isMeat() {
        return _isMeat;
    }

	/*
    FoodItem* setEatEffect(int id, int durationInSecods, int amplifier, float effectProbability) {
        effectId = id;
        effectDurationSeconds = durationInSecods;
        effectAmplifier = amplifier;
        this->effectProbability = effectProbability;
        return this;
    }
	*/

    FoodItem* setCanAlwaysEat() {
        canAlwaysEat = true;
        return this;
    }
private:
    const int nutrition;
    const float saturationModifier;
    const bool _isMeat;
    bool canAlwaysEat;
	/*

    int effectId;
    int effectDurationSeconds;
    int effectAmplifier;
    float effectProbability;
	*/
};

#endif /*NET_MINECRAFT_WORLD_ITEM__FoodItem_H__*/
