#include "CreativeMode.h"
#include "../Minecraft.h"
#ifndef STANDALONE_SERVER
#include "../particle/ParticleEngine.h"
#endif
#include "../player/LocalPlayer.h"
#ifndef STANDALONE_SERVER
#include "../renderer/LevelRenderer.h"
#include "../sound/SoundEngine.h"
#endif
#include "../../world/level/Level.h"
//#include "../../network/Packet.h"
#include "../../network/packet/RemoveBlockPacket.h"
#include "../../world/entity/player/Abilities.h"

static const int DestructionTickDelay = 10;

CreativeMode::CreativeMode(Minecraft* minecraft)
:	super(minecraft)
{
}

void CreativeMode::startDestroyBlock(int x, int y, int z, int face) {
	if(minecraft->player->getCarriedItem() != NULL && minecraft->player->getCarriedItem()->id == Item::bow->id)
		return;

	creativeDestroyBlock(x, y, z, face);
	destroyDelay = DestructionTickDelay;
}

void CreativeMode::creativeDestroyBlock(int x, int y, int z, int face) {
	//if (!
	minecraft->level->extinguishFire(x, y, z, face)
	//{
		;
		destroyBlock(x, y, z, face);
	//}
}

void CreativeMode::continueDestroyBlock(int x, int y, int z, int face) {
	destroyDelay--;
	if (destroyDelay <= 0) {
		destroyDelay = DestructionTickDelay;
		creativeDestroyBlock(x, y, z, face);
	}
}

void CreativeMode::stopDestroyBlock() {
}

void CreativeMode::initAbilities( Abilities& abilities ) {
	abilities.mayfly = true;
	abilities.instabuild = true;
	abilities.invulnerable = true;
}

bool CreativeMode::isCreativeType() {
	return true;
}

void CreativeMode::releaseUsingItem( Player* player ) {
	if(player->getCarriedItem() != NULL) {
		int oldItemId = player->getCarriedItem()->id;
		int oldAux = player->getAuxData();
		super::releaseUsingItem(player);
		if(player->getCarriedItem() != NULL && player->getCarriedItem()->id == oldItemId) {
			player->getCarriedItem()->setAuxValue(oldAux);
		}
	} else {
		super::releaseUsingItem(player);
	}
}
