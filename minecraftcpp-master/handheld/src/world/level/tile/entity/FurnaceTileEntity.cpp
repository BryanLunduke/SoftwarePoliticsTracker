#include "FurnaceTileEntity.h"

#include "../FurnaceTile.h"
#include "../../Level.h"
#include "../../material/Material.h"
#include "../../../Container.h"
#include "../../../entity/player/Player.h"
//#include "../../../item/crafting/FurnaceRecipes.h"
#include "../../../item/ItemInstance.h"
#include "../../../../nbt/ListTag.h"
#include "../../../item/crafting/FurnaceRecipes.h"

FurnaceTileEntity::FurnaceTileEntity()
:   super(TileEntityType::Furnace),
	Container(ContainerType::FURNACE),
	litTime(0),
	litDuration(0),
	tickCount(0),
	finished(false),
	_canBeFinished(false)
{
	//LOGI("CREATING FurnaceTileEntity! %p\n", this);
}

FurnaceTileEntity::~FurnaceTileEntity() {
	//LOGI("DELETING FurnaceTileEntity! %p @ %d, %d, %d\n", this, x, y, z);
}

ItemInstance* FurnaceTileEntity::getItem(int slot) {// @todo @container @fix
	return &items[slot];
}

ItemInstance FurnaceTileEntity::removeItem(int slot, int count) {
	if (!items[slot].isNull()) {
		if (items[slot].count <= count) {
			ItemInstance item = items[slot];
			items[slot].setNull();
			return item;
		} else {
			ItemInstance i = items[slot].remove(count);
			if (items[slot].count == 0) items[slot].setNull();
			return i;
		}
	}
	return ItemInstance();
}

void FurnaceTileEntity::setItem(int slot, ItemInstance* item) {
    items[slot] = *item;
    if (item != NULL && item->count > getMaxStackSize())
		items[slot].count = getMaxStackSize();
	//LOGI("Furnace: Setting slot %d : %s\n", slot, item->getDescriptionId().c_str());
}

std::string FurnaceTileEntity::getName() const {
    return "Furnace";
}

bool FurnaceTileEntity::shouldSave() {
	if (litTime > 0) return true;
	for (int i = 0; i < NumItems; ++i)
		if (!items[i].isNull()) return true;
	return false;
}

void FurnaceTileEntity::load(CompoundTag* base) {
	super::load(base);
	ListTag* inventoryList = base->getList("Items");
	for (int i = 0; i < 3; ++i)
		items[i].setNull();
	for (int i = 0; i < inventoryList->size(); i++) {
		Tag* tt = inventoryList->get(i);
		if (tt->getId() == Tag::TAG_Compound) {
			CompoundTag* tag = (CompoundTag*)tt;
			int slot = tag->getByte("Slot");
			if (slot >= 0 && slot < NumItems) {
				ItemInstance* loaded = ItemInstance::fromTag(tag);
				if (loaded) {
					items[slot] = *loaded;
					delete loaded;
				} else
					items[slot].setNull();
			}
		} else {
			LOGE("load @ FurnaceTileEntity failed. item's not a compoundTag!\n");
		}
	}

	litTime = base->getShort("BurnTime");
	tickCount = base->getShort("CookTime");
	litDuration = getBurnDuration(items[SLOT_FUEL]);
}

bool FurnaceTileEntity::save(CompoundTag* base) {
	if (!super::save(base))
		return false;

	base->putShort("BurnTime", (short) (litTime));
	base->putShort("CookTime", (short) (tickCount));
	ListTag* listTag = new ListTag();

	for (int i = 0; i < NumItems; i++) {
		if (!items[i].isNull()) {
			CompoundTag* tag = new CompoundTag();
			tag->putByte("Slot", (char) i);
			items[i].save(tag);
			listTag->add(tag);
		}
	}
	base->put("Items", listTag);
	return true;
}

int FurnaceTileEntity::getMaxStackSize() const {
	return Container::LARGE_MAX_STACK_SIZE;
}

int FurnaceTileEntity::getContainerSize() const {
	return NumItems;
}

int FurnaceTileEntity::getBurnProgress(int max) {
	return tickCount * max / BURN_INTERVAL;
}

