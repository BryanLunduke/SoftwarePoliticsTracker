#ifndef _MINECRAFT_NETWORK_SERVERSIDENETWORKHANDLER_H_
#define _MINECRAFT_NETWORK_SERVERSIDENETWORKHANDLER_H_


#include "NetEventCallback.h"
#include "../raknet/RakNetTypes.h"
#include "../world/level/LevelListener.h"
#include <vector>

class Minecraft;
class Level;
class IRakNetInstance;
class Packet;
class Player;

class ServerSideNetworkHandler : public NetEventCallback, public LevelListener
{
public:
	ServerSideNetworkHandler(Minecraft* minecraft, IRakNetInstance* raknetInstance);
	virtual ~ServerSideNetworkHandler();

	virtual void levelGenerated(Level* level);

	virtual void tileChanged(int x, int y, int z);
	virtual void tileBrightnessChanged(int x, int y, int z) { /* do nothing */ }
	virtual Packet* getAddPacketFromEntity(Entity* entity);
	virtual void entityAdded(Entity* e);
	virtual void entityRemoved(Entity* e);
	virtual void levelEvent(Player* source, int type, int x, int y, int z, int data);
	virtual void tileEvent(int x, int y, int z, int b0, int b1);

	virtual void onNewClient(const RakNet::RakNetGUID& clientGuid);
	virtual void onDisconnect(const RakNet::RakNetGUID& guid);

	void onReady_ClientGeneration(const RakNet::RakNetGUID& source);
	void onReady_RequestedChunks(const RakNet::RakNetGUID& source);

	virtual void handle(const RakNet::RakNetGUID& source, LoginPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ReadyPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, MovePlayerPacket* packet);
	//virtual void handle(const RakNet::RakNetGUID& source, PlaceBlockPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RemoveBlockPacket* packet);
	//virtual void handle(const RakNet::RakNetGUID& source, ExplodePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RequestChunkPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerEquipmentPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerArmorEquipmentPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SetHealthPacket* packet);
	//virtual void handle(const RakNet::RakNetGUID& source, TeleportEntityPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, InteractPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, AnimatePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, UseItemPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, EntityEventPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, PlayerActionPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, RespawnPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SendInventoryPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, DropItemPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerSetSlotPacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, ContainerClosePacket* packet);
	virtual void handle(const RakNet::RakNetGUID& source, SignUpdatePacket* packet);

	bool allowsIncomingConnections() { return _allowIncoming; }
	void allowIncomingConnections(bool doAllow);

	Player* popPendingPlayer(const RakNet::RakNetGUID& source);
private:

	void redistributePacket(Packet* packet, const RakNet::RakNetGUID& fromPlayer);
	void displayGameMessage(const std::string& message);

	Player* getPlayer(const RakNet::RakNetGUID& source);
private:

	Minecraft*					minecraft;
	Level*						level;
	IRakNetInstance*			raknetInstance;
	RakNet::RakPeerInterface*	rakPeer;

	std::vector<Player*> _pendingPlayers;
	bool _allowIncoming;
};

#endif
