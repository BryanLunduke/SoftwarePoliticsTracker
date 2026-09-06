#ifndef NET_MINECRAFT_WORLD_ITEM__Item_H__
#define NET_MINECRAFT_WORLD_ITEM__Item_H__

//package net.minecraft.world.item;

#include "../../locale/I18n.h"
#include "../../util/Random.h"
#include "UseAnim.h"
class Level;
class Tile;
class Entity;
class Mob;
class Player;
class ItemInstance;

class ShearsItem;

class Item
{
    static const int MAX_STACK_SIZE = 64;//Container::LARGE_MAX_STACK_SIZE;
public:
	static const int MAX_ITEMS = 512;//32000;
    static const int ICON_COLUMNS = 16;
    static const std::string ICON_DESCRIPTION_PREFIX;

    class Tier {
        const int level;
        const int uses;
        const float speed;
        const int damage;
	public:
		static const Tier WOOD;
		static const Tier STONE;
		static const Tier IRON;
		static const Tier EMERALD;
		static const Tier GOLD;

		Tier(int level, int uses, float speed, int damage)
		:	level(level),
			uses(uses),
			speed(speed),
			damage(damage)
		{
        }

        int getUses() const {
            return uses;
        }

        float getSpeed() const {
            return speed;
        }

        int getAttackDamageBonus() const {
            return damage;
        }

        int getLevel() const {
            return level;
        }
    };

public:
    static Item* items[MAX_ITEMS];

    static Item* shovel_iron;
    static Item* pickAxe_iron;
    static Item* hatchet_iron;
    static Item* flintAndSteel;
    static Item* apple;
    static Item* bow;
    static Item* arrow;
    static Item* coal;
    static Item* emerald;
    static Item* ironIngot;
    static Item* goldIngot;
    static Item* sword_iron;

    static Item* sword_wood;
    static Item* shovel_wood;
    static Item* pickAxe_wood;
    static Item* hatchet_wood;

    static Item* sword_stone;
    static Item* shovel_stone;
    static Item* pickAxe_stone;
    static Item* hatchet_stone;

    static Item* sword_emerald;
    static Item* shovel_emerald;
    static Item* pickAxe_emerald;
    static Item* hatchet_emerald;

    static Item* stick;
    static Item* bowl;
    static Item* mushroomStew;

    static Item* sword_gold;
    static Item* shovel_gold;
    static Item* pickAxe_gold;
    static Item* hatchet_gold;

    static Item* string;
    static Item* feather;
    static Item* sulphur;

    static Item* hoe_wood;
    static Item* hoe_stone;
    static Item* hoe_iron;
    static Item* hoe_emerald;
    static Item* hoe_gold;

    static Item* seeds_wheat;
    static Item* wheat;
    static Item* bread;

    static Item* helmet_cloth;
    static Item* chestplate_cloth;
    static Item* leggings_cloth;
    static Item* boots_cloth;

    static Item* helmet_chain;
    static Item* chestplate_chain;
    static Item* leggings_chain;
    static Item* boots_chain;

    static Item* helmet_iron;
    static Item* chestplate_iron;
    static Item* leggings_iron;
    static Item* boots_iron;

    static Item* helmet_diamond;
    static Item* chestplate_diamond;
    static Item* leggings_diamond;
    static Item* boots_diamond;

    static Item* helmet_gold;
    static Item* chestplate_gold;
    static Item* leggings_gold;
    static Item* boots_gold;

    static Item* flint;
    static Item* porkChop_raw;
    static Item* porkChop_cooked;
    static Item* painting;

    static Item* apple_gold;

    static Item* sign;
    static Item* door_wood;

    static Item* bucket_empty;
    static Item* bucket_water;
    static Item* bucket_lava;

    static Item* minecart;
    static Item* saddle;
    static Item* door_iron;
    static Item* redStone;
    static Item* snowBall;

    static Item* boat;

    static Item* leather;
    static Item* milk;
    static Item* brick;
    static Item* clay;
    static Item* reeds;
    static Item* paper;
    static Item* book;
    static Item* slimeBall;
    static Item* minecart_chest;
    static Item* minecart_furnace;
    static Item* egg;
    static Item* compass;
    static Item* fishingRod;
    static Item* clock;
    static Item* yellowDust;
    static Item* fish_raw;
    static Item* fish_cooked;

	static Item* melon;
	static Item* seeds_melon;

    static Item* dye_powder;
    static Item* bone;
    static Item* sugar;
    static Item* cake;

    static Item* bed;

    static Item* diode;

	static ShearsItem* shears;

	static Item* beef_raw;
	static Item* beef_cooked;
	static Item* chicken_raw;
	static Item* chicken_cooked;

