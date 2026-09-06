#include "NetherReactor.h"
#include "../../entity/player/Player.h"
#include "../Level.h"
#include "entity/NetherReactorTileEntity.h"
#include "NetherReactorPattern.h"
NetherReactor::NetherReactor( int id, int tex, const Material* material ) : super(id, tex, material) { }

bool NetherReactor::use( Level* level, int x, int y, int z, Player* player ) {
	if(level->getLevelData()->getGameType() != GameType::Survival)
		return false;

	// Level, X, Z
	NetherReactorPattern pattern;
	for(int checkLevel = 0; checkLevel <= 2; ++checkLevel) {
		for(int checkX = -1; checkX <= 1; ++checkX) {
			for(int checkZ = -1; checkZ <= 1; ++checkZ) {
				if(level->getTile(x + checkX, y + checkLevel  - 1, z + checkZ) != pattern.getTileAt(checkLevel, checkX + 1, checkZ + 1)) {
					player->displayClientMessage("Not the correct pattern!");
					return false;
				}
			}
		}
	}
	if(!canSpawnStartNetherReactor(level, x, y, z, player)) return false;
	player->displayClientMessage("Active!");
	NetherReactorTileEntity* reactor = static_cast<NetherReactorTileEntity*>(level->getTileEntity(x, y, z));
	if (reactor != NULL) {
		reactor->lightItUp(x, y, z);
	}
	return true;
}

TileEntity* NetherReactor::newTileEntity() {
	return TileEntityFactory::createTileEntity(TileEntityType::NetherReactor);
}

void NetherReactor::setPhase(Level* level, int x, int y, int z, int phase) {
	int curPhase = level->getData(x, y, z);
	if(curPhase != phase) {
		level->setData(x, y, z, phase);
	}
}

int NetherReactor::getTexture( int face, int data ) {
	switch(data) {
	case 1: return tex-1;
	case 2: return tex-2;
	default: return tex;
	}
}

bool NetherReactor::canSpawnStartNetherReactor( Level* level, int x, int y, int z, Player* player ) {
	if(!allPlayersCloseToReactor(level, x, y, z)) {
		player->displayClientMessage("All players need to be close to the reactor.");
		return false;
	} else if(y > LEVEL_HEIGHT - 28) {
		player->displayClientMessage("The nether reactor needs to be built lower down.");
		return false;
	} else if(y < 2) {
		player->displayClientMessage("The nether reactor needs to be built higher up.");
		return false;
	}
	return true;
}

bool NetherReactor::allPlayersCloseToReactor( Level* level, int x, int y, int z ) {
	for(PlayerList::const_iterator i =  level->players.begin(); i != level->players.end(); ++i) {
		Player* currentPlayer = (*i);
		if(!(currentPlayer->x >= x - 5 && currentPlayer->x <= x + 5)) return false;
		if(!(currentPlayer->y - currentPlayer->heightOffset >= y -1 &&  currentPlayer->y - currentPlayer->heightOffset <= y + 1)) return false;
		if(!(currentPlayer->z >= z - 5 && currentPlayer->z <= z + 5)) return false;
	}
	return true;
}
