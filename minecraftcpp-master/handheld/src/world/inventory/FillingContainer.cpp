#include "FillingContainer.h"
#include "../level/tile/TreeTile.h"
#include "../item/crafting/Recipe.h"
#include "../../util/Mth.h"
#include "../../nbt/CompoundTag.h"
#include "../level/tile/StoneSlabTile.h"

#define MAGIX_VAL 255
#define MAX_SLOTS 96

FillingContainer::FillingContainer(
		int numTotalSlots,
		int numLinkedSlots,
		int containerType,
		bool isCreative)
:	super(containerType),
	numTotalSlots(numTotalSlots),
	numLinkedSlots(numLinkedSlots),
	linkedSlots(NULL),
	_isCreative(isCreative)
{
	if (numLinkedSlots > 0)
		linkedSlots = new LinkedSlot[numLinkedSlots];
	
	items.resize(numTotalSlots);
}

FillingContainer::~FillingContainer()
{
	clearInventory();

	if (numLinkedSlots > 0)
		delete[] linkedSlots;
}

void FillingContainer::clearInventory()
{
	for (int i = 0; i < numLinkedSlots; ++i) {
		linkedSlots[i] = LinkedSlot(i);
	}

	//@todo: i = MAX_ -> get() transforms count=255-ptrs to real
	for (unsigned int i = numLinkedSlots; i < items.size(); i++) {
		release(i);
	}

	items.resize(numTotalSlots);
}

bool FillingContainer::removeResource( int type )
{
	if (_isCreative) return true;

	int slot = getSlot(type);
	if (slot < 0) return false;
	if (--items[slot]->count <= 0) release(slot);

	return true;
}

bool FillingContainer::removeResource( const ItemInstance& item ) {
	return removeResource(item, false) == 0;
}

int FillingContainer::removeResource( const ItemInstance& item, bool requireExactAux )
{
	if (_isCreative) return 0;

	int count = item.count;
	while (count > 0) {
		// If any AUX value, remove any with that id
		int slot = -1;
		if (!requireExactAux && (Recipe::isAnyAuxValue(&item) || item.getAuxValue() == Recipe::ANY_AUX_VALUE))
			slot = getNonEmptySlot(item.id);
		else
			slot = getNonEmptySlot(item.id, item.getAuxValue());

		if (slot < 0)
            return count;

		// Try to remove all items, but if it's not enough,
		// we continue on another slot next time
		ItemInstance* slotItem = items[slot];
		int toRemove = Mth::Min(count, slotItem->count);
		slotItem->count -= toRemove;
		count -= toRemove;

		if (slotItem->count <= 0)
			clearSlot(slot);
		//removeItem(slot, item.count);
	}
	return 0;
}

bool FillingContainer::hasResource( int type ) const
{
	if (_isCreative) return true;

	int slot = getSlot(type);
	if (slot < 0) return false;

	return true;
}

void FillingContainer::swapSlots( int from, int to )
{
	ItemInstance* tmp = items[to];
	items[to] = items[from];
	items[from] = tmp;
}

bool FillingContainer::add( ItemInstance* item )
{
	if (_isCreative) return true;

	if (!item || item->isNull()) return true;

	if (!item->isDamaged()) {
		int lastSize;
		do {
			lastSize = item->count;
			item->count = addResource(*item);
		} while (item->count > 0 && item->count < lastSize);
		//if (item->count == lastSize && player->abilities.instabuild) {
		//	// silently destroy the item when having a full inventory
		//	item->count = 0;
		//	return true;
		//}
		return item->count < lastSize;
	}

	int slot = getFreeSlot();
	if (slot >= 0) {
		items[slot] = ItemInstance::clone(item);
		//            items[slot] = item;
		//items[slot]->popTime = FillingContainer::POP_TIME_DURATION;
		linkEmptySlot(slot);
		item->count = 0;
		return true;
	//} else if (player->abilities.instabuild) {
	//	// silently destroy the item when having a full inventory
	//	item->count = 0;
	//	return true;
	}

	return false;
}

ItemInstance FillingContainer::removeItem( int slot, int count )
{
	ItemInstance* item = getItem(slot);
	if (item) {
		if (count > item->count)
			count = item->count;
		item->count -= count;

		if (item->count <= 0)
			clearSlot(slot);
	}
	
	/*
	ItemList& pile = *getSlotList(slot);
	if (pile[slot] != NULL) {
		if (pile[slot]->count <= count) {
			ItemInstance item = *pile[slot];
			release(slot);
			return item;
		} else {
			ItemInstance i = pile[slot]->remove(count);
			if (pile[slot]->count == 0) release(slot);
			return i;
		}
	}
	*/
	return ItemInstance();
}

