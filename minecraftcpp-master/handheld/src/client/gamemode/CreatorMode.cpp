#include "CreatorMode.h"
#include "../Minecraft.h"
#include "../particle/ParticleEngine.h"
#include "../player/LocalPlayer.h"
#include "../renderer/LevelRenderer.h"
#include "../sound/SoundEngine.h"
#include "../../world/level/Level.h"
//#include "../../network/Packet.h"
#include "../../network/packet/RemoveBlockPacket.h"
#include "../../world/entity/player/Abilities.h"

static const int DestructionTickDelay = 10;

class Creator: public ICreator {
	//virtual void getEvents();

public:
	Creator(/*int eventLifeTime*/)
	:	_tileEvents(32),
		_tickId(0)
	{
	}

	void setTick(int tick) {
		_tickId = tick;
	}

	EventList<TileEvent>& getTileEvents() { return _tileEvents; }

	void addevent_blockUse(int entityId, int x, int y, int z, int face) {
		TileEvent t = {
			entityId,
			x,y,z,
			face
		};
		_tileEvents.add(t, _tickId);
	}

private:
	EventList<TileEvent> _tileEvents;
	int _tickId;
};

CreatorMode::CreatorMode(Minecraft* minecraft)
:	super(minecraft)
{
	_creator = new Creator();
}

CreatorMode::~CreatorMode() {
	delete _creator;
}

void CreatorMode::startDestroyBlock(int x, int y, int z, int face) {
	if(minecraft->player->getCarriedItem() != NULL && minecraft->player->getCarriedItem()->id == Item::bow->id)
		return;

	CreatorDestroyBlock(x, y, z, face);
	destroyDelay = DestructionTickDelay;
}

void CreatorMode::CreatorDestroyBlock(int x, int y, int z, int face) {
	//if (!
	minecraft->level->extinguishFire(x, y, z, face)
	//{
		;
		destroyBlock(x, y, z, face);
	//}
}

void CreatorMode::continueDestroyBlock(int x, int y, int z, int face) {
	destroyDelay--;
	if (destroyDelay <= 0) {
		destroyDelay = DestructionTickDelay;
		CreatorDestroyBlock(x, y, z, face);
	}
}

bool CreatorMode::useItemOn( Player* player, Level* level, ItemInstance* item, int x, int y, int z, int face, const Vec3& hit ) {
	if (item && item->id == ((Item*)Item::sword_iron)->id)
		_creator->addevent_blockUse(player->entityId, x, y, z, face);
	return super::useItemOn(player, level, item, x, y, z, face, hit);
}

void CreatorMode::stopDestroyBlock() {
}

void CreatorMode::initAbilities( Abilities& abilities ) {
	abilities.mayfly = true;
	abilities.instabuild = true;
	abilities.invulnerable = true;
}

bool CreatorMode::isCreativeType() {
	return true;
}

void CreatorMode::releaseUsingItem( Player* player ) {
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

ICreator* CreatorMode::getCreator() {
	return _creator;
}

void CreatorMode::tick() {
	_creator->setTick(minecraft->level->getTime());
	super::tick();
}
