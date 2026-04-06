#ifndef COMMANDSERVER_H__
#define COMMANDSERVER_H__

#include <string>
#include <vector>

#ifdef WIN32
	#include <WinSock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

#include "../../world/PosTranslator.h"
#include "../../world/Pos.h"

class Minecraft;
class Packet;
class CameraEntity;

class ConnectedClient {
public:
	ConnectedClient(int socketFd)
	:	socket(socketFd),
		lastPoll_blockHit(0)
	{
	}

	int socket;
	std::string data;

	int lastPoll_blockHit;
};

class CommandServer {
public:
	CommandServer(Minecraft* mc);
	~CommandServer();

	bool init(short port);
	void tick();
	//void update();
private:
	std::string parse(ConnectedClient& client, const std::string& s);
	void _close();

	void _updateAccept();
	void _updateClients();
	// return true if client is in error/should be removed, false if not
	bool _updateClient(ConnectedClient& client);

	bool handleCheckpoint(bool doRestore);

	void dispatchPacket(Packet& p);
	std::string handleEventPollMessage( ConnectedClient& client, const std::string& cmd );
	std::string handleSetSetting(const std::string& setting, int value);

	bool inited;
	int serverSocket;
	struct sockaddr_in serverAddress;

	Minecraft* mc;
	OffsetPosTranslator apiPosTranslate;

	static const int RestoreHeight = 48;
	unsigned char* restoreBuffer;
	Pos restorePos;

	CameraEntity* camera;

	std::vector<ConnectedClient> clients;

	static const std::string Ok;
	static const std::string Fail;
};

#endif /*COMMANDSERVER_H__*/
