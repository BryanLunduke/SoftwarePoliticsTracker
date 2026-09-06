#ifndef NET_MINECRAFT_WORLD_ITEM__DyePowderItem_H__
#define NET_MINECRAFT_WORLD_ITEM__DyePowderItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include <string>

class ItemInstance;
class Level;
class Mob;
class Player;

class DyePowderItem: public Item
{
    typedef Item super;
public:
    static const std::string COLOR_DESCS[];
    static const int COLOR_RGB[];

    static const int BLACK = 0;
    static const int RED = 1;
    static const int GREEN = 2;
    static const int BROWN = 3;
    static const int BLUE = 4;
    static const int PURPLE = 5;
    static const int CYAN = 6;
    static const int SILVER = 7;
    static const int GRAY = 8;
    static const int PINK = 9;
    static const int LIME = 10;
    static const int YELLOW = 11;
    static const int LIGHT_BLUE = 12;
    static const int MAGENTA = 13;
    static const int ORANGE = 14;
    static const int WHITE = 15;

    DyePowderItem(int id);

    ///*@Override*/
    int getIcon(int itemAuxValue);

    ///*@Override*/
    std::string getDescriptionId(const ItemInstance* itemInstance) const;

    ///*@Override*/
    bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);

    ///*@Override*/
    void interactEnemy(ItemInstance* itemInstance, Mob* mob);
};

#endif /*NET_MINECRAFT_WORLD_ITEM__DyePowderItem_H__*/
