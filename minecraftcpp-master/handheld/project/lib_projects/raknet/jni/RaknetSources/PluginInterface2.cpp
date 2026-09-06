/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "PluginInterface2.h"
#include "PacketizedTCP.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

using namespace RakNet;

PluginInterface2::PluginInterface2()
{
	rakPeerInterface=0;
#if _RAKNET_SUPPORT_PacketizedTCP==1 && _RAKNET_SUPPORT_TCPInterface==1
	packetizedTCP=0;
#endif
}
PluginInterface2::~PluginInterface2()
{

}
void PluginInterface2::SendUnified( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast )
{
	if (rakPeerInterface)
		rakPeerInterface->Send(bitStream,priority,reliability,orderingChannel,systemIdentifier,broadcast);
#if _RAKNET_SUPPORT_PacketizedTCP==1 && _RAKNET_SUPPORT_TCPInterface==1
	else
		packetizedTCP->Send((const char*) bitStream->GetData(), bitStream->GetNumberOfBytesUsed(), systemIdentifier.systemAddress, broadcast);
#endif
}
Packet *PluginInterface2::AllocatePacketUnified(unsigned dataSize)
{
	if (rakPeerInterface)
		return rakPeerInterface->AllocatePacket(dataSize);
#if _RAKNET_SUPPORT_PacketizedTCP==1 && _RAKNET_SUPPORT_TCPInterface==1
	else
		return packetizedTCP->AllocatePacket(dataSize);
#else
	return 0;
#endif
		
}
void PluginInterface2::PushBackPacketUnified(Packet *packet, bool pushAtHead)
{
	if (rakPeerInterface)
		rakPeerInterface->PushBackPacket(packet,pushAtHead);
#if _RAKNET_SUPPORT_PacketizedTCP==1 && _RAKNET_SUPPORT_TCPInterface==1
	else
		packetizedTCP->PushBackPacket(packet,pushAtHead);
#endif
}
void PluginInterface2::DeallocPacketUnified(Packet *packet)
{
	if (rakPeerInterface)
		rakPeerInterface->DeallocatePacket(packet);
#if _RAKNET_SUPPORT_PacketizedTCP==1 && _RAKNET_SUPPORT_TCPInterface==1
	else
		packetizedTCP->DeallocatePacket(packet);
#endif
}
bool PluginInterface2::SendListUnified( const char **data, const int *lengths, const int numParameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast )
{
	if (rakPeerInterface)
	{
		return rakPeerInterface->SendList(data,lengths,numParameters,priority,reliability,orderingChannel,systemIdentifier,broadcast)!=0;
	}
#if _RAKNET_SUPPORT_PacketizedTCP==1 && _RAKNET_SUPPORT_TCPInterface==1
	else
	{
		return packetizedTCP->SendList(data,lengths,numParameters,systemIdentifier.systemAddress,broadcast );
	}
#else
	return false;
#endif
}
void PluginInterface2::SetRakPeerInterface( RakPeerInterface *ptr )
{
	rakPeerInterface=ptr;
}
#if _RAKNET_SUPPORT_PacketizedTCP==1 && _RAKNET_SUPPORT_TCPInterface==1
void PluginInterface2::SetPacketizedTCP( PacketizedTCP *ptr )
{
	packetizedTCP=ptr;
}
#endif
