#include "ServerLevel.h"
#include "../network/RakNetInstance.h"
#include "../network/packet/SetTimePacket.h"
#include "../network/packet/SetHealthPacket.h"
#include "../world/level/tile/LevelEvent.h"
ServerLevel::ServerLevel(LevelStorage* levelStorage, const std::string& levelName, const LevelSettings& settings, int generatorVersion, Dimension* fixedDimension /* = NULL */)
	: super(levelStorage, levelName, settings, generatorVersion, fixedDimension),
	allPlayersAreSleeping(false){

}

void ServerLevel::updateSleepingPlayerList() {
	bool allPlayersWasSleeping = allPlayersAreSleeping;
	allPlayersAreSleeping = !players.empty();
	for(PlayerList::iterator it = players.begin(); it != players.end(); ++it) {
		Player* player = *it;
		if(!player->isSleeping()) {
			allPlayersAreSleeping = false;
			break;
		}
	}
	if(!allPlayersWasSleeping && allPlayersAreSleeping) {
		levelEvent(NULL, LevelEvent::ALL_PLAYERS_SLEEPING, 0, 0, 0, 0);
		for(PlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			(*it)->setAllPlayersSleeping();
		}
	}
}

void ServerLevel::awakenAllPlayers() {
	allPlayersAreSleeping = false;
	for(PlayerList::iterator it = players.begin(); it != players.end(); ++it) {
		Player* player = *it;
		if(player->isSleeping()) {
			player->stopSleepInBed(false, false, true);
			player->health = Player::MAX_HEALTH;
			player->lastHealth = Player::MAX_HEALTH; 
		}
	}
	SetHealthPacket packet(Player::MAX_HEALTH);
	raknetInstance->send(packet);
}

bool ServerLevel::allPlayersSleeping() {
	if(allPlayersAreSleeping && !isClientSide) {
		// all players are sleeping, but have they slept long enough?
		for(PlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if(!(*it)->isSleepingLongEnough()) {
				return false;
			}
		}
		// yep
		return true;
	}
	return false;
}

void ServerLevel::tick(){
	super::tick();
	if(allPlayersSleeping()) {
		long newTime = levelData.getTime() + TICKS_PER_DAY;
		levelData.setTime(newTime - (newTime % TICKS_PER_DAY));
		SetTimePacket packet(levelData.getTime());
		raknetInstance->send(packet);
		awakenAllPlayers();
	}
}