void FillingContainer::setItem( int slot, ItemInstance* item )
{
	if (slot < 0 || slot >= numTotalSlots)
		return;

	if (ItemList* p = getSlotList(slot)) {
		ItemList& pile = *p;
		if (pile[slot]) *pile[slot] = item? *item : ItemInstance();
		else pile[slot] = new ItemInstance(item? *item : ItemInstance());
	}
}

ListTag* FillingContainer::save( ListTag* listTag )
{
	//LOGI("Save.inv-creative? %d. Size %d\n", _isCreative, items.size());
	if (!_isCreative) {
		ItemInstance selTmp;
		for (int i = 0; i < (int)items.size(); i++) {
			ItemInstance* item = items[i];

			if (i < numLinkedSlots) {
				int slot = linkedSlots[i].inventorySlot;
				selTmp = ItemInstance(MAGIX_VAL, MAGIX_VAL, slot);
				item = &selTmp;
			}
			if (item != NULL) {
				CompoundTag* tag = new CompoundTag();
				tag->putByte("Slot", (char) i);
                
				if (item->count < 0) item->count = 0;
				if (item->count > 255) item->count = 255;

                ItemInstance iitem(*item);
				iitem.save(tag);
				//LOGI("Saving %d as : %d@%d:%d\n", i, item->id, item->getAuxValue(), item->count);
				listTag->add(tag);
			}
		}
	}
	return listTag;
}

void FillingContainer::load( ListTag* inventoryList )
{
	//LOGI("Load.inv-creative? %d. Size %d\n", _isCreative, items.size());
	if (_isCreative)
		return;

	clearInventory();

	//items = /*new*/ ItemInstance[INVENTORY_SIZE];
	for (int i = inventoryList->size()-1; i >= 0; --i) {
		Tag* t = inventoryList->get(i);
		if (t->getId() != Tag::TAG_Compound) continue;

		CompoundTag* tag = (CompoundTag*) t;
        int slot = tag->getByte("Slot") & 0xff;
		if (slot < 0) continue;

		ItemInstance* item = ItemInstance::fromTag(tag);
		if (item != NULL) {
			//LOGI("Loading %d as : %d@%d:%d\n", slot, item->id, item->getAuxValue(), item->count);
			// Special case! (don't we love them!). item->count == 255 means
			// we POINT to our real inventory item (from a selection slot).
			// This is ONLY AND ALWAYS used in Selection Slots at this moment.
			if (slot < numLinkedSlots) {
				if (slot < (int)items.size() && item->id == MAGIX_VAL && item->count == MAGIX_VAL) {
					int invSlot = item->getAuxValue();
					if (invSlot >= numLinkedSlots && invSlot < (int)items.size())
						linkSlot(slot, invSlot, false);
				} else { // Something's wrong, shouldnt go here
					LOGE("Error: Slot %d is selection slot (inventory size: %d) but id/count is %d/%d\n", slot, (int)items.size(), item->id, item->count);
				}
				delete item;
				continue;
			}

			//item->count = 3;
			if (slot < MAX_SLOTS && slot >= numLinkedSlots)
			{
				// Zero count (valid before, but not anymore), destroy
				if (item->count == 0) {
					delete item;
					continue;
				}
				// Suggesting the container is larger than it is, drop the item!
				if (slot >= (int)items.size()) {
					doDrop(item, true);
					continue;
				}

				fixBackwardCompabilityItem(*item);
				items[slot] = item;
			}
			//else if (slot >= 100 && slot < armor.length + 100) armor[slot - 100] = item;
			else {
				if (slot >= MAX_SLOTS) {
					LOGE("Error: Too large slot ID in Inventory: %d!\n", slot);
				}
				delete item;
			}
		}
	}
	compressLinkedSlotList(0);
}

int FillingContainer::getContainerSize() const
{
	int sz = items.size();
	if (sz != numTotalSlots) {
		LOGE("Error@getContainerSize: num items != InventorySize: %d != %d\n", sz, numTotalSlots);
	}
	return numTotalSlots;
}

ItemInstance* FillingContainer::getItem( int slot )
{
	if (slot < 0 || slot >= numTotalSlots)
		return NULL;

	// Real inventory slot
	if (slot >= numLinkedSlots)
		return (*getSlotList(slot))[slot];

	return getLinked(slot); //@note
}

std::string FillingContainer::getName() const
{
	return "Inventory";
}

int FillingContainer::getMaxStackSize() const
{
	return MAX_INVENTORY_STACK_SIZE;
}

