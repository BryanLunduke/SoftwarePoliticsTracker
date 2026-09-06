#ifndef NET_MINECRAFT_WORLD_INVENTORY__FurnaceMenu_H__
#define NET_MINECRAFT_WORLD_INVENTORY__FurnaceMenu_H__

//package net.minecraft.world.inventory;

#include "BaseContainerMenu.h"
#include <vector>

class FurnaceTileEntity;
class ItemInstance;

class FurnaceMenu: public BaseContainerMenu
{
	typedef BaseContainerMenu super;
public:
    FurnaceMenu(FurnaceTileEntity* furnace);

    virtual ~FurnaceMenu() {}

    virtual void setSlot(int slot, ItemInstance* item);
	virtual void setData(int id, int value);
	virtual std::vector<ItemInstance> getItems();

	virtual void broadcastChanges();
	virtual void setListener(IContainerListener* listener);

	virtual bool tileEntityDestroyedIsInvalid( int tileEntityId );

	FurnaceTileEntity* furnace;

	int furnaceTileEntityId;
private:
	int lastTickCount;
	int lastLitTime;
	int lastLitDuration;
};

#endif /*NET_MINECRAFT_WORLD_INVENTORY__FurnaceMenu_H__*/
