#include "ItemInclude.h"
#include "ItemCategory.h"
#include "../level/tile/Tile.h"
#include "ItemInstance.h"

const std::string Item::ICON_DESCRIPTION_PREFIX("item.");
Random Item::random;

Item* Item::items[MAX_ITEMS] = {NULL};

Item::Tier const Item::Tier::WOOD(0, 59, 2, 0);
Item::Tier const Item::Tier::STONE(1, 131, 4, 1);
Item::Tier const Item::Tier::IRON(2, 250, 6, 2);
Item::Tier const Item::Tier::EMERALD(3, 1561, 8, 3);
Item::Tier const Item::Tier::GOLD(0, 32, 12, 0);

// Static Item Definitions
Item* Item::shovel_iron = NULL;
Item* Item::pickAxe_iron = NULL;
Item* Item::hatchet_iron = NULL;
Item* Item::flintAndSteel = NULL;
Item* Item::apple = NULL;
Item* Item::bow = NULL;
Item* Item::arrow = NULL;
Item* Item::coal = NULL;
Item* Item::emerald = NULL;
Item* Item::ironIngot = NULL;
Item* Item::goldIngot = NULL;
Item* Item::sword_iron = NULL;

Item* Item::sword_wood = NULL;
Item* Item::shovel_wood = NULL;
Item* Item::pickAxe_wood = NULL;
Item* Item::hatchet_wood = NULL;

Item* Item::sword_stone = NULL;
Item* Item::shovel_stone = NULL;
Item* Item::pickAxe_stone = NULL;
Item* Item::hatchet_stone = NULL;

Item* Item::sword_emerald = NULL;
Item* Item::shovel_emerald = NULL;
Item* Item::pickAxe_emerald = NULL;
Item* Item::hatchet_emerald = NULL;

Item* Item::stick = NULL;
Item* Item::bowl = NULL;
Item* Item::mushroomStew = NULL;

Item* Item::sword_gold = NULL;
Item* Item::shovel_gold = NULL;
Item* Item::pickAxe_gold = NULL;
Item* Item::hatchet_gold = NULL;

Item* Item::string = NULL;
Item* Item::feather = NULL;
Item* Item::sulphur = NULL;

Item* Item::hoe_wood = NULL;
Item* Item::hoe_stone = NULL;
Item* Item::hoe_iron = NULL;
Item* Item::hoe_emerald = NULL;
Item* Item::hoe_gold = NULL;

Item* Item::seeds_wheat = NULL;
Item* Item::wheat = NULL;
Item* Item::bread = NULL;

Item* Item::helmet_cloth = NULL;
Item* Item::chestplate_cloth = NULL;
Item* Item::leggings_cloth = NULL;
Item* Item::boots_cloth = NULL;

Item* Item::helmet_chain = NULL;
Item* Item::chestplate_chain = NULL;
Item* Item::leggings_chain = NULL;
Item* Item::boots_chain = NULL;

Item* Item::helmet_iron = NULL;
Item* Item::chestplate_iron = NULL;
Item* Item::leggings_iron = NULL;
Item* Item::boots_iron = NULL;

Item* Item::helmet_diamond = NULL;
Item* Item::chestplate_diamond = NULL;
Item* Item::leggings_diamond = NULL;
Item* Item::boots_diamond = NULL;

Item* Item::helmet_gold = NULL;
Item* Item::chestplate_gold = NULL;
Item* Item::leggings_gold = NULL;
Item* Item::boots_gold = NULL;

Item* Item::flint = NULL;
Item* Item::porkChop_raw = NULL;
Item* Item::porkChop_cooked = NULL;
Item* Item::painting = NULL;

//Item* Item::apple_gold = NULL;

Item* Item::sign = NULL;
Item* Item::door_wood = NULL;

//Item* Item::bucket_empty = NULL;
//Item* Item::bucket_water = NULL;
//Item* Item::bucket_lava = NULL;

//Item* Item::minecart = NULL;
//Item* Item::saddle = NULL;
Item* Item::door_iron = NULL;
//Item* Item::redStone = NULL;
Item* Item::snowBall = NULL;

