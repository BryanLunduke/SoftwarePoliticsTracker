#ifndef NET_MINECRAFT_CLIENT__User_H__
#define NET_MINECRAFT_CLIENT__User_H__

//package net.minecraft.client;

#include "../world/level/tile/Tile.h"

class User
{
public:
    //static List<Tile> allowedTiles = /*new*/ ArrayList<Tile>();

    //static {
    //    allowedTiles.push_back(Tile::rock);
    //    allowedTiles.push_back(Tile::stoneBrick);
    //    allowedTiles.push_back(Tile::redBrick);
    //    allowedTiles.push_back(Tile::dirt);
    //    allowedTiles.push_back(Tile::wood);
    //    allowedTiles.push_back(Tile::treeTrunk);
    //    allowedTiles.push_back(Tile::leaves);
    //    allowedTiles.push_back(Tile::torch);
    //    allowedTiles.push_back(Tile::stoneSlabHalf);

    //    allowedTiles.push_back(Tile::glass);
    //    allowedTiles.push_back(Tile::mossStone);
    //    allowedTiles.push_back(Tile::sapling);
    //    allowedTiles.push_back(Tile::flower);
    //    allowedTiles.push_back(Tile::rose);
    //    allowedTiles.push_back(Tile::mushroom1);
    //    allowedTiles.push_back(Tile::mushroom2);
    //    allowedTiles.push_back(Tile::sand);
    //    allowedTiles.push_back(Tile::gravel);
    //    allowedTiles.push_back(Tile::sponge);

    //    allowedTiles.push_back(Tile::cloth);
    //    allowedTiles.push_back(Tile::coalOre);
    //    allowedTiles.push_back(Tile::ironOre);
    //    allowedTiles.push_back(Tile::goldOre);
    //    allowedTiles.push_back(Tile::ironBlock);
    //    allowedTiles.push_back(Tile::goldBlock);
    //    allowedTiles.push_back(Tile::bookshelf);
    //    allowedTiles.push_back(Tile::tnt);
    //    allowedTiles.push_back(Tile::obsidian);

    //    // System.out.println(allowedTiles.size());
    //}

	static bool isTileAllowed(const Tile& tile) {
		return true;
	}

    std::string name;
    std::string sessionId;
    //std::string mpPassword;

    User(const std::string& name, const std::string& sessionId) {
        this->name = name;
        this->sessionId = sessionId;
    }
};

#endif /*NET_MINECRAFT_CLIENT__User_H__*/
