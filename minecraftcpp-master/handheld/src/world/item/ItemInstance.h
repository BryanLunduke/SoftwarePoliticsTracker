#ifndef NET_MINECRAFT_WORLD_ITEM__ItemInstance_H__
#define NET_MINECRAFT_WORLD_ITEM__ItemInstance_H__

//package net.minecraft.world.item;

#include <string>
#include "UseAnim.h"

class Level;
class Tile;
class Entity;
class Mob;
class Player;
class CompoundTag;
class Item;

/*final*/
class ItemInstance
{
public:
	ItemInstance();
    explicit ItemInstance(const Tile* tile); // for catching NULL inits
    ItemInstance(const Tile* tile, int count);
    ItemInstance(const Tile* tile, int count, int auxValue);
    explicit ItemInstance(const Item* item); // for catching NULL inits
    ItemInstance(const Item* item, int count);
    ItemInstance(const Item* item, int count, int auxValue);
    ItemInstance(int id, int count, int damage);
	ItemInstance(const ItemInstance& rhs);

	void init(int id, int count, int damage);
	bool isNull() const;
	void setNull();
	bool operator==(const ItemInstance& rhs) const;
    bool matches(const ItemInstance* b) const;

    //ItemInstance(CompoundTag itemTag);

    ItemInstance remove(int count);

    Item* getItem() const;
    int getIcon() const;

    float getDestroySpeed(Tile* tile);

	bool useOn(Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
    ItemInstance* use(Level* level, Player* player);

	/**
     * Returns true if this item type only can be stacked with items that have
     * the same auxValue data.
     */
    bool isStackedByData() const;
    bool isStackable() const;
    int getMaxStackSize() const;
	static bool isStackable(const ItemInstance* a, const ItemInstance* b);

    bool isDamaged() const;
    bool isDamageableItem() const;
    int getDamageValue() const;
    int getMaxDamage() const;

	int getAuxValue() const;
	void setAuxValue(int value);

    void hurt(int i);
    void hurtEnemy(Mob* mob);

	void mineBlock(int tile, int x, int y, int z);
    int getAttackDamage(Entity* entity);
    bool canDestroySpecial(Tile* tile);
    void snap(Player* player);
    ItemInstance useTimeDepleted(Level* level, Player* player);

    void interactEnemy(Mob* mob);

	//@huge @attn @note: this returns a NEW'ed copy, change?
	ItemInstance* copy() const;
    static bool matches(const ItemInstance* a, const ItemInstance* b);
	static bool matchesNulls(const ItemInstance* a, const ItemInstance* b);

	static bool isArmorItem(const ItemInstance* instance);

    /**
     * Checks if this item is the same item as the other one, disregarding the
     * 'count' value.
     */
    bool sameItem(ItemInstance* b);

    static ItemInstance* clone(const ItemInstance* item);
	static ItemInstance cloneSafe(const ItemInstance* item);

    std::string getDescriptionId() const;
	std::string getName() const;
    ItemInstance* setDescriptionId(const std::string& id);
	std::string toString() const;

	CompoundTag* save(CompoundTag* compoundTag);
	void load(CompoundTag* compoundTag);
	static ItemInstance* fromTag( CompoundTag* tag );
	void releaseUsing( Level* level, Player* player, int durationLeft );
	int getUseDuration();
	UseAnim::UseAnimation getUseAnimation() const;
public:
	int count;
	//int popTime;
	int id;
private:
    /**
     * This was previously the damage value, but is now used for different stuff
     * depending on item / tile. Use the getter methods to make sure the value
     * is interpreted correctly.
     */
	int auxValue;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__ItemInstance_H__*/