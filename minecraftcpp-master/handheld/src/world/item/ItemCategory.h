#ifndef NET_MINECRAFT_WORLD_ITEM__ItemCategory_H__
#define NET_MINECRAFT_WORLD_ITEM__ItemCategory_H__

class ItemCategory {
public:
	static const int Structures = 1;
	static const int Tools		= 2;
	static const int FoodArmor	= 4;
	static const int Decorations= 8;
	static const int Mechanisms = 16;

	static const int NUM_CATEGORIES = 5;

	static const char* categoryToString(int id) {
		switch (id) {
		case Structures:	return "Structures";
		case Tools:			return "Tools";
		case FoodArmor:		return "Food and\nArmor";
		case Decorations:	return "Decorations";
		case Mechanisms:	return "Mechanisms";

		default:			return "<Unknown category>";
		}
	}
};

#endif /*NET_MINECRAFT_WORLD_ITEM__ItemCategory_H__*/
