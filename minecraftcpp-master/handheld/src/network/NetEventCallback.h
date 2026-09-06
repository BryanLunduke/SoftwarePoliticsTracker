#ifndef _MINECRAFT_NETWORK_NETEVENTCALLBACK_H_
#define _MINECRAFT_NETWORK_NETEVENTCALLBACK_H_

class AddItemEntityPacket;
class AddPaintingPacket;
class AdventureSettingsPacket;
class TakeItemEntityPacket;
class LoginPacket;
class ReadyPacket;
class LoginStatusPacket;
class MessagePacket;
class SetTimePacket;
class StartGamePacket;
class AddEntityPacket;
class AddMobPacket;
class AddPlayerPacket;
class RemovePlayerPacket;
class RemoveEntityPacket;
class MoveEntityPacket;
//class TeleportEntityPacket;
class MovePlayerPacket;
class PlaceBlockPacket;
class RemoveBlockPacket;
class UpdateBlockPacket;
class ExplodePacket;
class LevelEventPacket;
class TileEventPacket;
class EntityEventPacket;
class RequestChunkPacket;
class ChunkDataPacket;
class PlayerEquipmentPacket;
class PlayerArmorEquipmentPacket;
class InteractPacket;
class SetEntityDataPacket;
class SetEntityMotionPacket;
class SetHealthPacket;
class SetSpawnPositionPacket;
class SendInventoryPacket;
class DropItemPacket;
class AnimatePacket;
class UseItemPacket;
class PlayerActionPacket;
class HurtArmorPacket;
class RespawnPacket;
class ContainerAckPacket;
class ContainerOpenPacket;
class ContainerClosePacket;
class ContainerSetSlotPacket;
class ContainerSetDataPacket;
class ContainerSetContentPacket;
class ChatPacket;
class SignUpdatePacket;
class Minecraft;
class Level;

#include "../world/level/tile/Tile.h"

namespace RakNet
{
	struct RakNetGUID;
}

class NetEventCallback
{
public:
	virtual void levelGenerated(Level* level) {}
	virtual ~NetEventCallback() {}

	virtual void onConnect(const RakNet::RakNetGUID& hostGuid) {};
	virtual void onUnableToConnect() {};
	virtual void onNewClient(const RakNet::RakNetGUID& clientGuid) {};
	virtual void onDisconnect(const RakNet::RakNetGUID& guid) {};

	virtual void handle(const RakNet::RakNetGUID& source, LoginPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ReadyPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, LoginStatusPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, SetTimePacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, MessagePacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, StartGamePacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, AddItemEntityPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, AddPaintingPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, TakeItemEntityPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, AddEntityPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, AddMobPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, AddPlayerPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, RemovePlayerPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, RemoveEntityPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, MoveEntityPacket* packet) {}
	//virtual void handle(const RakNet::RakNetGUID& source, TeleportEntityPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, MovePlayerPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, PlaceBlockPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, RemoveBlockPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, UpdateBlockPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ExplodePacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, LevelEventPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, TileEventPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, EntityEventPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, RequestChunkPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ChunkDataPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, PlayerEquipmentPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, PlayerArmorEquipmentPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, SetEntityDataPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, SetEntityMotionPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, SetHealthPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, SetSpawnPositionPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, InteractPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, UseItemPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, PlayerActionPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, HurtArmorPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, SendInventoryPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, DropItemPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ContainerOpenPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ContainerClosePacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ContainerAckPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetDataPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetSlotPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetContentPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, ChatPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, SignUpdatePacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, AdventureSettingsPacket* packet) {}
	virtual void handle(const RakNet::RakNetGUID& source, AnimatePacket* packet) {}

	//
	// Common implementation for Client and Server
	//
	virtual void handle(const RakNet::RakNetGUID& source, RespawnPacket* packet) {}
	virtual void handle(Level* level, const RakNet::RakNetGUID& source, RespawnPacket* packet);

	Player* findPlayer(Level* level, int entityId);
	Player* findPlayer(Level* level, const RakNet::RakNetGUID* source);
	Player* findPlayer(Level* level, int entityId, const RakNet::RakNetGUID* source);
};

#endif