//Item* Item::boat = NULL;

Item* Item::leather = NULL;
//Item* Item::milk = NULL;
Item* Item::brick = NULL;
Item* Item::clay = NULL;
Item* Item::reeds = NULL;
Item* Item::paper = NULL;
Item* Item::book = NULL;
Item* Item::slimeBall = NULL;
//Item* Item::minecart_chest = NULL;
//Item* Item::minecart_furnace = NULL;
Item* Item::egg = NULL;
Item* Item::compass = NULL;
//Item* Item::fishingRod = NULL;
Item* Item::clock = NULL;
Item* Item::yellowDust = NULL;
//Item* Item::fish_raw = NULL;
//Item* Item::fish_cooked = NULL;

Item* Item::melon = NULL;
Item* Item::seeds_melon = NULL;

Item* Item::dye_powder = NULL;
Item* Item::bone = NULL;
Item* Item::sugar = NULL;
//Item* Item::cake = NULL;

Item* Item::bed = NULL;

//Item* Item::diode = NULL;
ShearsItem* Item::shears = NULL;
Item* Item::beef_raw = NULL;
Item* Item::beef_cooked = NULL;
Item* Item::chicken_raw = NULL;
Item* Item::chicken_cooked = NULL;

Item* Item::netherbrick = NULL;
Item* Item::netherQuartz = NULL;

//Item* Item::record_01 = NULL;
//Item* Item::record_02 = NULL;

Item* Item::camera = NULL;

