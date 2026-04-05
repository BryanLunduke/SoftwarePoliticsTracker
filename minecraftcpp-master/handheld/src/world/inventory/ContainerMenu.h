#ifndef NET_MINECRAFT_WORLD_INVENTORY__ContainerMenu_H__
#define NET_MINECRAFT_WORLD_INVENTORY__ContainerMenu_H__

//package net.minecraft.world.inventory;

#include "BaseContainerMenu.h"
#include <vector>

class Container;
class ItemInstance;

class ContainerMenu: public BaseContainerMenu
{
	typedef BaseContainerMenu super;
public:
    ContainerMenu(Container* container, int tileEntityId = -1);

    virtual ~ContainerMenu() {}

    virtual void setSlot(int slot, ItemInstance* item);
	virtual std::vector<ItemInstance> getItems();

	virtual bool tileEntityDestroyedIsInvalid( int tileEntityId );

	Container* container;
protected:
	int tileEntityId;
};

#endif /*NET_MINECRAFT_WORLD_INVENTORY__ContainerMenu_H__*/
