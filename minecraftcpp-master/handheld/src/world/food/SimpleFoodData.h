#ifndef NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__
#define NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__

//package net.minecraft.world.food;

class FoodItem;
class Player;

class SimpleFoodData
{
public:
    SimpleFoodData();

    void eat(int food);
    void eat(FoodItem* item);

    void tick(Player* player);
private:
	int foodLevel;
};

#endif /*NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__*/
