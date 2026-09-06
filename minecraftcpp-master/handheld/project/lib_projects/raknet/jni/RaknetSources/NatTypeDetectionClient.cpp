#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_NatTypeDetectionClient==1

#include "NatTypeDetectionClient.h"
#include "RakNetSocket.h"
#include "RakNetSmartPtr.h"
#include "BitStream.h"
#include "SocketIncludes.h"
#include "RakString.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "SocketLayer.h"
#include "SocketDefines.h"

using namespace RakNet;

STATIC_FACTORY_DEFINITIONS(NatTypeDetectionClient,NatTypeDetectionClient);

NatTypeDetectionClient::NatTypeDetectionClient()
{
	c2=INVALID_SOCKET;
}
NatTypeDetectionClient::~NatTypeDetectionClient()
{
	if (c2!=INVALID_SOCKET)
	{
		closesocket__(c2);
	}
}
void NatTypeDetectionClient::DetectNATType(SystemAddress _serverAddress)
{
	if (IsInProgress())
		return;

	if (c2==INVALID_SOCKET)
	{
		DataStructures::List<RakNetSmartPtr<RakNetSocket> > sockets;
		rakPeerInterface->GetSockets(sockets);
		SystemAddress sockAddr;
		SocketLayer::GetSystemAddress(sockets[0]->s, &sockAddr);
		char str[64];
		sockAddr.ToString(false,str);
		c2=CreateNonblockingBoundSocket(str);
		c2Port=SocketLayer::GetLocalPort(c2);
	}


	serverAddress=_serverAddress;

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_NAT_TYPE_DETECTION_REQUEST);
	bs.Write(true); // IsRequest
	bs.Write(c2Port);
	rakPeerInterface->Send(&bs,MEDIUM_PRIORITY,RELIABLE,0,serverAddress,false);
}
void NatTypeDetectionClient::OnCompletion(NATTypeDetectionResult result)
{
	Packet *p = AllocatePacketUnified(sizeof(MessageID)+sizeof(unsigned char)*2);
	printf("Returning nat detection result to the user\n");
	p->data[0]=ID_NAT_TYPE_DETECTION_RESULT;
	p->systemAddress=serverAddress;
	p->systemAddress.systemIndex=(SystemIndex)-1;
	p->guid=rakPeerInterface->GetGuidFromSystemAddress(serverAddress);
	p->data[1]=(unsigned char) result;
	p->wasGeneratedLocally=true;
	rakPeerInterface->PushBackPacket(p, true);

	// Symmetric and port restricted are determined by server, so no need to notify server we are done
	if (result!=NAT_TYPE_PORT_RESTRICTED && result!=NAT_TYPE_SYMMETRIC)
	{
		// Otherwise tell the server we got this message, so it stops sending tests to us
		RakNet::BitStream bs;
		bs.Write((unsigned char)ID_NAT_TYPE_DETECTION_REQUEST);
		bs.Write(false); // Done
		rakPeerInterface->Send(&bs,HIGH_PRIORITY,RELIABLE,0,serverAddress,false);
	}

	Shutdown();
}
bool NatTypeDetectionClient::IsInProgress(void) const
{
	return serverAddress!=UNASSIGNED_SYSTEM_ADDRESS;
}
void NatTypeDetectionClient::Update(void)
{
	if (IsInProgress())
	{
		char data[ MAXIMUM_MTU_SIZE ];
		int len;
		SystemAddress sender;
		len=NatTypeRecvFrom(data, c2, sender);
		if (len==1 && data[0]==NAT_TYPE_NONE)
		{
			OnCompletion(NAT_TYPE_NONE);
			RakAssert(IsInProgress()==false);
		}
	}
}
PluginReceiveResult NatTypeDetectionClient::OnReceive(Packet *packet)
{
	if (IsInProgress())
	{
		switch (packet->data[0])
		{
		case ID_OUT_OF_BAND_INTERNAL:
			{
				if (packet->length>=3 && packet->data[1]==ID_NAT_TYPE_DETECT)
				{
					OnCompletion((NATTypeDetectionResult)packet->data[2]);
					return RR_STOP_PROCESSING_AND_DEALLOCATE;
				}
			}
			break;
		case ID_NAT_TYPE_DETECTION_RESULT:
			if (packet->wasGeneratedLocally==false)
			{
				OnCompletion((NATTypeDetectionResult)packet->data[1]);
				return RR_STOP_PROCESSING_AND_DEALLOCATE;
			}
			else
				break;
		case ID_NAT_TYPE_DETECTION_REQUEST:
			OnTestPortRestricted(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
	}

	return RR_CONTINUE_PROCESSING;
}
void NatTypeDetectionClient::OnClosedConnection(const SystemAddress &systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) rakNetGUID;

	if (IsInProgress() && systemAddress==serverAddress)
		Shutdown();
}
void NatTypeDetectionClient::OnTestPortRestricted(Packet *packet)
{
	RakNet::BitStream bsIn(packet->data,packet->length,false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	RakNet::RakString s3p4StrAddress;
	bsIn.Read(s3p4StrAddress);
	unsigned short s3p4Port;
	bsIn.Read(s3p4Port);

	DataStructures::List<RakNetSmartPtr<RakNetSocket> > sockets;
	rakPeerInterface->GetSockets(sockets);
	SystemAddress s3p4Addr = sockets[0]->boundAddress;
	s3p4Addr.FromStringExplicitPort(s3p4StrAddress.C_String(), s3p4Port);

	// Send off the RakNet socket to the specified address, message is unformatted
	// Server does this twice, so don't have to unduly worry about packetloss
	RakNet::BitStream bsOut;
	bsOut.Write((MessageID) NAT_TYPE_PORT_RESTRICTED);
	bsOut.Write(rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
	SocketLayer::SendTo_PC( sockets[0]->s, (const char*) bsOut.GetData(), bsOut.GetNumberOfBytesUsed(), s3p4Addr, __FILE__, __LINE__ );
}
void NatTypeDetectionClient::Shutdown(void)
{
	serverAddress=UNASSIGNED_SYSTEM_ADDRESS;
	if (c2!=INVALID_SOCKET)
	{
		closesocket__(c2);
		c2=INVALID_SOCKET;
	}

}

#endif // _RAKNET_SUPPORT_*
