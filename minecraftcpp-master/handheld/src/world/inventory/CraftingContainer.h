#ifndef NET_MINECRAFT_WORLD_INVENTORY__None_H__
#define NET_MINECRAFT_WORLD_INVENTORY__None_H__

//package net.minecraft.world.inventory;

#include "../Container.h"
#include "../entity/player/Player.h"
#include "../item/ItemInstance.h"
//#include "AbstractContainerMenu.h"

class CraftingContainer: public Container
{
	typedef Container super;
public:
    CraftingContainer(/*AbstractContainerMenu* menu,*/ int w, int h)
	:	super(ContainerType::WORKBENCH),
		size(w * h),
		width(w)/*,
		menu(menu)*/
	{
        items = new ItemInstance[size];
    }

	~CraftingContainer() {
		delete[] items;
	}

    ItemInstance* getItem(int slot) {
        if (slot >= getContainerSize()) {
            return NULL;
        }
        return &items[slot];
    }

    ItemInstance* getItem(int x, int y) {
        if (x < 0 || x >= width) {
            return NULL;
        }
        int pos = x + y * width;
        return getItem(pos);
    }

	//@itodo
    void setItem(int slot, const ItemInstance& item) {
        items[slot] = item;
        //menu->slotsChanged(this);
    }

    ItemInstance removeItem(int slot, int count) {
        if (!items[slot].isNull()) {
            if (items[slot].count <= count) {
                ItemInstance item = items[slot];
                items[slot] = ItemInstance();
                //menu->slotsChanged(this);
                return item;
            } else {
                ItemInstance i = items[slot].remove(count);
                if (items[slot].count == 0) items[slot] = ItemInstance();
                //menu->slotsChanged(this);
                return i;
            }
        }
        return ItemInstance();
    }

    int getContainerSize() const {
        return size;
    }
    int getMaxStackSize() const {
        return Container::LARGE_MAX_STACK_SIZE;
    }
    std::string getName() const {
        return "Crafting";
    }

    void setContainerChanged() {}

    bool stillValid(Player* player) {
        return true;
    }

    void startOpen() {}
    void stopOpen() {}

private:
    ItemInstance* items;
	int size;
    int width;
    //AbstractContainerMenu* menu;
};

#endif /*NET_MINECRAFT_WORLD_INVENTORY__None_H__*/
