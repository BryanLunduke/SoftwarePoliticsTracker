#ifndef NET_MINECRAFT_WORLD_INVENTORY__FILLINGCONTAINER_H__
#define NET_MINECRAFT_WORLD_INVENTORY__FILLINGCONTAINER_H__

#include "../Container.h"

#include <vector>

class Player;
class ListTag;
class CompoundTag;

class FillingContainer: public Container
{
	typedef Container super;
protected:
	typedef std::vector<ItemInstance*> ItemList;
public:
	static const int MAX_INVENTORY_STACK_SIZE = 254;

	FillingContainer(int numTotalSlots, int numLinkedSlots, int containerType, bool isCreative);
	~FillingContainer();

	bool			hasResource(int type) const;
	bool			removeResource(int type);
	bool			removeResource(const ItemInstance& item);
	int				removeResource(const ItemInstance& item, bool requireExactAux);
	void			swapSlots(int from, int to);

	virtual bool	add(ItemInstance* item);
	ItemList*		getSlotList(int& slot);

	void			dropSlot(int slot, bool onlyClearContainer, bool randomly=false);
	void			dropAll(bool onlyClearContainer);
	// @itodo: keep return by value?
	ItemInstance	removeItem(int slot, int count);

	void			clearSlot( int slot );
	void			clearInventory();
	void			clearInventoryWithDefault();

	ListTag*		save(ListTag* listTag);
	void			load(ListTag* inventoryList);
	void			replace( std::vector<ItemInstance> newItems, int count = -1 );
	void			replaceSlot(int slotId, ItemInstance* ins);

	void			setItem(int slot, ItemInstance* item);
	ItemInstance*	getItem(int slot);

	std::string		getName() const;
	int				getContainerSize() const;
	int				getMaxStackSize() const;

	void			setContainerChanged();

	bool stillValid(Player* player);
	bool contains(ItemInstance* itemInstance) const;

	void startOpen() {}
	void stopOpen() {}

	virtual void doDrop(ItemInstance* item, bool randomly);

	int getSlot(int tileId, int data) const;
	int getNonEmptySlot(int tileId, int data) const;
	int getNumEmptySlots();
	int getNumLinkedSlots();
protected:
	int getSlot(int tileId) const;
	int getNonEmptySlot(int tileId) const;
	int getSlotWithRemainingSpace(const ItemInstance& item);
	int getFreeSlot() const;

	int addResource(const ItemInstance& itemInstance);
	int	addItem(ItemInstance* item);

	void release(int slot);
	void fixBackwardCompabilityItem( ItemInstance& item );
public:
	//
	// Linked slots
	//
	bool linkSlot(int selectionSlot, int inventorySlot, bool propagate);
	bool linkEmptySlot(int inventorySlot);
	ItemInstance* getLinked(int slot);
	void compressLinkedSlotList(int slot);

	typedef struct LinkedSlot {
		LinkedSlot()
			:	inventorySlot(-1)
		{}
		LinkedSlot(int slot)
			:	inventorySlot(slot)
		{}

		int inventorySlot;
	} LinkedSlot;
	LinkedSlot* linkedSlots;

	int numTotalSlots;
	const int numLinkedSlots;
protected:
	ItemList items;

	bool _isCreative;
};

#endif /*NET_MINECRAFT_WORLD_INVENTORY__FILLINGCONTAINER_H__*/
