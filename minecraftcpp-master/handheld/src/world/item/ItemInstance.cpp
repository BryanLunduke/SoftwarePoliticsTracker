#include "ItemInstance.h"
#include "Item.h"
#include "../level/tile/Tile.h"
#include "../../nbt/CompoundTag.h"

ItemInstance::ItemInstance() {
	init(0, 0, 0);
}

ItemInstance::ItemInstance(const Tile* tile) {
    init(tile->id, 1, 0);
}

ItemInstance::ItemInstance(const Tile* tile, int count) {
    init(tile->id, count, 0);
}

ItemInstance::ItemInstance(const Tile* tile, int count, int auxValue) {
    init(tile->id, count, auxValue);
}

ItemInstance::ItemInstance(const Item* item) {
    init(item->id, 1, 0);
}

ItemInstance::ItemInstance(const Item* item, int count) {
    init(item->id, count, 0);
}

ItemInstance::ItemInstance(const Item* item, int count, int auxValue) {
    init(item->id, count, auxValue);
}

ItemInstance::ItemInstance(int id, int count, int damage) {
	init(id, count, damage);
}

ItemInstance::ItemInstance(const ItemInstance& rhs) {
	this->auxValue	= rhs.auxValue;
	this->count		= rhs.count;
	//this->popTime	= rhs.popTime;
	this->id		= rhs.id;
}

void ItemInstance::init(int id, int count, int damage) {
	this->id = id;
	this->count = count;
	this->auxValue = damage;
}

bool ItemInstance::isNull() const {
	return (id|count|auxValue) == 0;
}

void ItemInstance::setNull() {
	id = count = auxValue = 0;
}

//ItemInstance::ItemInstance(CompoundTag itemTag) {
//    load(itemTag);
//}

ItemInstance ItemInstance::remove(int count) {
    this->count -= count;
    return /*new*/ ItemInstance(id, count, auxValue);
}

bool ItemInstance::useOn(Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
    if (getItem()->useOn(this, player, level, x, y, z, face, clickX, clickY, clickZ)) {
		//player.awardStat(Stats.itemUsed[id], 1);
		return true;
	}
    return false;
}

float ItemInstance::getDestroySpeed(Tile* tile) {
    return getItem()->getDestroySpeed(this, tile);
}

ItemInstance* ItemInstance::use(Level* level, Player* player) {
    return getItem()->use(this, level, player);
}

int ItemInstance::getMaxStackSize() const {
    return getItem()->getMaxStackSize();
}

bool ItemInstance::isStackable() const {
    return getMaxStackSize() > 1 && (!isDamageableItem() || !isDamaged());
}

bool ItemInstance::isStackable( const ItemInstance* a, const ItemInstance* b ) {
	return a && b && a->id == b->id && b->isStackable()
		&& (!b->isStackedByData() || a->getAuxValue() == b->getAuxValue());
}

bool ItemInstance::isDamageableItem() const {
    return Item::items[id]->getMaxDamage() > 0;
}

/**
 * Returns true if this item type only can be stacked with items that have
 * the same auxValue data.
 * 
 * @return
 */
bool ItemInstance::isStackedByData() const {
    return Item::items[id]->isStackedByData();
}

bool ItemInstance::isDamaged() const {
    return isDamageableItem() && auxValue > 0;
}

int ItemInstance::getDamageValue() const {
    return auxValue;
}

int ItemInstance::getAuxValue() const {
    return auxValue;
}
void ItemInstance::setAuxValue(int value) {
	auxValue = value;
}

int ItemInstance::getMaxDamage() const {
    return Item::items[id]->getMaxDamage();
}

void ItemInstance::hurt(int i) {
    if (!isDamageableItem())
        return;

    auxValue += i;
    if (auxValue > getMaxDamage()) {
        count--;
        if (count < 0) count = 0;
        auxValue = 0;
    }
}

void ItemInstance::hurtEnemy(Mob* mob) {
    Item::items[id]->hurtEnemy(this, mob);
}

void ItemInstance::mineBlock(int tile, int x, int y, int z) {
    Item::items[id]->mineBlock(this, tile, x, y, z);
}