void FillingContainer::dropSlot( int slot, bool onlyClearContainer, bool randomly/*=false*/ )
{
	if (slot >= 0 && slot < numLinkedSlots)
		slot = linkedSlots[slot].inventorySlot;
	if (slot < 0 || slot >= (int)items.size()) return;

	if (items[slot] && items[slot]->count) {
		if (!onlyClearContainer)
			doDrop(items[slot]->copy(), randomly);
		items[slot]->count = 0;
		release(slot); //@itodo:
		compressLinkedSlotList(slot);
	}
}

void FillingContainer::dropAll(bool onlyClearContainer)
{
	for (unsigned int i = numLinkedSlots; i < items.size(); i++) {
		dropSlot(i, onlyClearContainer, true);
	}
}

void FillingContainer::setContainerChanged()
{
}

bool FillingContainer::stillValid( Player* player )
{
	return !player->removed;
}

bool FillingContainer::contains( ItemInstance* itemInstance ) const
{
	for (unsigned int i = 0; i < items.size(); i++) {
		if (items[i] != NULL && items[i]->matches(itemInstance)) return true;
	}
	return false;
}

int FillingContainer::getSlot( int tileId) const
{
	for (unsigned int i = numLinkedSlots; i < items.size(); i++) {
		if (items[i] != NULL && items[i]->id == tileId) return i;
	}
	return -1;
}

int FillingContainer::getSlot( int tileId, int data) const
{
	for (unsigned int i = numLinkedSlots; i < items.size(); i++) {
		if (items[i] != NULL
		 && items[i]->id == tileId
		 && items[i]->getAuxValue() == data)
			return i;
	}
	return -1;
}

int FillingContainer::getNonEmptySlot( int tileId) const
{
	for (unsigned int i = numLinkedSlots; i < items.size(); i++) {
		if (items[i] != NULL && items[i]->id == tileId && items[i]->count > 0) return i;
	}
	return -1;
}

int FillingContainer::getNonEmptySlot( int tileId, int data) const
{
	for (unsigned int i = numLinkedSlots; i < items.size(); i++) {
		if (items[i] != NULL
			&& items[i]->id == tileId
			&& items[i]->getAuxValue() == data
			&& items[i]->count > 0)
			return i;
	}
	return -1;
}

int FillingContainer::getNumEmptySlots() {
	int numEmpty = 0;
	for (unsigned int i = numLinkedSlots; i < items.size(); ++i) {
		if (!items[i] || items[i]->isNull())
			++numEmpty;
	}
	return numEmpty;
}

int FillingContainer::getNumLinkedSlots() {
	return numLinkedSlots;
}

int FillingContainer::getSlotWithRemainingSpace( const ItemInstance& item )
{
	for (unsigned int i = 0; i < items.size(); i++) {
		if (items[i] != NULL && items[i]->id == item.id
		 && items[i]->isStackable()
		 && items[i]->count < items[i]->getMaxStackSize()
		 && items[i]->count < getMaxStackSize()
		 && (!items[i]->isStackedByData() || items[i]->getAuxValue() == item.getAuxValue())) {
			return i;
		}
	}
	return -1;
}

int FillingContainer::addResource( const ItemInstance& itemInstance )
{
	int type = itemInstance.id;
	int count = itemInstance.count;

	if (itemInstance.getMaxStackSize() == 1) {
		int slot = getFreeSlot();
		if (slot < 0) return count;
		if (items[slot] == NULL) {
			items[slot] = ItemInstance::clone(&itemInstance);
			linkEmptySlot(slot);
		} else if (items[slot]->isNull()) {
			*items[slot] = itemInstance;
			linkEmptySlot(slot);
		}
		return 0;
	}

	int slot = getSlotWithRemainingSpace(itemInstance);
	if (slot < 0) slot = getFreeSlot();
	if (slot < 0) return count;
	if (items[slot] == NULL) {
		items[slot] = new ItemInstance(type, 0, itemInstance.getAuxValue());
	} else if (items[slot]->isNull()) { //@attn: added when doing chests
		*items[slot] = ItemInstance(type, 0, itemInstance.getAuxValue());
	}

	// Add the newly added item to a selections lot, if one's free
	linkEmptySlot(slot);

	int toAdd = count;

	if (toAdd > items[slot]->getMaxStackSize() - items[slot]->count) {
		toAdd = items[slot]->getMaxStackSize() - items[slot]->count;
	}

	if (toAdd > getMaxStackSize() - items[slot]->count) {
		toAdd = getMaxStackSize() - items[slot]->count;
	}

	if (toAdd == 0) return count;

	count -= toAdd;
	items[slot]->count += toAdd;
	//items[slot]->popTime = POP_TIME_DURATION;

	return count;
}

int FillingContainer::getFreeSlot() const
{
	for (unsigned int i = numLinkedSlots; i < items.size(); i++) {
		if (items[i] == NULL || items[i]->isNull()) return i;
	}
	return -1;
}

