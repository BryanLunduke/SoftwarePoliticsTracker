#ifndef NET_MINECRAFT_NETWORK_PACKET__AdventureSettingsPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__AdventureSettingsPacket_H__

#include "../Packet.h"
#include "../../world/level/Level.h"

class AdventureSettingsPacket: public Packet
{
public:
	enum Flags {
		WorldImmutable	= 1,
		NoPvP			= 2,
		NoPvM			= 4,
		NoMvP			= 8,
		StaticTime		= 16,
		ShowNameTags	= 32,
	};

	AdventureSettingsPacket() {}

	AdventureSettingsPacket(const AdventureSettings& settings)
	:	flags(0)
	{
		set(WorldImmutable,	settings.immutableWorld);
		set(NoPvP,			settings.noPvP);
		set(NoPvM,			settings.noPvM);
		set(NoMvP,			settings.noMvP);
		set(StaticTime,		!settings.doTickTime);
		set(ShowNameTags,	settings.showNameTags);
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_ADVENTURESETTINGS));
		bitStream->Write(flags);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(flags);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (AdventureSettingsPacket*)this);
	}

	void fillIn( AdventureSettings& adventureSettings ) const
	{
		adventureSettings.immutableWorld= isSet(WorldImmutable);
		adventureSettings.noPvP			= isSet(NoPvP);
		adventureSettings.noPvM			= isSet(NoPvM);
		adventureSettings.noMvP			= isSet(NoMvP);
		adventureSettings.doTickTime	= !isSet(StaticTime);
		adventureSettings.showNameTags	= isSet(ShowNameTags);
	}

	unsigned int flags;

	void   set(Flags flag, bool status) { status? set(flag) : clear(flag); }
	void   set(Flags flag) { flags |= flag; }
	void toggle(Flags flag){ flags ^= flag; }
	void clear(Flags flag) { flags &= ~flag; }
	bool isSet(Flags flag) const { return (flags & flag) != 0; }
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__AdventureSettingsPacket_H__*/
