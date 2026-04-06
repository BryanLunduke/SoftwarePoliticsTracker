#ifndef NET_MINECRAFT_NETWORK_PACKET__ExplodePacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ExplodePacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"
#include "../../world/level/Explosion.h"

class ExplodePacket: public Packet
{
public:
    float x, y, z;
    float r;
	std::vector<TilePos> toBlow;

    ExplodePacket() {}

    ExplodePacket(float x, float y, float z, float r, const TilePosSet& tiles)
	:	x(x),
		y(y),
		z(z),
		r(r),
		toBlow(tiles.begin(), tiles.end())
	{}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_EXPLODE));
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(r);
		int xp = (int)x;
		int yp = (int)y;
		int zp = (int)z;

		int count = (int)toBlow.size();
		bitStream->Write(count);
		for (int i = 0; i < count; ++i) {
			const TilePos& tp = toBlow[i];
			bitStream->Write((signed char)(tp.x - xp));
			bitStream->Write((signed char)(tp.y - yp));
			bitStream->Write((signed char)(tp.z - zp));
		}
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(r);
		int xp = (int)x;
		int yp = (int)y;
		int zp = (int)z;

		// Write the tileset for the exploded tiles
		int count;
		bitStream->Read(count);
		toBlow.reserve(count);
		for (int i = 0; i < count; ++i) {
			signed char xx,yy,zz;
			bitStream->Read(xx);
			bitStream->Read(yy);
			bitStream->Read(zz);
			toBlow.push_back( TilePos(xp + xx, yp + yy, zp + zz) );
		}
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (ExplodePacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ExplodePacket_H__*/
