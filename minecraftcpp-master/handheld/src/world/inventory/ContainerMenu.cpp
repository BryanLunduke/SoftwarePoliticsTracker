#include "ContainerMenu.h"
#include "../Container.h"
#include "../entity/player/Player.h"
#include "../item/ItemInstance.h"

ContainerMenu::ContainerMenu( Container* container, int tileEntityId /* = -1 */ )
:   super(ContainerType::CONTAINER),
	container(container),
	tileEntityId(tileEntityId)
{
}

void ContainerMenu::setSlot( int slot, ItemInstance* item )
{
	container->setItem(slot, item);
}

std::vector<ItemInstance> ContainerMenu::getItems()
{
	std::vector<ItemInstance> out;
	for (int i = 0; i < container->getContainerSize(); ++i) {
		ItemInstance* item = container->getItem(i);
		out.push_back(item? *item : ItemInstance());
	}
	return out;
}

bool ContainerMenu::tileEntityDestroyedIsInvalid( int tileEntityId )
{
	return (this->tileEntityId == tileEntityId);
	/*
	return (this->tileEntityId >= 0
		&&	this->tileEntityId == tileEntityId);
		*/
}
