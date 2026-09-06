#ifndef NET_MINECRAFT_WORLD_INVENTORY__BaseContainerMenu_H__
#define NET_MINECRAFT_WORLD_INVENTORY__BaseContainerMenu_H__

//package net.minecraft.world.inventory;

#include <vector>
#include "../item/ItemInstance.h"

class BaseContainerMenu;

class IContainerListener {
public:
    virtual ~IContainerListener() {}
    virtual void refreshContainer(BaseContainerMenu* menu, const std::vector<ItemInstance>& items) {}
    virtual void slotChanged(BaseContainerMenu* menu, int slot, const ItemInstance& item, bool isResultSlot) {}
    virtual void setContainerData(BaseContainerMenu* menu, int id, int value) {}
};

class TileEntity;

class BaseContainerMenu {
public:
	typedef std::vector<ItemInstance> ItemList;

    BaseContainerMenu(int containerType);
    virtual ~BaseContainerMenu() {}

    virtual ItemList getItems() = 0;

	virtual void setSlot(int slot, ItemInstance* item) = 0;
	virtual void setData( int id, int value ) {}

	virtual void setListener(IContainerListener* listener);
	virtual void broadcastChanges();

	virtual bool isResultSlot(int slot) { return false; }
	//virtual bool stillValid( Player* player );
	virtual bool tileEntityDestroyedIsInvalid( int tileEntityId ) = 0;

    int containerId;
	int containerType;
    IContainerListener* listener;

	ItemList lastSlots;
};

#endif /*NET_MINECRAFT_WORLD_INVENTORY__BaseContainerMenu_H__*/
