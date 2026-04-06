#ifndef NET_MINECRAFT_WORLD_ITEM__CoalItem_H__
#define NET_MINECRAFT_WORLD_ITEM__CoalItem_H__

//package net.minecraft.world.item;

class CoalItem: public Item
{
    typedef Item super;
public:
    static const int STONE_COAL = 0;
    static const int CHAR_COAL = 1;

    CoalItem(int id)
    :   super(id)
    {
        setStackedByData(true);
        setMaxDamage(0);
    }

    /*@Override*/
    std::string getDescriptionId(ItemInstance* instance) {
        if (instance->getAuxValue() == CHAR_COAL) {
            return ICON_DESCRIPTION_PREFIX + "charcoal";
        }
        return ICON_DESCRIPTION_PREFIX + "coal";
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__CoalItem_H__*/
