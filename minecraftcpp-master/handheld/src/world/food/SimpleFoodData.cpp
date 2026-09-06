#include "SimpleFoodData.h"
#include "FoodConstants.h"
#include "../item/FoodItem.h"
#include "../entity/player/Player.h"

SimpleFoodData::SimpleFoodData()
:	foodLevel(0)
{
}

void SimpleFoodData::eat( int food ) {
	foodLevel = Mth::Min(food + foodLevel, FoodConstants::MAX_FOOD);
}

void SimpleFoodData::eat( FoodItem* item ) {
	eat(item->getNutrition());
}

void SimpleFoodData::tick( Player* player ) {
	if (foodLevel > 0 && player->isHurt()) {
		const int healAmount = foodLevel;// / 2;
		player->heal(healAmount);
		foodLevel = 0;
		/*
		if (!level->isClientSide) {
			player->heal(healAmount);
		} else {
			SetHealthPacket packet(SetHealthPacket::HEALTH_MODIFY_OFFSET - healAmount);
			level->raknetInstance->send(packet);
		}
		*/
	}
}
