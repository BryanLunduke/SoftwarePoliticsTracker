#ifndef NET_MINECRAFT_NETWORK_PACKET__AddPaintingPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__AddPaintingPacket_H__
#include "../Packet.h"
#include "../../world/entity/Painting.h"
class AddPaintingPacket : public Packet {
public:
	AddPaintingPacket() : entityId(0), xTile(0), yTile(0), zTile(0), dir(-1) {
	
	}
	AddPaintingPacket(Painting* painting) {
		entityId = painting->entityId;
		xTile = painting->xTile;
		yTile = painting->yTile;
		zTile = painting->zTile;
		dir = painting->dir;
		motive = painting->motive->name;
	}
	void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_ADDPAINTING));
		bitStream->Write(entityId);
		bitStream->Write(xTile);
		bitStream->Write(yTile);
		bitStream->Write(zTile);
		bitStream->Write(dir);
		RakNet::RakString rakMotive(motive.c_str());
		bitStream->Write(rakMotive);
	}
	void read(RakNet::BitStream* bitStream) {
		bitStream->Read(entityId);
		bitStream->Read(xTile);
		bitStream->Read(yTile);
		bitStream->Read(zTile);
		bitStream->Read(dir);
		RakNet::RakString rakMotive;
		bitStream->Read(rakMotive);
		motive = std::string(rakMotive.C_String());
	}
	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (AddPaintingPacket*)this);
	}
public:
	int entityId;
	int xTile, yTile, zTile;
	int dir;
	std::string motive;
};

#endif /* NET_MINECRAFT_NETWORK_PACKET__AddPaintingPacket_H__ */