	static Item* netherbrick;
	static Item* netherQuartz;

    static Item* record_01;
    static Item* record_02;

	static Item* camera;

	static void initItems();
	static void teardownItems();

    Item(int id)
	:	id(256 + id),
		craftingRemainingItem(NULL),
		maxStackSize(MAX_STACK_SIZE),
		maxDamage(32),
		category(-1),
		_handEquipped(false),
		_isStackedByData(false)
	{
        if (items[this->id] != NULL) {
            printf("Item conflict id @ %d! Id already used\n", this->id);
        }

        items[this->id] = this;
    }

    virtual ~Item() {}

    virtual Item* setIcon(int icon) {
        this->icon = icon;
        return this;
    }

    virtual Item* setMaxStackSize(int max) {
        maxStackSize = max;
        return this;
    }

	virtual bool canBeDepleted() {
		return maxDamage > 0 && !_isStackedByData;
	}

	virtual int getIcon(int auxValue) {
		return icon;
	}

    virtual Item* setIcon(int column, int row) {
        icon = column + row * ICON_COLUMNS;
        return this;
    }

	virtual bool useOn(ItemInstance* itemInstance, Level* level, int x, int y, int z, int face) {
        return false;
    }

    virtual bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        return false;
    }

	virtual int getUseDuration(ItemInstance* itemInstance) {
		return 0;
	}

	virtual ItemInstance useTimeDepleted(ItemInstance* itemInstance, Level* level, Player* player);

    virtual float getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
        return 1;
    }

    virtual ItemInstance* use(ItemInstance* itemInstance, Level* level, Player* player) {
        return itemInstance;
    }

    virtual int getMaxStackSize() {
        return maxStackSize;
    }

    virtual int getLevelDataForAuxValue(int auxValue) {
        return 0;
    }

    virtual bool isStackedByData() {
        return _isStackedByData;
    }

	Item* setCategory(int category) {
		this->category = category;
		return this;
	}

    virtual int getMaxDamage() {
        return maxDamage;
    }

    virtual void hurtEnemy(ItemInstance* itemInstance, Mob* mob) {
    }

    virtual bool mineBlock(ItemInstance* itemInstance, int tile, int x, int y, int z) {
		return false;
    }

    virtual int getAttackDamage(Entity* entity) {
        return 1;
    }

    virtual bool canDestroySpecial(const Tile* tile) const {
        return false;
    }

    virtual void interactEnemy(ItemInstance* itemInstance, Mob* mob) {
    }

    virtual Item* handEquipped() {
        _handEquipped = true;
        return this;
    }

    virtual bool isHandEquipped() const {
        return _handEquipped;
    }

    virtual bool isMirroredArt() const {
        return false;
    }

	virtual bool isFood() const {
		return false;
	}

	virtual bool isArmor() const {
		return false;
	}

    virtual Item* setDescriptionId(const std::string& id) {
        descriptionId = ICON_DESCRIPTION_PREFIX + id;
        return this;
    }

    virtual std::string getDescription() const {
        return I18n::get(getDescriptionId());
    }

    virtual std::string getDescription(const ItemInstance* instance) const {
        return I18n::get(getDescriptionId(instance));
    }

    virtual std::string getDescriptionId() const {
        return descriptionId;
    }

    virtual std::string getDescriptionId(const ItemInstance* instance) const {
        return descriptionId;
    }

    virtual Item* setCraftingRemainingItem(Item* craftingRemainingItem) {
        if (maxStackSize > 1) {
			printf("Max stack size must be 1 for items with crafting results\n");
        }
        this->craftingRemainingItem = craftingRemainingItem;
        return this;
    }

    virtual Item* getCraftingRemainingItem() {
        return craftingRemainingItem;
    }

    virtual bool hasCraftingRemainingItem() const {
        return craftingRemainingItem != NULL;
    }

    virtual std::string getName() const {
        return I18n::get(getDescriptionId() + ".name");
    }

	virtual void releaseUsing( ItemInstance* itemInstance, Level* level, Player* player, int durationLeft ) {}
	virtual UseAnim::UseAnimation getUseAnimation() {return UseAnim::none;}
protected:
    static Random random;

    Item* setStackedByData(bool isStackedByData) {
        _isStackedByData = isStackedByData;
        return this;
    }

	Item* setMaxDamage(int maxDamage) {
        this->maxDamage = maxDamage;
        return this;
	}
	

public:
    const int id;
    int maxDamage;

    int icon;
	int category;
protected:
	int maxStackSize;
private:
	bool _handEquipped;
    bool _isStackedByData;

	Item* craftingRemainingItem;
    std::string descriptionId;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__Item_H__*/
