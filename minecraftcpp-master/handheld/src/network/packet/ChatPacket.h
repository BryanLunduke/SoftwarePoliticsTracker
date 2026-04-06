#ifndef NET_MINECRAFT_NETWORK_PACKET__ChatPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ChatPacket_H__
#include "../Packet.h"
#include "../..//world/entity/player/Player.h"

class ChatPacket: public Packet
{
public:
	static const int MAX_CHAT_LENGTH = 100;
	static const int MAX_LENGTH = MAX_CHAT_LENGTH + Player::MAX_NAME_LENGTH;
	ChatPacket() {
	}

	ChatPacket(std::string message, bool isSystem = true) {
		if(message.length() > MAX_LENGTH) {
			message = message.substr(0, MAX_LENGTH);
		}
		this->message = message;
		this->isSystem = isSystem;
	}

	void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CHAT));
		bitStream->Write(message.c_str());
	}

	void read(RakNet::BitStream* bitStream) {
		char buff[MAX_LENGTH + 30];
		bitStream->Read(buff);
		message = buff;
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (ChatPacket*)this);
	}

	std::string message;
	bool isSystem;
};

#endif /* NET_MINECRAFT_NETWORK_PACKET__ChatPacket_H__ */