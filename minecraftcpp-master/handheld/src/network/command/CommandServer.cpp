#include "CommandServer.h"
#include "../../client/Minecraft.h"
#include "../../world/level/Level.h"
#include "../../world/entity/Entity.h"

#ifdef WIN32
	#define SERR(x) (WSA ## x)
#else
	#define SERR(x) (x)
    #include <errno.h>
	#include <unistd.h>
#endif

#include "../RakNetInstance.h"
#include "../packet/ChatPacket.h"
#include "../packet/AdventureSettingsPacket.h"
#include "../../world/level/LevelSettings.h"
#include "../../world/entity/player/Player.h"
#include "../../client/gamemode/CreatorMode.h"
#include "../../client/player/LocalPlayer.h"
#include "../RakNetInstance.h"

const std::string NullString;
const std::string CommandServer::Ok("\n");
const std::string CommandServer::Fail("Fail\n");

static bool setSocketBlocking(int socket, bool blocking);

// Return value < 0 means socket is shut down / broken
static int Readline(ConnectedClient* client, std::string& out, int maxlen);
static int Writeline(ConnectedClient* client, const std::string& in, int maxlen);

class CameraEntity: public Mob
{
	typedef Mob super;
public:
	CameraEntity(Level* level)
	:	super(level),
		followEntityId(-1)
	{
		moveTo(128, 72, 128, 0, 90.0f);
	}

	void follow(int entityId) {
		followEntityId = entityId;
	}

	void tick() {
		if (followEntityId < 0) return;

		xOld = xo = x;
		yOld = yo = y;
		zOld = zo = z;
		xRotO = xRot;
		yRotO = yRot;

		Entity* e = level->getEntity(followEntityId);
		if (!e) return;
		setPos(e->x, e->y + 6, e->z);
	}

	int getEntityTypeId() const { return 0; }

private:
	int followEntityId;
};

static int getSocketError() {
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

static int t = 0;

template <typename T>
static std::string ToStringOk(const T& a) {
	std::stringstream ss;
	ss << a << "\n";
	return ss.str();
}
template <typename T>
static std::string ToStringOk(const T& a, const T& b) {
	std::stringstream ss;
	ss << a << "," << b << "\n";
	return ss.str();
}
template <typename T>
static std::string ToStringOk(const T& a, const T& b, const T& c) {
	std::stringstream ss;
	ss << a << "," << b << "," << c << "\n";
	return ss.str();
}

static bool inRange(int c, int lowInclusive, int highInclusive) {
	return c >= lowInclusive && c <= highInclusive;
}

CommandServer::CommandServer( Minecraft* mc )
:	mc(mc),
	serverSocket(0),
	restoreBuffer(0),
	inited(false)
{
	camera = new CameraEntity(mc->level);

	Pos p = mc->level->getSharedSpawnPos();
	apiPosTranslate = OffsetPosTranslator((float)-p.x, (float)-p.y, (float)-p.z);
}

CommandServer::~CommandServer() {
	_close();

	delete camera;
	delete[] restoreBuffer;
}

void CommandServer::_close()
{
	if (inited) {
		if (serverSocket > 0) {
		#ifdef WIN32
			closesocket(serverSocket);
		#else
			close(serverSocket);
		#endif
		}
		inited = false;
		serverSocket = 0;
	}
}

bool CommandServer::init(short port) {
	_close();

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Failed creating socket - 1\n");
		return false;
	}

	setSocketBlocking(serverSocket, false);

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family      = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port        = htons(port);

	int enabled = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enabled, sizeof(enabled));

	if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		printf("Failed binding socket - 2\n");
		return false;
	}

	if (listen(serverSocket, 128) < 0 ) {
		printf("Failed listening on socket - 3\n");
		return false;
	}

	LOGI("Listening on port %d\n", port);
	inited = true;
	return true;
}

