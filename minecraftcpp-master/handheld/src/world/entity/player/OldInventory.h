#ifndef NET_MINECRAFT_WORLD_ENTITY_PLAYER__Inventory_H__
#define NET_MINECRAFT_WORLD_ENTITY_PLAYER__Inventory_H__

//package net.minecraft.world.entity.player;

class Player;
class Tile;

class Inventory
{
public:
    static const int POP_TIME_DURATION = 5;
    static const int MAX_SELECTION_SIZE = 9; // Including "More..." right now

	static const int INVENTORY_ROWS = 5;
	static const int INVENTORY_COLS = 9;
	static const int INVENTORY_SIZE = INVENTORY_COLS * INVENTORY_ROWS;
    static const int INVENTORY_SIZE_DEMO = 18;

    Inventory(Player* player_);

	void selectSlot(int slot);

    int getSelectedItemId();
	int getSelectionSlotItemId(int slot);

	void setSelectionSlotItemId(int slot, int id);

	float getDestroySpeed(Tile* tile);


	//int getCurrentSelectionSize();
	//void setCurrentSelectionSize(int size) { _selectionSize = size; }

	int selected;
protected:
	//int _selectionSize;
	Player* player;
	
	int itemIds[MAX_SELECTION_SIZE];
	int inventoryIds[INVENTORY_SIZE];
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PLAYER__Inventory_H__*/