void FillingContainer::release( int slot )
{
	if (items[slot]) {
		delete items[slot];// @itodo
		items[slot] = NULL;
	}
}

void FillingContainer::clearSlot( int slot )
{
	if (slot < 0)
		return;

	if (slot < numLinkedSlots) {
		release(linkedSlots[slot].inventorySlot);
		linkedSlots[slot].inventorySlot = -1;
	}
	else {
		release(slot);
	}
	compressLinkedSlotList(slot);
}

int FillingContainer::addItem(ItemInstance* item) {
	for (unsigned int i = numLinkedSlots; i < items.size(); ++i)
		if (!items[i]) {
			items[i] = item;
			return i;
		}

	int newSize = items.size() + 1;
	if (_isCreative && newSize > numTotalSlots)
		numTotalSlots = newSize;
	if (newSize <= numTotalSlots) {
		items.push_back(item);
		return newSize-1;
	}
	LOGE("Error@addItem: adding an item to an already full inventory: %s!\n", item->getDescriptionId().c_str());
	delete item;
	return 0;
}

void FillingContainer::fixBackwardCompabilityItem( ItemInstance& item )
{
	if (item.id == Tile::stoneSlabHalf->id)
		item.setAuxValue(item.getAuxValue() & StoneSlabTile::TYPE_MASK);
}

void FillingContainer::replace( std::vector<ItemInstance> newItems, int maxCount /* = -1 */)
{
	clearInventory();
	maxCount = maxCount < 0 ? newItems.size() : Mth::Min(newItems.size(), maxCount);

	int base = numLinkedSlots;
	const int iMax = Mth::Min(getContainerSize() - base, maxCount);
	for (int i = 0; i < iMax; ++i) {
		replaceSlot(base + i, newItems[i].isNull()? NULL : &newItems[i]);
		//LOGI("Adding to slot: %d - %s :: %s\n", newItems[i], )
	}
}

void FillingContainer::replaceSlot( int slotId, ItemInstance* ins )
{
	if (ins) {
		if (!items[slotId])
			items[slotId] = new ItemInstance();
		*items[slotId] = *ins;
	} else {
		release(slotId);
	}
}

FillingContainer::ItemList* FillingContainer::getSlotList( int& slot )
{
	ItemList* pile = &items;
	//if (slot >= pile->size()) {
	//	slot -= pile->size();
	//	pile = armor;
	//}
	return pile;
}

// Special for this "selection based" inventory
bool FillingContainer::linkSlot(int selectionSlot, int inventorySlot, bool propagate) {
	if (selectionSlot < 0 || selectionSlot >= numLinkedSlots)
		return false;
	if (inventorySlot < numLinkedSlots || inventorySlot >= numTotalSlots)
		return false;
	if (inventorySlot == linkedSlots[selectionSlot].inventorySlot)
		return false;

	if (propagate) {
		int i = 0;
		for (; i < numLinkedSlots - 1; ++i)
			if (linkedSlots[i].inventorySlot == inventorySlot) break;
		for (; i > selectionSlot; --i) {
			linkedSlots[i].inventorySlot = linkedSlots[i-1].inventorySlot;
		}
	}
	linkedSlots[selectionSlot].inventorySlot = inventorySlot;

	return true;
}

bool FillingContainer::linkEmptySlot( int inventorySlot )
{
	// Check if we already got the inventory slot
	for (int i = 0; i < numLinkedSlots; ++i)
		if (linkedSlots[i].inventorySlot == inventorySlot) return true;

	// Check if there's an empty slot to place the new resource in
	for (int i = 0; i < numLinkedSlots; ++i) {
		ItemInstance* item = getLinked(i);
		if (!item) {
			linkedSlots[i].inventorySlot = inventorySlot;
			return true;
		}
	}
	return false;
}

void FillingContainer::compressLinkedSlotList(int slot)
{
	int i = slot-1, j = 0;
	while (++i < numLinkedSlots) {
		linkedSlots[i-j] = linkedSlots[i];

		ItemInstance* item = getLinked(i);
		if (!item) ++j;
	}
	for (int k = i-j; k < i; ++k)
		linkedSlots[k].inventorySlot = -1;
}

void FillingContainer::doDrop( ItemInstance* item, bool randomly )
{
	delete item;
}

ItemInstance* FillingContainer::getLinked( int slot )
{
	// sanity checking to prevent exploits
	if (slot < numLinkedSlots && slot >= 0) {
		int i = linkedSlots[slot].inventorySlot;
		return (i >= numLinkedSlots && i < numTotalSlots)? items[i] : NULL;
	}
	return NULL;
}
