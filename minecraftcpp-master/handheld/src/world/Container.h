#ifndef NET_MINECRAFT_WORLD__Container_H__
#define NET_MINECRAFT_WORLD__Container_H__

//package net.minecraft.world;

#include "item/ItemInstance.h"
#include <string>
#include <vector>

class Player;

class ContainerType {
public:
	static const int NONE          = -9;
	static const int INVENTORY     = -1;
    static const int CONTAINER     = 0;
    static const int WORKBENCH     = 1;
    static const int FURNACE       = 2;
    static const int TRAP          = 3;
    static const int ENCHANTMENT   = 4;
    static const int BREWING_STAND = 5;
};

class Container
{
public:
    static const int LARGE_MAX_STACK_SIZE = 64;

	Container(int containerType)
	:	containerId(-1),
		containerType(containerType)
	{}
	virtual ~Container() {}

    virtual ItemInstance* getItem(int slot) = 0;
    virtual void setItem(int slot, ItemInstance* item) = 0;
    virtual ItemInstance removeItem(int slot, int i) = 0;

    virtual std::string getName() const = 0;
    virtual int getContainerSize() const = 0;
    virtual int getMaxStackSize() const = 0;

    virtual bool stillValid(Player* player) = 0;

    virtual void startOpen() = 0;
    virtual void stopOpen() = 0;

	virtual std::vector<ItemInstance> getSlotCopies() {
		std::vector<ItemInstance> items;
		ItemInstance NullItem;
		for (int i = 0; i < getContainerSize(); ++i) {
			ItemInstance* item = getItem(i);
			items.push_back( item? *item : NullItem );
		}
		return items;
	}

	int containerId;
	int containerType;
};

#endif /*NET_MINECRAFT_WORLD__Container_H__*/
