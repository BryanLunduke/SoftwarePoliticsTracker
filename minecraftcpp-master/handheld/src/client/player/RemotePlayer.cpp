#include "RemotePlayer.h"
#include "../../world/entity/player/Inventory.h"
#include "../../world/entity/SharedFlags.h"

RemotePlayer::RemotePlayer(Level* level, bool isCreative)
:	Player(level, isCreative),
	hasStartedUsingItem(false)
{
	hasFakeInventory = true;
}

void RemotePlayer::tick() {
	super::tick();

	if (!hasStartedUsingItem && getSharedFlag(SharedFlagsInformation::FLAG_USINGITEM)) {
		hasStartedUsingItem = true;
		ItemInstance* instance = inventory->getItem(Inventory::MAX_SELECTION_SIZE);
		if (instance) {
			Item* item = Item::items[instance->id];
			if (item) {
				startUsingItem(*instance, item->getUseDuration(instance));
				//LOGI("Start using item! %s\n", instance->getDescriptionId().c_str());
			}
		}
	} else if (hasStartedUsingItem && !getSharedFlag(SharedFlagsInformation::FLAG_USINGITEM)) {
//		LOGI("Stop using item! %s\n", useItem.getDescriptionId().c_str());
		stopUsingItem();
		hasStartedUsingItem = false;
	}
}

void RemotePlayer::aiStep() {
	updateAttackAnim();
	super::aiStep();
}