/*static*/
void Item::initItems() {
	static bool isInited = false;

	if (isInited)
		return;

	isInited = true;

	Item::shovel_iron = (new ShovelItem(0, Tier::IRON))->setIcon(2, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("shovelIron");
	Item::pickAxe_iron = (new PickaxeItem(1, Tier::IRON))->setIcon(2, 6)->setCategory(ItemCategory::Tools)->setDescriptionId("pickaxeIron");
	Item::hatchet_iron = (new HatchetItem(2, Tier::IRON))->setIcon(2, 7)->setCategory(ItemCategory::Tools)->setDescriptionId("hatchetIron");
	Item::flintAndSteel = (new FlintAndSteelItem(3))->setIcon(5, 0)->setCategory(ItemCategory::Tools)->setDescriptionId("flintAndSteel");
	Item::apple = (new FoodItem(4, 4, false))->setIcon(10, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("apple");
	Item::bow = (new BowItem(5))->setIcon(5, 1)->setCategory(ItemCategory::Tools)->setDescriptionId("bow");
	Item::arrow = (new Item(6))->setIcon(5, 2)->setCategory(ItemCategory::Tools)->setDescriptionId("arrow");
	Item::coal = (new CoalItem(7))->setIcon(7, 0)->setCategory(ItemCategory::Tools)->setDescriptionId("coal");
	Item::emerald = (new Item(8))->setIcon(7, 3)->setCategory(ItemCategory::Decorations)->setDescriptionId("emerald");
	Item::ironIngot = (new Item(9))->setIcon(7, 1)->setCategory(ItemCategory::Decorations)->setDescriptionId("ingotIron");
	Item::goldIngot = (new Item(10))->setIcon(7, 2)->setCategory(ItemCategory::Decorations)->setDescriptionId("ingotGold");
	Item::sword_iron = (new WeaponItem(11, Tier::IRON))->setIcon(2, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("swordIron");
	Item::sword_wood = (new WeaponItem(12, Tier::WOOD))->setIcon(0, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("swordWood");
	Item::shovel_wood = (new ShovelItem(13, Tier::WOOD))->setIcon(0, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("shovelWood");
	Item::pickAxe_wood = (new PickaxeItem(14, Tier::WOOD))->setIcon(0, 6)->setCategory(ItemCategory::Tools)->setDescriptionId("pickaxeWood");
	Item::hatchet_wood = (new HatchetItem(15, Tier::WOOD))->setIcon(0, 7)->setCategory(ItemCategory::Tools)->setDescriptionId("hatchetWood");
	Item::sword_stone = (new WeaponItem(16, Tier::STONE))->setIcon(1, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("swordStone");
	Item::shovel_stone = (new ShovelItem(17, Tier::STONE))->setIcon(1, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("shovelStone");
	Item::pickAxe_stone = (new PickaxeItem(18, Tier::STONE))->setIcon(1, 6)->setCategory(ItemCategory::Tools)->setDescriptionId("pickaxeStone");
	Item::hatchet_stone = (new HatchetItem(19, Tier::STONE))->setIcon(1, 7)->setCategory(ItemCategory::Tools)->setDescriptionId("hatchetStone");
	Item::sword_emerald = (new WeaponItem(20, Tier::EMERALD))->setIcon(3, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("swordDiamond");
	Item::shovel_emerald = (new ShovelItem(21, Tier::EMERALD))->setIcon(3, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("shovelDiamond");
	Item::pickAxe_emerald = (new PickaxeItem(22, Tier::EMERALD))->setIcon(3, 6)->setCategory(ItemCategory::Tools)->setDescriptionId("pickaxeDiamond");
	Item::hatchet_emerald = (new HatchetItem(23, Tier::EMERALD))->setIcon(3, 7)->setCategory(ItemCategory::Tools)->setDescriptionId("hatchetDiamond");
	Item::stick = (new Item(24))->setIcon(5, 3)->handEquipped()->setCategory(ItemCategory::Structures)->setDescriptionId("stick");
	Item::bowl = (new Item(25))->setIcon(7, 4)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("bowl");
	Item::mushroomStew = (new BowlFoodItem(26, 8))->setIcon(8, 4)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("mushroomStew");
	Item::sword_gold = (new WeaponItem(27, Tier::GOLD))->setIcon(4, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("swordGold");
	Item::shovel_gold = (new ShovelItem(28, Tier::GOLD))->setIcon(4, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("shovelGold");
	Item::pickAxe_gold = (new PickaxeItem(29, Tier::GOLD))->setIcon(4, 6)->setCategory(ItemCategory::Tools)->setDescriptionId("pickaxeGold");
	Item::hatchet_gold = (new HatchetItem(30, Tier::GOLD))->setIcon(4, 7)->setCategory(ItemCategory::Tools)->setDescriptionId("hatchetGold");
	Item::string = (new Item(31))->setIcon(8, 0)->setCategory(ItemCategory::Tools)->setDescriptionId("string");
	Item::feather = (new Item(32))->setIcon(8, 1)->setCategory(ItemCategory::Tools)->setDescriptionId("feather");
	Item::sulphur = (new Item(33))->setIcon(8, 2)->setCategory(ItemCategory::Tools)->setDescriptionId("sulphur");
	Item::hoe_wood = (new HoeItem(34, Tier::WOOD))->setIcon(0, 8)->setCategory(ItemCategory::Tools)->setDescriptionId("hoeWood");
	Item::hoe_stone = (new HoeItem(35, Tier::STONE))->setIcon(1, 8)->setCategory(ItemCategory::Tools)->setDescriptionId("hoeStone");
	Item::hoe_iron = (new HoeItem(36, Tier::IRON))->setIcon(2, 8)->setCategory(ItemCategory::Tools)->setDescriptionId("hoeIron");
	Item::hoe_emerald = (new HoeItem(37, Tier::EMERALD))->setIcon(3, 8)->setCategory(ItemCategory::Tools)->setDescriptionId("hoeDiamond");
	Item::hoe_gold = (new HoeItem(38, Tier::GOLD))->setIcon(4, 8)->setCategory(ItemCategory::Tools)->setDescriptionId("hoeGold");
	Item::seeds_wheat = (new SeedItem(39, Tile::crops->id, Tile::farmland->id))->setIcon(9, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("seeds");
	Item::wheat = (new Item(40))->setIcon(9, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("wheat");
	Item::bread = (new FoodItem(41, 5, false))->setIcon(9, 2)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("bread");

	Item::helmet_cloth     = (new ArmorItem(42, ArmorItem::CLOTH, 0, ArmorItem::SLOT_HEAD))->setIcon(0, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("helmetCloth");
	Item::chestplate_cloth = (new ArmorItem(43, ArmorItem::CLOTH, 0, ArmorItem::SLOT_TORSO))->setIcon(0, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("chestplateCloth");
	Item::leggings_cloth   = (new ArmorItem(44, ArmorItem::CLOTH, 0, ArmorItem::SLOT_LEGS))->setIcon(0, 2)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("leggingsCloth");
	Item::boots_cloth      = (new ArmorItem(45, ArmorItem::CLOTH, 0, ArmorItem::SLOT_FEET))->setIcon(0, 3)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("bootsCloth");

	Item::helmet_chain     = (new ArmorItem(46, ArmorItem::CHAIN, 1, ArmorItem::SLOT_HEAD))->setIcon(1, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("helmetChain");
	Item::chestplate_chain = (new ArmorItem(47, ArmorItem::CHAIN, 1, ArmorItem::SLOT_TORSO))->setIcon(1, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("chestplateChain");
	Item::leggings_chain   = (new ArmorItem(48, ArmorItem::CHAIN, 1, ArmorItem::SLOT_LEGS))->setIcon(1, 2)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("leggingsChain");
	Item::boots_chain      = (new ArmorItem(49, ArmorItem::CHAIN, 1, 3))->setIcon(1, 3)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("bootsChain");

	Item::helmet_iron      = (new ArmorItem(50, ArmorItem::IRON, 2, ArmorItem::SLOT_HEAD))->setIcon(2, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("helmetIron");
	Item::chestplate_iron  = (new ArmorItem(51, ArmorItem::IRON, 2, ArmorItem::SLOT_TORSO))->setIcon(2, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("chestplateIron");
	Item::leggings_iron    = (new ArmorItem(52, ArmorItem::IRON, 2, ArmorItem::SLOT_LEGS))->setIcon(2, 2)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("leggingsIron");
	Item::boots_iron       = (new ArmorItem(53, ArmorItem::IRON, 2, ArmorItem::SLOT_FEET))->setIcon(2, 3)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("bootsIron");

	Item::helmet_diamond   = (new ArmorItem(54, ArmorItem::DIAMOND, 3, ArmorItem::SLOT_HEAD))->setIcon(3, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("helmetDiamond");
	Item::chestplate_diamond=(new ArmorItem(55, ArmorItem::DIAMOND, 3, ArmorItem::SLOT_TORSO))->setIcon(3, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("chestplateDiamond");
	Item::leggings_diamond = (new ArmorItem(56, ArmorItem::DIAMOND, 3, ArmorItem::SLOT_LEGS))->setIcon(3, 2)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("leggingsDiamond");
	Item::boots_diamond    = (new ArmorItem(57, ArmorItem::DIAMOND, 3, ArmorItem::SLOT_FEET))->setIcon(3, 3)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("bootsDiamond");

	Item::helmet_gold      = (new ArmorItem(58, ArmorItem::GOLD, 4, ArmorItem::SLOT_HEAD))->setIcon(4, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("helmetGold");
	Item::chestplate_gold  = (new ArmorItem(59, ArmorItem::GOLD, 4, ArmorItem::SLOT_TORSO))->setIcon(4, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("chestplateGold");
	Item::leggings_gold    = (new ArmorItem(60, ArmorItem::GOLD, 4, ArmorItem::SLOT_LEGS))->setIcon(4, 2)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("leggingsGold");
	Item::boots_gold       = (new ArmorItem(61, ArmorItem::GOLD, 4, ArmorItem::SLOT_FEET))->setIcon(4, 3)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("bootsGold");

	Item::flint = (new Item(62))->setIcon(6, 0)->setCategory(ItemCategory::Tools)->setDescriptionId("flint");
	Item::porkChop_raw = (new FoodItem(63, 3, true))->setIcon(7, 5)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("porkchopRaw");
	Item::porkChop_cooked = (new FoodItem(64, 8, true))->setIcon(8, 5)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("porkchopCooked");
	Item::painting = (new HangingEntityItem(65, EntityTypes::IdPainting))->setIcon(10, 1)->setCategory(ItemCategory::Decorations)->setDescriptionId("painting");
	//Item::apple_gold = (new FoodItem(66, 42, false))->setIcon(11, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("appleGold");
	Item::sign = (new SignItem(67))->setIcon(10, 2)->setCategory(ItemCategory::Decorations)->setDescriptionId("sign");
	Item::door_wood = (new DoorItem(68, Material::wood))->setIcon(11, 2)->setCategory(ItemCategory::Structures)->setDescriptionId("doorWood");
	//Item::bucket_empty = (new BucketItem(69, 0))->setIcon(10, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("bucket");
	//Item::bucket_water = (new BucketItem(70, Tile::water.id))->setIcon(11, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("bucketWater")->setCraftingRemainingItem(Item.bucket_empty);
	//Item::bucket_lava = (new BucketItem(71, Tile::lava.id))->setIcon(12, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("bucketLava")->setCraftingRemainingItem(Item.bucket_empty);
	//Item::minecart = (new MinecartItem(72, Minecart.RIDEABLE))->setIcon(7, 8)->setCategory(ItemCategory::Mechanisms)->setDescriptionId("minecart");
	//Item::saddle = (new SaddleItem(73))->setIcon(8, 6)->setCategory(ItemCategory::Mechanisms)->setDescriptionId("saddle");
	Item::door_iron = (new DoorItem(74, Material::metal))->setIcon(12, 2)->setCategory(ItemCategory::Structures)->setDescriptionId("doorIron");
	//Item::redStone = (new RedStoneItem(75))->setIcon(8, 3)->setCategory(ItemCategory::Mechanisms)->setDescriptionId("redstone");
	Item::snowBall = (new SnowballItem(76))->setIcon(14, 0)->setCategory(ItemCategory::Decorations)->setDescriptionId("snowball");
	//Item::boat = (new BoatItem(77))->setIcon(8, 8)->setCategory(ItemCategory::Mechanisms)->setDescriptionId("boat");
	Item::leather = (new Item(78))->setIcon(7, 6)->setCategory(ItemCategory::Tools)->setDescriptionId("leather");
	//Item::milk = (new BucketItem(79, -1))->setIcon(13, 4)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("milk")->setCraftingRemainingItem(Item.bucket_empty);
	Item::brick = (new Item(80))->setIcon(6, 1)->setCategory(ItemCategory::Structures)->setDescriptionId("brick");
	Item::clay = (new Item(81))->setIcon(9, 3)->setCategory(ItemCategory::Structures)->setDescriptionId("clay");
	Item::reeds = (new TilePlanterItem(82, Tile::reeds))->setIcon(11, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("reeds");
	Item::paper = (new Item(83))->setIcon(10, 3)->setCategory(ItemCategory::Decorations)->setDescriptionId("paper");
	Item::book = (new Item(84))->setIcon(11, 3)->setCategory(ItemCategory::Decorations)->setDescriptionId("book");
	Item::slimeBall = (new Item(85))->setIcon(14, 1)->setCategory(ItemCategory::Decorations)->setDescriptionId("slimeball");
	//Item::minecart_chest = (new MinecartItem(86, Minecart::CHEST))->setIcon(7, 9)->setCategory(ItemCategory::Mechanisms)->setDescriptionId("minecartChest");
	//Item::minecart_furnace = (new MinecartItem(87, Minecart::FURNACE))->setIcon(7, 10)->setCategory(ItemCategory::Mechanisms)->setDescriptionId("minecartFurnace");
	Item::egg = (new EggItem(88))->setIcon(12, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("egg");
	Item::compass = (new Item(89))->setIcon(6, 3)->setCategory(ItemCategory::Tools)->setDescriptionId("compass");
	//Item::fishingRod = (new FishingRodItem(90))->setIcon(5, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("fishingRod");
	Item::clock = (new Item(91))->setIcon(6, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("clock");
	Item::yellowDust = (new Item(92))->setIcon(9, 4)->setCategory(ItemCategory::Tools)->setDescriptionId("yellowDust");
	//Item::fish_raw = (new FoodItem(93, 2, false))->setIcon(9, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("fishRaw");
	//Item::fish_cooked = (new FoodItem(94, 5, false))->setIcon(10, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("fishCooked");
	Item::dye_powder = (new DyePowderItem(95))->setIcon(14, 4)->setCategory(ItemCategory::Decorations)->setDescriptionId("dyePowder");
	Item::bone = (new Item(96))->setIcon(12, 1)->setCategory(ItemCategory::Tools)->setDescriptionId("bone")->handEquipped();
	Item::sugar = (new Item(97))->setIcon(13, 0)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("sugar")->handEquipped();
	//Item::cake = (new TilePlanterItem(98, Tile::cake))->setMaxStackSize(1)->setIcon(13, 1)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("cake");
	Item::bed = (new BedItem(99))->setMaxStackSize(1)->setIcon(13, 2)->setCategory(ItemCategory::Structures)->setDescriptionId("bed");
	//Item::diode = (new TilePlanterItem(100, Tile::diode_off))->setIcon(6, 5)->setCategory(ItemCategory::Mechanisms)->setDescriptionId("diode");
	Item::shears = (ShearsItem*)(new ShearsItem(103))->setIcon(13, 5)->setCategory(ItemCategory::Tools)->setDescriptionId("shears");
	Item::melon = (new FoodItem(104, 2, false))->setIcon(13, 6)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("melon");
	Item::seeds_melon = (new SeedItem(106, Tile::melonStem->id, Tile::farmland->id))->setIcon(14, 3)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("seedsMelon");
	Item::beef_raw = (new FoodItem(107, 3, /*FoodConstants.FOOD_SATURATION_LOW,*/ true))->setIcon(9, 6)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("beefRaw");
	Item::beef_cooked = (new FoodItem(108, 8, /*FoodConstants.FOOD_SATURATION_GOOD,*/ true))->setIcon(10, 6)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("beefCooked");
	Item::chicken_raw = (new FoodItem(109, 2, /*FoodConstants.FOOD_SATURATION_LOW,*/ true))->setIcon(9, 7)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("chickenRaw");
	Item::chicken_cooked = (new FoodItem(110, 6, /*FoodConstants.FOOD_SATURATION_NORMAL,*/ true))->setIcon(10, 7)->setCategory(ItemCategory::FoodArmor)->setDescriptionId("chickenCooked");

	Item::netherbrick = (new Item(149))->setIcon(5, 9)->setDescriptionId("netherbrickItem")->setCategory(ItemCategory::Structures);
	Item::netherQuartz = (new Item(150))->setIcon(5, 10)->setDescriptionId("netherquartz")->setCategory(ItemCategory::Mechanisms);

	//Item::record_01 = (new RecordingItem(2000, "13"))->setIcon(0, 15)->setCategory(ItemCategory::Decorations)->setDescriptionId("record");
	//Item::record_02 = (new RecordingItem(2001, "cat"))->setIcon(1, 15)->setCategory(ItemCategory::Decorations)->setDescriptionId("record");
	Item::camera = (new CameraItem(200))->setIcon(2, 15)->setCategory(ItemCategory::Decorations)->setDescriptionId("camera");

	for (int i = 256; i < MAX_ITEMS; ++i) {
		if (items[i] && items[i]->category == -1)
			LOGE("Error: Missing category for item %d: %s\n", items[i]->id, items[i]->getDescriptionId().c_str());
	}
}

/*static*/
void Item::teardownItems() {
	for (int i = 0; i < MAX_ITEMS; ++i)
		if (Item::items[i]) {
			delete Item::items[i];
			Item::items[i] = NULL;
		}
}

ItemInstance Item::useTimeDepleted(ItemInstance* itemInstance, Level* level, Player* player )
{
	return *itemInstance;
}