int FurnaceTileEntity::getLitProgress(int max) {
	if (litDuration == 0) litDuration = BURN_INTERVAL;
	return litTime * max / litDuration;
}

bool FurnaceTileEntity::isLit() {
	return litTime > 0;
}

void FurnaceTileEntity::tick()
{
	//LOGI("lit|time, tick, dur: %d, %d, %d\n", litTime, tickCount, litDuration);

	bool wasLit = litTime > 0;
	bool changed = false;
	if (litTime > 0) {
		--litTime;
	}

	//LOGI("Ticking FurnaceTileEntity: %d\n", litTime);

	if (!level->isClientSide) {
		if (litTime == 0 && canBurn()) {
			litDuration = litTime = getBurnDuration(items[SLOT_FUEL]);
			if (litTime > 0) {
				changed = true;
				if (!items[SLOT_FUEL].isNull()) {
					if (--items[SLOT_FUEL].count == 0) items[SLOT_FUEL].setNull();
				}
			}
		}

		if (isLit() && canBurn()) {
			if (++tickCount == BURN_INTERVAL) {
				tickCount = 0;
				burn();
				changed = true;
			}
		} else {
			tickCount = 0;
		}

		if (wasLit != (litTime > 0)) {
			changed = true;
			FurnaceTile::setLit(litTime > 0, level, x, y, z);
		}
	}

	if (changed) setChanged();
	else {
		if (!wasLit) {
			finished = true;
		}
	}
}

bool FurnaceTileEntity::isFinished()
{
	return _canBeFinished && finished;
}


void FurnaceTileEntity::burn() {
	if (!canBurn()) return;

	ItemInstance result = FurnaceRecipes::getInstance()->getResult(items[0].getItem()->id);
	if (items[2].isNull()) items[2] = result;
	else if (items[2].id == result.id) items[2].count++;

	if (--items[0].count <= 0) items[0].setNull();
}

bool FurnaceTileEntity::stillValid(Player* player) {
	if (level->getTileEntity(x, y, z) != this) return false;
	if (player->distanceToSqr(x + 0.5f, y + 0.5f, z + 0.5f) > 8 * 8) return false;
	return true;
}

void FurnaceTileEntity::startOpen() {
	//_canBeFinished = false;
	// TODO Auto-generated method stub
}

void FurnaceTileEntity::stopOpen() {
	//_canBeFinished = true;
	// TODO Auto-generated method stub
}

bool FurnaceTileEntity::canBurn() {
	if (items[SLOT_INGREDIENT].isNull()) return false;
	ItemInstance burnResult = FurnaceRecipes::getInstance()->getResult(items[SLOT_INGREDIENT].getItem()->id);
	if (burnResult.isNull()) return false;
	if (items[SLOT_RESULT].isNull()) return true;
	if (!items[SLOT_RESULT].sameItem(&burnResult)) return false;
	if (items[SLOT_RESULT].count < getMaxStackSize() && items[SLOT_RESULT].count < items[SLOT_RESULT].getMaxStackSize()) return true;
	if (items[SLOT_RESULT].count < burnResult.getMaxStackSize()) return true;
	return false;
}

/*static*/
int FurnaceTileEntity::getBurnDuration(const ItemInstance& itemInstance) {
	//if (itemInstance == NULL) return 0;
	if (itemInstance.isNull()) return 0;
	int id = itemInstance.getItem()->id;

	if (id < 256 && Tile::tiles[id]->material == Material::wood)
		return BURN_INTERVAL * 3 / 2;

	if (id == Item::stick->id)  return BURN_INTERVAL / 2;
	if (id == Item::coal->id)   return BURN_INTERVAL * 8;
		//case Item::bucket_lava->id: return BURN_INTERVAL * 100;
		//case Tile::sapling->id:     return BURN_INTERVAL / 2;
		//case Item::blazeRod->id:    return BURN_INTERVAL * 12;

	return 0;
}

bool FurnaceTileEntity::isFuel( const ItemInstance& itemInstance )
{
	return getBurnDuration(itemInstance) > 0;
}

bool FurnaceTileEntity::isSlotEmpty( int slot )
{
	return items[slot].isNull();
}
