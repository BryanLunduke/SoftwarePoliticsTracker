#include "RakNetInstance.h"
#include "Packet.h"
#include "NetEventCallback.h"
#include "../raknet/RakPeerInterface.h"
#include "../raknet/BitStream.h"
#include "../raknet/MessageIdentifiers.h"
#include "../raknet/GetTime.h"
#include "../AppConstants.h"

#include "../platform/log.h"

#define APP_IDENTIFIER "MCCPP;" APP_VERSION_STRING ";"
#define APP_IDENTIFIER_MINECON "MCCPP;MINECON;"

RakNetInstance::RakNetInstance()
:	rakPeer(NULL),
	_isServer(false),
	_isLoggedIn(false)
{
	rakPeer = RakNet::RakPeerInterface::GetInstance();
	rakPeer->SetTimeoutTime(20000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
    rakPeer->SetOccasionalPing(true);
}

RakNetInstance::~RakNetInstance()
{
	if (rakPeer)
	{
		rakPeer->Shutdown(100, 0);
		RakNet::RakPeerInterface::DestroyInstance(rakPeer);
		rakPeer = NULL;
	}
}

bool RakNetInstance::host(const std::string& localName, int port, int maxConnections /* = 4 */)
{
	if (rakPeer->IsActive())
	{
		rakPeer->Shutdown(500);
	}

	RakNet::SocketDescriptor socket(port, 0);
	socket.socketFamily = AF_INET;

	rakPeer->SetMaximumIncomingConnections(maxConnections);
	RakNet::StartupResult result = rakPeer->Startup(maxConnections, &socket, 1);

	_isServer = true;
	isPingingForServers = false;

	return (result == RakNet::RAKNET_STARTED);
}

void RakNetInstance::announceServer(const std::string& localName)
{
	if (_isServer && rakPeer->IsActive())
	{
		RakNet::RakString connectionData;

#if defined(MINECON)
		connectionData += APP_IDENTIFIER_MINECON;
#else
		connectionData += APP_IDENTIFIER;
#endif
		connectionData += localName.c_str();

		RakNet::BitStream bitStream;
		bitStream.Write(connectionData);
		rakPeer->SetOfflinePingResponse((const char*)bitStream.GetData(), bitStream.GetNumberOfBytesUsed());
	}
}

bool RakNetInstance::connect(const char* host, int port)
{
	_isLoggedIn = false;
	RakNet::StartupResult result;

	RakNet::SocketDescriptor socket(0, 0);
	socket.socketFamily = AF_INET;

	if (rakPeer->IsActive())
	{
		rakPeer->Shutdown(500);
	}

	result = rakPeer->Startup(4, &socket, 1);

	_isServer = false;
	isPingingForServers = false;

	if (result == RakNet::RAKNET_STARTED)
	{
		RakNet::ConnectionAttemptResult connectResult = rakPeer->Connect(host, port, NULL, 0, NULL, 0, 12, 500, 0);

		return (connectResult == RakNet::CONNECTION_ATTEMPT_STARTED);
	}

	return false;
}

void RakNetInstance::disconnect()
{
	if (rakPeer->IsActive())
	{
		rakPeer->Shutdown(500);
	}
	_isLoggedIn = false;
	_isServer = false;
	isPingingForServers = false;
}

void RakNetInstance::pingForHosts(int basePort)
{
	if (!rakPeer->IsActive())
	{
		RakNet::SocketDescriptor socket(0, 0);
		rakPeer->Startup(4, &socket, 1);
	}

	isPingingForServers = true;
	pingPort = basePort;
	lastPingTime = RakNet::GetTimeMS();

	for (int i = 0; i < 4; ++i)
		rakPeer->Ping("255.255.255.255", basePort + i, true);
}

void RakNetInstance::stopPingForHosts()
{
	if (isPingingForServers)
	{
		rakPeer->Shutdown(0);
		isPingingForServers = false;
	}
}

const ServerList& RakNetInstance::getServerList()
{
	return availableServers;
}

void RakNetInstance::clearServerList()
{
	availableServers.clear();
	/*
	for (int i = 0; i < 20; ++i) {
		PingedCompatibleServer ps;
		ps.isSpecial = false;
		ps.name = "Fake-Real";
		ps.address.FromString("192.168.1.236|19132");
		availableServers.push_back(ps);
	}
	*/
}

RakNet::RakPeerInterface* RakNetInstance::getPeer()
{
	return rakPeer;
}

bool RakNetInstance::isProbablyBroken() {
    return rakPeer->errorState < -100;
}
void RakNetInstance::resetIsBroken() {
	rakPeer->errorState = 0;
}

bool RakNetInstance::isMyLocalGuid(const RakNet::RakNetGUID& guid)
{
	return rakPeer->IsActive() && rakPeer->GetMyGUID() == guid;
}

void RakNetInstance::runEvents(NetEventCallback* callback)
{
	RakNet::Packet* currentEvent;

	while ((currentEvent = rakPeer->Receive()) != NULL)
	{
		int packetId = currentEvent->data[0];
		int length = currentEvent->length;

		RakNet::BitStream activeBitStream(currentEvent->data + 1, length - 1, false);

		if (callback) {
			if (packetId < ID_USER_PACKET_ENUM)
			{
				//LOGI("Received event: %s\n", getPacketName(packetId));
				switch (packetId)
				{
				case ID_NEW_INCOMING_CONNECTION:
					callback->onNewClient(currentEvent->guid);
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
					serverGuid = currentEvent->guid;
					callback->onConnect(currentEvent->guid);
					break;
				case ID_CONNECTION_ATTEMPT_FAILED:
					callback->onUnableToConnect();
					break;
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_CONNECTION_LOST:
					callback->onDisconnect(currentEvent->guid);
					break;
				case ID_UNCONNECTED_PONG:
					{
						RakNet::TimeMS time;
						RakNet::RakString data;
						activeBitStream.Read(time);
						activeBitStream.Read(data);

						int index = handleUnconnectedPong(data, currentEvent, APP_IDENTIFIER, false);
						if (index < 0) {
							// Check if it's an official Mojang MineCon server
							index = handleUnconnectedPong(data, currentEvent, APP_IDENTIFIER_MINECON, true);
							if (index >= 0) availableServers[index].isSpecial = true;
						}
					}
					break;
				}
			}
			else
			{
				int userPacketId = packetId - ID_USER_PACKET_ENUM;
				bool isStatusPacket = userPacketId <= PACKET_READY;

				if (isStatusPacket || _isServer || _isLoggedIn) {

					if (Packet* packet = MinecraftPackets::createPacket(packetId)) {
						packet->read(&activeBitStream);
						packet->handle(currentEvent->guid, callback);
						delete packet;
					}
				}
			}
		}

		rakPeer->DeallocatePacket(currentEvent);
		//delete activeBitStream;
	}

	if (isPingingForServers)
	{
		if (RakNet::GetTimeMS() - lastPingTime > 1000)
		{
			// remove servers that hasn't responded for a while
			ServerList::iterator it = availableServers.begin();
			for (; it != availableServers.end(); )
			{
				if (RakNet::GetTimeMS() - it->pingTime > 3000)
				{
					it = availableServers.erase(it);
				}
				else
				{
					++it;
				}
			}

			pingForHosts(pingPort);
		}
	}

}

void RakNetInstance::send(Packet& packet) {
	RakNet::BitStream bitStream;
	packet.write(&bitStream);
	if (_isServer)
	{
		// broadcast to all connected clients
		rakPeer->Send(&bitStream, packet.priority, packet.reliability, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}
	else
	{
		// send to server
		rakPeer->Send(&bitStream, packet.priority, packet.reliability, 0, serverGuid, false);
	}
}

void RakNetInstance::send(const RakNet::RakNetGUID& guid, Packet& packet) {
	RakNet::BitStream bitStream;
	packet.write(&bitStream);
	rakPeer->Send(&bitStream, packet.priority, packet.reliability, 0, guid, false);
}


void RakNetInstance::send(Packet* packet)
{
	send(*packet);
	delete packet;
}

void RakNetInstance::send(const RakNet::RakNetGUID& guid, Packet* packet)
{
	send(guid, *packet);
	delete packet;
}

#ifdef _DEBUG
const char* RakNetInstance::getPacketName(int packetId)
{

	switch (packetId)
	{
	case ID_CONNECTED_PING:
		return "ID_CONNECTED_PING";
		
	case ID_UNCONNECTED_PING:
		return "ID_UNCONNECTED_PING";
		
	case ID_UNCONNECTED_PING_OPEN_CONNECTIONS:
		return "ID_UNCONNECTED_PING_OPEN_CONNECTIONS";
		
	case ID_CONNECTED_PONG:
		return "ID_CONNECTED_PONG";
		
	case ID_DETECT_LOST_CONNECTIONS:
		return "ID_DETECT_LOST_CONNECTIONS";
		
	case ID_OPEN_CONNECTION_REQUEST_1:
		return "ID_OPEN_CONNECTION_REQUEST_1";
		
	case ID_OPEN_CONNECTION_REPLY_1:
		return "ID_OPEN_CONNECTION_REPLY_1";
		
	case ID_OPEN_CONNECTION_REQUEST_2:
		return "ID_OPEN_CONNECTION_REQUEST_2";
		
	case ID_OPEN_CONNECTION_REPLY_2:
		return "ID_OPEN_CONNECTION_REPLY_2";
		
	case ID_CONNECTION_REQUEST:
		return "ID_CONNECTION_REQUEST";
		
	case ID_REMOTE_SYSTEM_REQUIRES_PUBLIC_KEY:
		return "ID_REMOTE_SYSTEM_REQUIRES_PUBLIC_KEY";
		
	case ID_OUR_SYSTEM_REQUIRES_SECURITY:
		return "ID_OUR_SYSTEM_REQUIRES_SECURITY";
		
	case ID_PUBLIC_KEY_MISMATCH:
		return "ID_PUBLIC_KEY_MISMATCH";
		
	case ID_OUT_OF_BAND_INTERNAL:
		return "ID_OUT_OF_BAND_INTERNAL";
		
	case ID_SND_RECEIPT_ACKED:
		return "ID_SND_RECEIPT_ACKED";
		
	case ID_SND_RECEIPT_LOSS:
		return "ID_SND_RECEIPT_LOSS";
		
	case ID_CONNECTION_REQUEST_ACCEPTED:
		return "ID_CONNECTION_REQUEST_ACCEPTED";
		
	case ID_CONNECTION_ATTEMPT_FAILED:
		return "ID_CONNECTION_ATTEMPT_FAILED";
		
	case ID_ALREADY_CONNECTED:
		return "ID_ALREADY_CONNECTED";
		
	case ID_NEW_INCOMING_CONNECTION:
		return "ID_NEW_INCOMING_CONNECTION";
		
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		return "ID_NO_FREE_INCOMING_CONNECTIONS";
		
	case ID_DISCONNECTION_NOTIFICATION:
		return "ID_DISCONNECTION_NOTIFICATION";
		
	case ID_CONNECTION_LOST:
		return "ID_CONNECTION_LOST";
		
	case ID_CONNECTION_BANNED:
		return "ID_CONNECTION_BANNED";
		
	case ID_INVALID_PASSWORD:
		return "ID_INVALID_PASSWORD";
		
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		return "ID_INCOMPATIBLE_PROTOCOL_VERSION";
		
	case ID_IP_RECENTLY_CONNECTED:
		return "ID_IP_RECENTLY_CONNECTED";
		
	case ID_TIMESTAMP:
		return "ID_TIMESTAMP";
		
	case ID_UNCONNECTED_PONG:
		return "ID_UNCONNECTED_PONG";
		
	case ID_ADVERTISE_SYSTEM:
		return "ID_ADVERTISE_SYSTEM";
		
	case ID_DOWNLOAD_PROGRESS:
		return "ID_DOWNLOAD_PROGRESS";
		
	case ID_REMOTE_DISCONNECTION_NOTIFICATION:
		return "ID_REMOTE_DISCONNECTION_NOTIFICATION";
		
	case ID_REMOTE_CONNECTION_LOST:
		return "ID_REMOTE_CONNECTION_LOST";
		
	case ID_REMOTE_NEW_INCOMING_CONNECTION:
		return "ID_REMOTE_NEW_INCOMING_CONNECTION";
		
	case ID_FILE_LIST_TRANSFER_HEADER:
		return "ID_FILE_LIST_TRANSFER_HEADER";
		
	case ID_FILE_LIST_TRANSFER_FILE:
		return "ID_FILE_LIST_TRANSFER_FILE";
		
	case ID_FILE_LIST_REFERENCE_PUSH_ACK:
		return "ID_FILE_LIST_REFERENCE_PUSH_ACK";
		
	case ID_DDT_DOWNLOAD_REQUEST:
		return "ID_DDT_DOWNLOAD_REQUEST";
		
	case ID_TRANSPORT_STRING:
		return "ID_TRANSPORT_STRING";
		
	case ID_REPLICA_MANAGER_CONSTRUCTION:
		return "ID_REPLICA_MANAGER_CONSTRUCTION";
		
	case ID_REPLICA_MANAGER_SCOPE_CHANGE:
		return "ID_REPLICA_MANAGER_SCOPE_CHANGE";
		
	case ID_REPLICA_MANAGER_SERIALIZE:
		return "ID_REPLICA_MANAGER_SERIALIZE";
		
	case ID_REPLICA_MANAGER_DOWNLOAD_STARTED:
		return "ID_REPLICA_MANAGER_DOWNLOAD_STARTED";
		
	case ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE:
		return "ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE";
		
	case ID_RAKVOICE_OPEN_CHANNEL_REQUEST:
		return "ID_RAKVOICE_OPEN_CHANNEL_REQUEST";
		
	case ID_RAKVOICE_OPEN_CHANNEL_REPLY:
		return "ID_RAKVOICE_OPEN_CHANNEL_REPLY";
		
	case ID_RAKVOICE_CLOSE_CHANNEL:
		return "ID_RAKVOICE_CLOSE_CHANNEL";
		
	case ID_RAKVOICE_DATA:
		return "ID_RAKVOICE_DATA";
		
	case ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE:
		return "ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE";
		
	case ID_AUTOPATCHER_CREATION_LIST:
		return "ID_AUTOPATCHER_CREATION_LIST";
		
	case ID_AUTOPATCHER_DELETION_LIST:
		return "ID_AUTOPATCHER_DELETION_LIST";
		
	case ID_AUTOPATCHER_GET_PATCH:
		return "ID_AUTOPATCHER_GET_PATCH";
		
	case ID_AUTOPATCHER_PATCH_LIST:
		return "ID_AUTOPATCHER_PATCH_LIST";
		
	case ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR:
		return "ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR";
		
	case ID_AUTOPATCHER_FINISHED_INTERNAL:
		return "ID_AUTOPATCHER_FINISHED_INTERNAL";
		
	case ID_AUTOPATCHER_FINISHED:
		return "ID_AUTOPATCHER_FINISHED";
		
	case ID_AUTOPATCHER_RESTART_APPLICATION:
		return "ID_AUTOPATCHER_RESTART_APPLICATION";
		
	case ID_NAT_PUNCHTHROUGH_REQUEST:
		return "ID_NAT_PUNCHTHROUGH_REQUEST";
		
	case ID_NAT_GROUP_PUNCHTHROUGH_REQUEST:
		return "ID_NAT_GROUP_PUNCHTHROUGH_REQUEST";
		
	case ID_NAT_GROUP_PUNCHTHROUGH_REPLY:
		return "ID_NAT_GROUP_PUNCHTHROUGH_REPLY";
		
	case ID_NAT_CONNECT_AT_TIME:
		return "ID_NAT_CONNECT_AT_TIME";
		
	case ID_NAT_GET_MOST_RECENT_PORT:
		return "ID_NAT_GET_MOST_RECENT_PORT";
		
	case ID_NAT_CLIENT_READY:
		return "ID_NAT_CLIENT_READY";
		
	case ID_NAT_GROUP_PUNCHTHROUGH_FAILURE_NOTIFICATION:
		return "ID_NAT_GROUP_PUNCHTHROUGH_FAILURE_NOTIFICATION";
		
	case ID_NAT_TARGET_NOT_CONNECTED:
		return "ID_NAT_TARGET_NOT_CONNECTED";
		
	case ID_NAT_TARGET_UNRESPONSIVE:
		return "ID_NAT_TARGET_UNRESPONSIVE";
		
	case ID_NAT_CONNECTION_TO_TARGET_LOST:
		return "ID_NAT_CONNECTION_TO_TARGET_LOST";
		
	case ID_NAT_ALREADY_IN_PROGRESS:
		return "ID_NAT_ALREADY_IN_PROGRESS";
		
	case ID_NAT_PUNCHTHROUGH_FAILED:
		return "ID_NAT_PUNCHTHROUGH_FAILED";
		
	case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
		return "ID_NAT_PUNCHTHROUGH_SUCCEEDED";
		
	case ID_NAT_GROUP_PUNCH_FAILED:
		return "ID_NAT_GROUP_PUNCH_FAILED";
		
	case ID_NAT_GROUP_PUNCH_SUCCEEDED:
		return "ID_NAT_GROUP_PUNCH_SUCCEEDED";
		
	case ID_READY_EVENT_SET:
		return "ID_READY_EVENT_SET";
		
	case ID_READY_EVENT_UNSET:
		return "ID_READY_EVENT_UNSET";
		
	case ID_READY_EVENT_ALL_SET:
		return "ID_READY_EVENT_ALL_SET";
		
	case ID_READY_EVENT_QUERY:
		return "ID_READY_EVENT_QUERY";
		
	case ID_LOBBY_GENERAL:
		return "ID_LOBBY_GENERAL";
		
	case ID_RPC_REMOTE_ERROR:
		return "ID_RPC_REMOTE_ERROR";
		
	case ID_RPC_PLUGIN:
		return "ID_RPC_PLUGIN";
		
	case ID_FILE_LIST_REFERENCE_PUSH:
		return "ID_FILE_LIST_REFERENCE_PUSH";
		
	case ID_READY_EVENT_FORCE_ALL_SET:
		return "ID_READY_EVENT_FORCE_ALL_SET";
		
	case ID_ROOMS_EXECUTE_FUNC:
		return "ID_ROOMS_EXECUTE_FUNC";
		
	case ID_ROOMS_LOGON_STATUS:
		return "ID_ROOMS_LOGON_STATUS";
		
	case ID_ROOMS_HANDLE_CHANGE:
		return "ID_ROOMS_HANDLE_CHANGE";
		
	case ID_LOBBY2_SEND_MESSAGE:
		return "ID_LOBBY2_SEND_MESSAGE";
		
	case ID_LOBBY2_SERVER_ERROR:
		return "ID_LOBBY2_SERVER_ERROR";
		
	case ID_FCM2_NEW_HOST:
		return "ID_FCM2_NEW_HOST";
		
	case ID_FCM2_REQUEST_FCMGUID:
		return "ID_FCM2_REQUEST_FCMGUID";
		
	case ID_FCM2_RESPOND_CONNECTION_COUNT:
		return "ID_FCM2_RESPOND_CONNECTION_COUNT";
		
	case ID_FCM2_INFORM_FCMGUID:
		return "ID_FCM2_INFORM_FCMGUID";
		
	case ID_FCM2_UPDATE_MIN_TOTAL_CONNECTION_COUNT:
		return "ID_FCM2_UPDATE_MIN_TOTAL_CONNECTION_COUNT";
		
	case ID_UDP_PROXY_GENERAL:
		return "ID_UDP_PROXY_GENERAL";
		
	case ID_SQLite3_EXEC:
		return "ID_SQLite3_EXEC";
		
	case ID_SQLite3_UNKNOWN_DB:
		return "ID_SQLite3_UNKNOWN_DB";
		
	case ID_SQLLITE_LOGGER:
		return "ID_SQLLITE_LOGGER";
		
	case ID_NAT_TYPE_DETECTION_REQUEST:
		return "ID_NAT_TYPE_DETECTION_REQUEST";
		
	case ID_NAT_TYPE_DETECTION_RESULT:
		return "ID_NAT_TYPE_DETECTION_RESULT";
		
	case ID_ROUTER_2_INTERNAL:
		return "ID_ROUTER_2_INTERNAL";
		
	case ID_ROUTER_2_FORWARDING_NO_PATH:
		return "ID_ROUTER_2_FORWARDING_NO_PATH";
		
	case ID_ROUTER_2_FORWARDING_ESTABLISHED:
		return "ID_ROUTER_2_FORWARDING_ESTABLISHED";
		
	case ID_ROUTER_2_REROUTED:
		return "ID_ROUTER_2_REROUTED";
		
	case ID_TEAM_BALANCER_INTERNAL:
		return "ID_TEAM_BALANCER_INTERNAL";
		
	case ID_TEAM_BALANCER_REQUESTED_TEAM_CHANGE_PENDING:
		return "ID_TEAM_BALANCER_REQUESTED_TEAM_CHANGE_PENDING";
		
	case ID_TEAM_BALANCER_TEAMS_LOCKED:
		return "ID_TEAM_BALANCER_TEAMS_LOCKED";
		
	case ID_TEAM_BALANCER_TEAM_ASSIGNED:
		return "ID_TEAM_BALANCER_TEAM_ASSIGNED";
		
	case ID_LIGHTSPEED_INTEGRATION:
		return "ID_LIGHTSPEED_INTEGRATION";
		
	case ID_XBOX_LOBBY:
		return "ID_XBOX_LOBBY";
		
	case ID_TWO_WAY_AUTHENTICATION_INCOMING_CHALLENGE_SUCCESS:
		return "ID_TWO_WAY_AUTHENTICATION_INCOMING_CHALLENGE_SUCCESS";
		
	case ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_SUCCESS:
		return "ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_SUCCESS";
		
	case ID_TWO_WAY_AUTHENTICATION_INCOMING_CHALLENGE_FAILURE:
		return "ID_TWO_WAY_AUTHENTICATION_INCOMING_CHALLENGE_FAILURE";
		
	case ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_FAILURE:
		return "ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_FAILURE";
		
	case ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_TIMEOUT:
		return "ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_TIMEOUT";
		
	case ID_TWO_WAY_AUTHENTICATION_NEGOTIATION:
		return "ID_TWO_WAY_AUTHENTICATION_NEGOTIATION";
		
	case ID_CLOUD_POST_REQUEST:
		return "ID_CLOUD_POST_REQUEST";
		
	case ID_CLOUD_RELEASE_REQUEST:
		return "ID_CLOUD_RELEASE_REQUEST";
		
	case ID_CLOUD_GET_REQUEST:
		return "ID_CLOUD_GET_REQUEST";
		
	case ID_CLOUD_GET_RESPONSE:
		return "ID_CLOUD_GET_RESPONSE";
		
	case ID_CLOUD_UNSUBSCRIBE_REQUEST:
		return "ID_CLOUD_UNSUBSCRIBE_REQUEST";
		
	case ID_CLOUD_SERVER_TO_SERVER_COMMAND:
		return "ID_CLOUD_SERVER_TO_SERVER_COMMAND";
		
	case ID_CLOUD_SUBSCRIPTION_NOTIFICATION:
		return "ID_CLOUD_SUBSCRIPTION_NOTIFICATION";
		
	case ID_RESERVED_1:
		return "ID_RESERVED_1";
		
	case ID_RESERVED_2:
		return "ID_RESERVED_2";
		
	case ID_RESERVED_3:
		return "ID_RESERVED_3";
		
	case ID_RESERVED_4:
		return "ID_RESERVED_4";
		
	case ID_RESERVED_5:
		return "ID_RESERVED_5";
		
	case ID_RESERVED_6:
		return "ID_RESERVED_6";
		
	case ID_RESERVED_7:
		return "ID_RESERVED_7";
		
	case ID_RESERVED_8:
		return "ID_RESERVED_8";
		
	case ID_RESERVED_9:
		return "ID_RESERVED_9";
		
	default:
		break;
	}
	return "Unknown or user-defined";
}
#endif

int RakNetInstance::handleUnconnectedPong(const RakNet::RakString& data, const RakNet::Packet* p, const char* appid, bool insertAtBeginning)
{
	RakNet::RakString appIdentifier(appid);
	// This weird code is a result of RakString.Find being pretty useless
	bool emptyNameOrLonger = data.GetLength() >= appIdentifier.GetLength();

	if ( !emptyNameOrLonger || appIdentifier.StrCmp(data.SubStr(0, appIdentifier.GetLength())) != 0)
		return -1;

	bool found = false;
	for (unsigned int i = 0; i < availableServers.size(); i++) {
		if (availableServers[i].address == p->systemAddress) {
			availableServers[i].pingTime = RakNet::GetTimeMS();

			bool emptyName = data.GetLength() == appIdentifier.GetLength();
			if (emptyName)
				availableServers[i].name = "";
			else {
				availableServers[i].name = data.SubStr(appIdentifier.GetLength(), data.GetLength() - appIdentifier.GetLength());
			}
			//LOGI("Swapping name: %s\n", availableServers[i].name.C_String());
			return i;
		}
	}
	PingedCompatibleServer server;
	server.address = p->systemAddress;
	server.pingTime = RakNet::GetTimeMS();
	server.isSpecial = false;
	server.name = data.SubStr(appIdentifier.GetLength(), data.GetLength() - appIdentifier.GetLength());

	if (insertAtBeginning) {
		availableServers.insert(availableServers.begin(), server);
		return 0;
	} else {
		availableServers.push_back(server);
		return availableServers.size() - 1;
	}
}

void RakNetInstance::setIsLoggedIn( bool status ) {
	_isLoggedIn = status;
}
