#include "BaseContainerMenu.h"
#include "../item/ItemInstance.h"

BaseContainerMenu::BaseContainerMenu( int containerType )
:   containerId(-1),
	containerType(containerType),
	listener(NULL)
{
}

void BaseContainerMenu::setListener( IContainerListener* listener )
{
	if (!(this->listener = listener))
		return;

	if (listener)
		listener->refreshContainer(this, lastSlots = getItems());
}

void BaseContainerMenu::broadcastChanges()
{
	//LOGI("broadcast: Base. Listener: %p\n", listener);
	if (!listener)
		return;

	ItemList slots = getItems();
	if (slots.size() != lastSlots.size()) {
		listener->refreshContainer(this, lastSlots = slots);
		return;
	}

	for (unsigned int i = 0; i < slots.size(); i++) {
		ItemInstance& current = slots[i];
		ItemInstance& expected = lastSlots[i];
		if (!ItemInstance::matches(&expected, &current)) {
			expected = current;
			//LOGI("Broadcasting a change!\n");
			listener->slotChanged(this, i, expected, isResultSlot(i));
		}
	}
}