int ItemInstance::getAttackDamage(Entity* entity) {
    return Item::items[id]->getAttackDamage(entity);
}

bool ItemInstance::canDestroySpecial(Tile* tile) {
    return Item::items[id]->canDestroySpecial(tile);
}

void ItemInstance::snap(Player* player) {
}

void ItemInstance::interactEnemy(Mob* mob) {
    Item::items[id]->interactEnemy(this, mob);
}

ItemInstance* ItemInstance::copy() const {
    return new ItemInstance(id, count, auxValue);
}

/*static*/
bool ItemInstance::matches(const ItemInstance* a, const ItemInstance* b) {
    if (a == NULL && b == NULL) return true;
    if (a == NULL || b == NULL) return false;
    return a->matches(b);
}

/*static*/
bool ItemInstance::matchesNulls(const ItemInstance* a, const ItemInstance* b) {
	bool aNull = !a || a->isNull();
	bool bNull = !b || b->isNull();
	if (aNull && bNull) return true;
	if (aNull || bNull) return false;
	return a->matches(b);
}

/**
 * Checks if this item is the same item as the other one, disregarding the
 * 'count' value.
 * 
 * @param b
 * @return
 */
bool ItemInstance::sameItem(ItemInstance* b) {
    return id == b->id && auxValue == b->auxValue;
}

std::string ItemInstance::getDescriptionId() const {
    return id? Item::items[id]->getDescriptionId(this) : "EmptyItemInstance";
}

ItemInstance* ItemInstance::setDescriptionId(const std::string& id) {
    return this;
}

std::string ItemInstance::getName() const {
	return I18n::get(getDescriptionId() + ".name");
}

std::string ItemInstance::toString() const {
	std::stringstream ss;
    ss << count << " x " << getDescriptionId() << "(" << id << ")" << "@" << auxValue;
	return ss.str();
}

/*static*/
ItemInstance* ItemInstance::clone(const ItemInstance* item) {
    return item == NULL ? NULL : item->copy();
}

/*static*/
ItemInstance ItemInstance::cloneSafe( const ItemInstance* item ) {
	return item? *item : ItemInstance();
}

/*private*/
bool ItemInstance::matches(const ItemInstance* b) const {
    return (id == b->id)
		&& (count == b->count)
		&& (auxValue == b->auxValue);
}

CompoundTag* ItemInstance::save(CompoundTag* compoundTag) {
    compoundTag->putShort("id", (short) id);
    compoundTag->putByte("Count", (unsigned char) count);
    compoundTag->putShort("Damage", (short) auxValue);
    return compoundTag;
}

void ItemInstance::load(CompoundTag* compoundTag) {
    id = compoundTag->getShort("id");
    count = (unsigned char) compoundTag->getByte("Count");
    auxValue = compoundTag->getShort("Damage");
}

bool ItemInstance::operator==( const ItemInstance& rhs ) const {
	return id		== rhs.id
		&& auxValue == rhs.auxValue
		&& count    == rhs.count;
}

ItemInstance* ItemInstance::fromTag( CompoundTag* tag ) {
	ItemInstance* item = new ItemInstance();
	item->load(tag);
	if (item->getItem() == NULL) {
		delete item;
		item = NULL;
	}
	return item;
}

Item* ItemInstance::getItem() const {
	return Item::items[id];
}

int ItemInstance::getIcon() const {
	return Item::items[id]->getIcon(this->auxValue);
}
void ItemInstance::releaseUsing( Level* level, Player* player, int durationLeft ) {
	getItem()->releaseUsing(this, level, player, durationLeft);
}

int ItemInstance::getUseDuration() {
	return getItem()->getUseDuration(this);
}

UseAnim::UseAnimation ItemInstance::getUseAnimation() const {
	return getItem()->getUseAnimation();
}

ItemInstance ItemInstance::useTimeDepleted( Level* level, Player* player ) {
	return getItem()->useTimeDepleted(this, level, player);
}

bool ItemInstance::isArmorItem( const ItemInstance* instance ) {
	if (!instance)
		return false;

	Item* item = instance->getItem();
	if (!item)
		return false;

	return item->isArmor();
}