std::string CommandServer::parse(ConnectedClient& client, const std::string& s) {
	int b = s.find("(");
	if (b == std::string::npos) {
		return Fail;
	}

	int e = s.rfind(")");
	if (b == std::string::npos) {
		return Fail;
	}

	std::string cmd = s.substr(0, b);
	std::string rest = s.substr(b+1, e-b-1);

	//
	// Block related get, set and query
	//
	if (cmd == "world.setBlock") {
		int x, y, z, id, data;
		bool hasData = true;

		int c = sscanf(rest.c_str(), "%d,%d,%d,%d,%d", &x, &y, &z, &id, &data);
		if (!inRange(c, 4, 5)) return Fail;
		if (c == 4) hasData = false;

		id &= 255;
		if (id > 0 && !Tile::tiles[id])
			return Fail;

		apiPosTranslate.from(x, y, z);

		if (hasData)
			mc->level->setTileAndData(x, y, z, id, data & 15);
		else
			mc->level->setTile(x, y, z, id);

		return NullString;
	}

	if (cmd == "world.getBlock") {
		int x, y, z;
		if (3 != sscanf(rest.c_str(), "%d,%d,%d", &x, &y, &z)) {
			return Fail;
		}
		apiPosTranslate.from(x, y, z);
		return ToStringOk(mc->level->getTile(x, y, z));
	}

	if (cmd == "world.setBlocks") {
		int x0, y0, z0, x1, y1, z1, id, data;
		bool hasData = true;

		int c = sscanf(rest.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d", &x0, &y0, &z0, &x1, &y1, &z1, &id, &data);
		if (!inRange(c, 7, 8)) return Fail;
		if (c == 7) hasData = false;

		id &= 255;

		if (id > 0 && !Tile::tiles[id])
			return Fail;

		if (x0 > x1) std::swap(x0, x1);
		if (y0 > y1) std::swap(y0, y1);
		if (z0 > z1) std::swap(z0, z1);

		apiPosTranslate.from(x0, y0, z0);
		apiPosTranslate.from(x1, y1, z1);

		if (x0 < 0) x0 = 0;
		if (y0 < 0) y0 = 0;
		if (z0 < 0) z0 = 0;
		if (x1 >= LEVEL_WIDTH ) x1 = LEVEL_WIDTH  - 1;
		if (y1 >= LEVEL_HEIGHT) y1 = LEVEL_HEIGHT - 1;
		if (z1 >= LEVEL_DEPTH ) z1 = LEVEL_DEPTH  - 1;

		for (int y = y0; y <= y1; ++y)
		for (int z = z0; z <= z1; ++z)
		for (int x = x0; x <= x1; ++x) {
			if (hasData)
				mc->level->setTileAndData(x, y, z, id, data & 15);
			else
				mc->level->setTile(x, y, z, id);
		}
		return NullString;
	}

	if (cmd == "world.getHeight") {
		int x, z;
		if (2 != sscanf(rest.c_str(), "%d,%d", &x, &z)) {
			return Fail;
		}
		x -= (int)apiPosTranslate.xo;
		z -= (int)apiPosTranslate.zo;
		const int y = mc->level->getHeightmap(x, z) + (int)apiPosTranslate.yo;
		return ToStringOk(y);
	}

	//
	// Player related get, set and query
	//
	if (cmd == "player.setTile") {
		if (!mc->player)
			return Fail;

		int x, y, z;
		if (3 != sscanf(rest.c_str(), "%d,%d,%d", &x, &y, &z)) {
			return Fail;
		}

		apiPosTranslate.from(x, y, z);
		Entity* e = (Entity*) mc->player;
		e->moveTo((float)x + 0.5f, (float)y, (float)z + 0.5f, e->yRot, e->xRot);
		return NullString;
	}

	if (cmd == "player.getTile") {
		if (!mc->player)
			return Fail;

		Entity* e = (Entity*) mc->player;

		int x = (int)e->x, y = (int)(e->y - e->heightOffset), z = (int)e->z;
		apiPosTranslate.to(x, y, z);
		return ToStringOk(x, y, z);
	}

	if (cmd == "player.setPos") {
		if (!mc->player)
			return Fail;

		float x, y, z;
		if (3 != sscanf(rest.c_str(), "%f,%f,%f", &x, &y, &z)) {
			return Fail;
		}

		apiPosTranslate.from(x, y, z);
		Entity* e = (Entity*) mc->player;
		e->moveTo(x, y, z, e->yRot, e->xRot);
		return NullString;
	}

	if (cmd == "player.getPos") {
		if (!mc->player)
			return Fail;

		Entity* e = (Entity*) mc->player;

		float x = e->x, y = e->y - e->heightOffset, z = e->z;
		apiPosTranslate.to(x, y, z);
		return ToStringOk(x, y, z);
	}

	//
	// Entity
	//
	if (cmd == "entity.setTile") {
		int id, x, y, z;
		if (4 != sscanf(rest.c_str(), "%d,%d,%d,%d", &id, &x, &y, &z)) {
			return Fail;
		}
		Entity* e = mc->level->getEntity(id);
		if (!e) return Fail;

		apiPosTranslate.from(x, y, z);
		e->moveTo((float)x + 0.5f, (float)y, (float)z + 0.5f, e->yRot, e->xRot);
		return NullString;
	}

	if (cmd == "entity.getTile") {
		int id;
		if (1 != sscanf(rest.c_str(), "%d", &id))
			return Fail;

		Entity* e = mc->level->getEntity(id);
		if (!e) return Fail;

		int x = (int)e->x, y = (int)(e->y - e->heightOffset), z = (int)e->z;
		apiPosTranslate.to(x, y, z);
		return ToStringOk(x, y, z);
	}

	if (cmd == "entity.setPos") {
		int id;
		float x, y, z;
		if (4 != sscanf(rest.c_str(), "%d,%f,%f,%f", &id, &x, &y, &z)) {
			return Fail;
		}
		Entity* e = mc->level->getEntity(id);
		if (!e) return Fail;

		apiPosTranslate.from(x, y, z);
		e->moveTo(x, y, z, e->yRot, e->xRot);
		return NullString;
	}

	if (cmd == "entity.getPos") {
		int id;
		if (1 != sscanf(rest.c_str(), "%d", &id))
			return Fail;

		Entity* e = mc->level->getEntity(id);
		if (!e) return Fail;

		float x = e->x, y = e->y - e->heightOffset, z = e->z;
		apiPosTranslate.to(x, y, z);
		return ToStringOk(x, y, z);
	}

	//
	// Chat
	//
	if (cmd == "chat.post") {
#ifndef STANDALONE_SERVER
		mc->gui.addMessage(rest);
#endif
		ChatPacket p(rest, false);
		dispatchPacket(p);
		return NullString;
	}

	//
	// Camera
	//
	if (cmd == "camera.mode.setFixed") {
		camera->follow(-1);
		mc->cameraTargetPlayer = camera;
		return NullString;
	}
	if (cmd == "camera.mode.setNormal") {
                int entityId = -1;
                if (!rest.empty()) {
                        if (1 != sscanf(rest.c_str(), "%d", &entityId)) return Fail;
		}
	        if (entityId > 0) {
			Entity* e = mc->level->getEntity(entityId);
			if (e && e->isMob()) mc->cameraTargetPlayer = (Mob*)e;
		} else {
			mc->cameraTargetPlayer = (Mob*)mc->player;
		}
		return NullString;
	}

	if (cmd == "camera.mode.setFollow") {
		int entityId = -1;
		if (!rest.empty()) {
			if (1 != sscanf(rest.c_str(), "%d", &entityId)) return Fail;
		}
		if (entityId < 0) entityId = mc->player->entityId;

		camera->follow(entityId);
		mc->cameraTargetPlayer = camera;
		return NullString;
	}

	if (cmd == "camera.setPos") {
		float x, y, z;
		if (3 != sscanf(rest.c_str(), "%f,%f,%f", &x, &y, &z)) {
			return Fail;
		}

		apiPosTranslate.from(x, y, z);
		Entity* e = (Entity*) mc->cameraTargetPlayer;
		e->moveTo((float)x + 0.5f, (float)y, (float)z + 0.5f, e->yRot, e->xRot);
		return NullString;
	}

	//
	// Entities
	//
	if (cmd == "world.getPlayerIds") {
		std::stringstream s;
		int size = mc->level->players.size();
		for (int i = 0; i < size; ++i) {
			if (i != 0) s << "|";
			s << mc->level->players[i]->entityId;
		}
		s << "\n";
		return s.str();
	}

	//
	// Set and restore Checkpoint
	//
	if (cmd == "world.checkpoint.save") {
		if (mc->player) {
			Entity* e = (Entity*) mc->player;

			static Stopwatch sw;
			sw.start();

			// Save a cuboid around the player
			const int CSize = CHUNK_CACHE_WIDTH;
			int cx = (int)e->x / CSize;
			int cz = (int)e->z / CSize;

			restorePos = Pos(cx, (int)e->y - 8, cz);
			handleCheckpoint(false);

			sw.stop();
			sw.printEvery(1, "set-checkpoint");
		}
	}

	if (cmd == "world.checkpoint.restore") {
		bool success = handleCheckpoint(true);
		if (success) {
			int xx = 16 * (restorePos.x - 2);
			int zz = 16 * (restorePos.z - 2);
			mc->level->setTilesDirty(xx, restorePos.y, zz,
				xx + 5 * 16, restorePos.y + RestoreHeight, zz + 5 * 16);
		}
		return success? NullString : Fail;
	}

	//
	// Event queries
	//
	if (cmd.find("events.") == 0) {
		return handleEventPollMessage(client, cmd);
	}

	// Settings
	if (cmd.find("player.setting") == 0
	||  cmd.find("world.setting") == 0) {
		int value;
		static char name[1024];
		if (rest.find(",") >= 100) return Fail;
		if (2 != sscanf(rest.c_str(), "%[^,],%d", name, &value)) return Fail;
		return handleSetSetting(name, value);
	}

    return NullString;
}

bool CommandServer::handleCheckpoint(bool doRestore ) {
	const int cx = restorePos.x;
	const int cz = restorePos.z;
	const int y0 = restorePos.y;
	const int y1 = y0 + RestoreHeight;
	const int CSize = CHUNK_CACHE_WIDTH;
	const int numChunkBytes = RestoreHeight * CSize * CSize * 20 / 8;

	if (!restoreBuffer) {
		if (doRestore) return false;

		int numBytes = 5 * 5 * numChunkBytes;
		restoreBuffer = new unsigned char[numBytes];
	}

	int offset = 0;
	for (int z = cz - 2; z <= cz + 2; ++z)
	for (int x = cx - 2; x <= cx + 2; ++x) {
		LevelChunk* c = mc->level->getChunk(x, z);
		if (!c) continue;

		if (doRestore) {
			//LOGI("restoring: %d, %d\n", x, z);
			c->setBlocksAndData(restoreBuffer, 0, y0, 0, CSize, y1, CSize, offset);
		} else {
			//LOGI("saving: %d, %d\n", x, z);
			c->getBlocksAndData(restoreBuffer, 0, y0, 0, CSize, y1, CSize, offset);
		}
		offset += numChunkBytes;
	}
	return true;
}

void CommandServer::tick() {
	if (!inited)
		return;

	_updateAccept();
	_updateClients();
	++t;

	if (mc->cameraTargetPlayer == camera) {
		camera->tick();
	}
}

void CommandServer::_updateAccept() {
	int fd = accept(serverSocket, NULL, NULL);
	if (fd == -1) {
		int err = getSocketError();
		if (err != SERR(EWOULDBLOCK)) {
			LOGE("Error when trying to accept connections. Error ID: %d\n", err);
		}
		return;
	}
	setSocketBlocking(fd, false);
	clients.push_back(ConnectedClient(fd));

	ConnectedClient& c = clients[clients.size()-1];
	c.lastPoll_blockHit = mc->level->getTime();
}

void CommandServer::_updateClients() {
	for (int i = clients.size() - 1; i >= 0; --i) {
		if (!_updateClient(clients[i]))
			clients.erase(clients.begin() + i);
	}
}

bool CommandServer::_updateClient(ConnectedClient& client) {
	std::string line;
	int maxReadCount = 32;

	while (--maxReadCount >= 0) {
		int ret = Readline(&client, line, 1024);
		//printf("Read: %s @ %d\n", client.data.c_str(), t);
		if (ret)
			return ret > 0;

		std::string response = parse(client, line);
		if (NullString != response)
			if (Writeline(&client, response, 1024) < 0)
				return false;
	}
	return true;
}

void CommandServer::dispatchPacket( Packet& p ) {
	if (!mc->netCallback || !mc->player) return;
	const RakNet::RakNetGUID& guid = ((Player*)mc->player)->owner;
	mc->raknetInstance->send(p);
	//p.handle(guid, mc->netCallback);
}

std::string CommandServer::handleEventPollMessage( ConnectedClient& client, const std::string& cmd ) {
	ICreator* c = mc->getCreator();
	if (!c) {
		return Fail;
	}

	if (cmd == "events.clear") {
		long t = mc->level->getTime();
		client.lastPoll_blockHit = t;
		return NullString;
	}

	if (cmd == "events.block.hits") {
		ICreator::EventList<ICreator::TileEvent>& events = c->getTileEvents();
		std::stringstream ss;

		events.write(ss, apiPosTranslate, client.lastPoll_blockHit);
		client.lastPoll_blockHit = mc->level->getTime();

		ss << "\n";
		return ss.str();
	}

	return Fail;
}

void updateAdventureSettingFlag(Minecraft* mc, AdventureSettingsPacket::Flags flag, bool status) {
	AdventureSettingsPacket p(mc->level->adventureSettings);
	p.set(flag, status);
	p.fillIn(mc->level->adventureSettings);
	mc->raknetInstance->send(p);
}

std::string CommandServer::handleSetSetting( const std::string& setting, int value )
{
	bool status = value != 0;

	if (setting == "autojump") mc->player->autoJumpEnabled = status;

	AdventureSettingsPacket::Flags flag = (AdventureSettingsPacket::Flags)0;
	if (setting == "nametags_visible") flag = AdventureSettingsPacket::ShowNameTags;
	if (setting == "world_immutable") flag = AdventureSettingsPacket::WorldImmutable;

	if (flag != 0)
		updateAdventureSettingFlag(mc, flag, status);

	return NullString;
}



bool setSocketBlocking(int socket, bool blocking) {
	if (socket< 0) {
		return false;
	}

#ifdef WIN32
	unsigned long mode = blocking ? 0 : 1;
	return (ioctlsocket(socket, FIONBIO, &mode) == 0) ? true : false;
#else
	int flags = fcntl(socket, F_GETFL, 0);
	if (flags < 0) return false;
	flags = blocking ? (flags & ~O_NONBLOCK) : (flags|O_NONBLOCK);
	return (fcntl(socket, F_SETFL, flags) == 0) ? true : false;
#endif
}

int Readline(ConnectedClient* client, std::string& out, int maxlen) {
	static char data[2048];
	char* buffer = data;

	if (!client->data.empty()) {
		memcpy(data, client->data.c_str(), client->data.length());
		client->data.clear();
	}

	bool socketError = false;
    for (int n = 1; n < maxlen; n++ ) {
		int rc;
		char c;
		if ( (rc = recv(client->socket, &c, 1, 0)) == 1 ) {
			*buffer++ = c;

			if ( c == '\n' ) {
				*buffer = 0;
				out.assign(data, buffer + 1);
				return 0;
			}
		}
		else if ( rc == 0 ) {
			socketError = true;
			break;
		}
		else if ( rc == -1) {
			int err = getSocketError();
			if (err == SERR(EINTR))
				continue;
			socketError = (err != SERR(EWOULDBLOCK));
			break;
		} else {
			break;
		}
    }

	client->data.assign(data, buffer + 1);
	return socketError? -1 : 1;
}

int Writeline(ConnectedClient* client, const std::string& in, int maxlen) {
    size_t      left = in.length();
    const char *buffer = in.c_str();

    while ( left > 0 ) {
		int n;
		if ( (n = send(client->socket, buffer, left, 0)) <= 0 ) {
			int err = getSocketError();
			if (err == SERR(EINTR))
				n = 0;
			else
				return (err == SERR(EWOULDBLOCK))? 1 : -1;
		}
		left  -= n;
		buffer += n;
    }
    return 0;
}
