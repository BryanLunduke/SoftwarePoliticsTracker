
#include "Packet.h"
#include "../world/level/chunk/LevelChunk.h"

#include "packet/PacketInclude.h"

Packet::Packet()
:	priority(HIGH_PRIORITY),
	reliability(RELIABLE)
{}

Packet* MinecraftPackets::createPacket(int id)
{
	Packet* packet = NULL;

	switch (id - ID_USER_PACKET_ENUM) {
	default:
		break;

	case PACKET_LOGIN:
		packet = new LoginPacket();
		break;
	case PACKET_LOGINSTATUS:
		packet = new LoginStatusPacket();
		break;
	case PACKET_READY:
		packet = new ReadyPacket();
		break;
	case PACKET_SETTIME:
		packet = new SetTimePacket();
		break;
	case PACKET_MESSAGE:
		packet = new MessagePacket();
		break;
	case PACKET_STARTGAME:
		packet = new StartGamePacket();
		break;
	case PACKET_ADDENTITY:
		packet = new AddEntityPacket();
		break;
	case PACKET_ADDITEMENTITY:
		packet = new AddItemEntityPacket();
		break;
	case PACKET_TAKEITEMENTITY:
		packet = new TakeItemEntityPacket();
		break;
	case PACKET_ADDMOB:
		packet = new AddMobPacket();
		break;
	case PACKET_ADDPLAYER:
		packet = new AddPlayerPacket();
		break;
	case PACKET_REMOVEPLAYER:
		packet = new RemovePlayerPacket();
		break;
	case PACKET_MOVEENTITY:
		packet = new MoveEntityPacket();
		break;
	case PACKET_MOVEENTITY_POSROT:
		packet = new MoveEntityPacket_PosRot();
		break;
	//case PACKET_TELEPORTENTITY:
	//	packet = new TeleportEntityPacket();
	//	break;
	case PACKET_MOVEPLAYER:
		packet = new MovePlayerPacket();
		break;
	case PACKET_RESPAWN:
		packet = new RespawnPacket();
		break;
	case PACKET_REMOVEENTITY:
		packet = new RemoveEntityPacket();
		break;
	case PACKET_PLACEBLOCK:
		packet = new PlaceBlockPacket();
		break;
	case PACKET_REMOVEBLOCK:
		packet = new RemoveBlockPacket();
		break;
	case PACKET_UPDATEBLOCK:
		packet = new UpdateBlockPacket();
		break;
	case PACKET_EXPLODE:
		packet = new ExplodePacket();
		break;
	case PACKET_LEVELEVENT:
		packet = new LevelEventPacket();
		break;
	case PACKET_TILEEVENT:
		packet = new TileEventPacket();
		break;
	case PACKET_ENTITYEVENT:
		packet = new EntityEventPacket();
		break;
	case PACKET_REQUESTCHUNK:
		packet = new RequestChunkPacket();
		break;
	case PACKET_CHUNKDATA:
		packet = new ChunkDataPacket();
		break;
	case PACKET_PLAYEREQUIPMENT:
		packet = new PlayerEquipmentPacket();
		break;
	case PACKET_PLAYERARMOREQUIPMENT:
		packet = new PlayerArmorEquipmentPacket();
		break;
	case PACKET_INTERACT:
		packet = new InteractPacket();
		break;
	case PACKET_USEITEM:
		packet = new UseItemPacket();
		break;
	case PACKET_PLAYERACTION:
		packet = new PlayerActionPacket();
		break;
	case PACKET_HURTARMOR:
		packet = new HurtArmorPacket();
		break;
	case PACKET_SETENTITYDATA:
		packet = new SetEntityDataPacket();
		break;
	case PACKET_SETENTITYMOTION:
		packet = new SetEntityMotionPacket();
		break;
	case PACKET_SETHEALTH:
		packet = new SetHealthPacket();
		break;
	case PACKET_SETSPAWNPOSITION:
		packet = new SetSpawnPositionPacket();
		break;
	case PACKET_ANIMATE:
		packet = new AnimatePacket();
		break;
	case PACKET_SENDINVENTORY:
		packet = new SendInventoryPacket();
		break;
	case PACKET_DROPITEM:
		packet = new DropItemPacket();
		break;
	case PACKET_CONTAINERACK:
		packet = new ContainerAckPacket();
		break;
	case PACKET_CONTAINEROPEN:
		packet = new ContainerOpenPacket();
		break;
	case PACKET_CONTAINERCLOSE:
		packet = new ContainerClosePacket();
		break;
	case PACKET_CONTAINERSETDATA:
		packet = new ContainerSetDataPacket();
		break;
	case PACKET_CONTAINERSETSLOT:
		packet = new ContainerSetSlotPacket();
		break;
	case PACKET_CONTAINERSETCONTENT:
		packet = new ContainerSetContentPacket();
		break;
	case PACKET_CHAT:
		packet = new ChatPacket();
		break;
    case PACKET_SIGNUPDATE:
        packet = new SignUpdatePacket();
        break;
	case PACKET_ADDPAINTING:
		packet = new AddPaintingPacket();
		break;
    case PACKET_ADVENTURESETTINGS:
        packet = new AdventureSettingsPacket();
        break;
	}

    return packet;
}


namespace PacketUtil
{
	signed char Rot_degreesToChar(float rot)  {
		return (signed char)(rot / 360.0f * 256.0f);
	}
	float Rot_charToDegrees(signed char rot) {
		return ((float)rot) / 256.0f * 360.0f;
	}

	void Rot_entityToChar(const Entity* e, signed char& yRot, signed char& xRot) {
		xRot = Rot_degreesToChar(e->xRot);
		yRot = Rot_degreesToChar(e->yRot);
	}

	void Rot_charToEntity(Entity* e, signed char yRot, signed char xRot) {
		e->xRot = e->xRotO = Rot_charToDegrees(xRot);
		e->yRot = e->yRotO = Rot_charToDegrees(yRot);
	}

	void writeItemInstance(const ItemInstance& item, RakNet::BitStream* stream) {
		short id			= item.id;
		unsigned char count = item.count;
		short aux			= item.getAuxValue();
		stream->Write(id);
		stream->Write(count);
		stream->Write(aux);
	}

	ItemInstance readItemInstance(RakNet::BitStream* stream) {
		short id, aux;
		unsigned char count;
		stream->Read(id);
		stream->Read(count);
		stream->Read(aux);
		return ItemInstance(id, count, aux);
	}
}
