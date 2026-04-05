#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_NatTypeDetectionServer==1

#include "NatTypeDetectionServer.h"
#include "SocketLayer.h"
#include "RakNetSocket.h"
#include "RakNetSmartPtr.h"
#include "SocketIncludes.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "GetTime.h"
#include "BitStream.h"
#include "SocketDefines.h"

using namespace RakNet;

STATIC_FACTORY_DEFINITIONS(NatTypeDetectionServer,NatTypeDetectionServer);

NatTypeDetectionServer::NatTypeDetectionServer()
{
	s1p2=s2p3=s3p4=s4p5=INVALID_SOCKET;
}
NatTypeDetectionServer::~NatTypeDetectionServer()
{
	Shutdown();
}
void NatTypeDetectionServer::Startup(
									 const char *nonRakNetIP2,
									 const char *nonRakNetIP3,
									 const char *nonRakNetIP4)
{
	DataStructures::List<RakNetSmartPtr<RakNetSocket> > sockets;
	rakPeerInterface->GetSockets(sockets);
	char str[64];
	sockets[0]->boundAddress.ToString(false,str);
	s1p2=CreateNonblockingBoundSocket(str);
	s1p2Port=SocketLayer::GetLocalPort(s1p2);
	s2p3=CreateNonblockingBoundSocket(nonRakNetIP2);
	s2p3Port=SocketLayer::GetLocalPort(s2p3);
	s3p4=CreateNonblockingBoundSocket(nonRakNetIP3);
	s3p4Port=SocketLayer::GetLocalPort(s3p4);
	s4p5=CreateNonblockingBoundSocket(nonRakNetIP4);
	s4p5Port=SocketLayer::GetLocalPort(s4p5);
	strcpy(s3p4Address, nonRakNetIP3);
}
void NatTypeDetectionServer::Shutdown()
{
	if (s1p2!=INVALID_SOCKET)
	{
		closesocket__(s1p2);
		s1p2=INVALID_SOCKET;
	}
	if (s2p3!=INVALID_SOCKET)
	{
		closesocket__(s2p3);
		s2p3=INVALID_SOCKET;
	}
	if (s3p4!=INVALID_SOCKET)
	{
		closesocket__(s3p4);
		s3p4=INVALID_SOCKET;
	}
	if (s4p5!=INVALID_SOCKET)
	{
		closesocket__(s4p5);
		s4p5=INVALID_SOCKET;
	}
}
void NatTypeDetectionServer::Update(void)
{
	int i=0;
	RakNet::TimeMS time = RakNet::GetTimeMS();
	RakNet::BitStream bs;
	SystemAddress boundAddress;

	// Only socket that receives messages is s3p4, to see if the external address is different than that of the connection to rakPeerInterface
	char data[ MAXIMUM_MTU_SIZE ];
	int len;
	SystemAddress senderAddr;
	len=NatTypeRecvFrom(data, s3p4, senderAddr);
	// Client is asking us if this is port restricted. Only client requests of this type come in on s3p4
	while (len>0 && data[0]==NAT_TYPE_PORT_RESTRICTED)
	{
		RakNet::BitStream bsIn((unsigned char*) data,len,false);
		RakNetGUID senderGuid;
		bsIn.IgnoreBytes(sizeof(MessageID));
		bool readSuccess = bsIn.Read(senderGuid);
		RakAssert(readSuccess);
		if (readSuccess)
		{
			unsigned int i = GetDetectionAttemptIndex(senderGuid);
			if (i!=(unsigned int)-1)
			{
				bs.Reset();
				bs.Write((unsigned char) ID_NAT_TYPE_DETECTION_RESULT);
				// If different, then symmetric
				if (senderAddr!=natDetectionAttempts[i].systemAddress)
				{
					printf("Determined client is symmetric\n");
					bs.Write((unsigned char) NAT_TYPE_SYMMETRIC);
				}
				else
				{
					// else port restricted
					printf("Determined client is port restricted\n");
					bs.Write((unsigned char) NAT_TYPE_PORT_RESTRICTED);
				}

				rakPeerInterface->Send(&bs,HIGH_PRIORITY,RELIABLE,0,natDetectionAttempts[i].systemAddress,false);

				// Done
				natDetectionAttempts.RemoveAtIndexFast(i);
			}
			else
			{
		//		RakAssert("i==0 in Update when looking up GUID in NatTypeDetectionServer.cpp. Either a bug or a late resend" && 0);
			}
		}
		else
		{
		//	RakAssert("Didn't read GUID in Update in NatTypeDetectionServer.cpp. Message format error" && 0);
		}

		len=NatTypeRecvFrom(data, s3p4, senderAddr);
	}


	while (i < (int) natDetectionAttempts.Size())
	{
		if (time > natDetectionAttempts[i].nextStateTime)
		{
			natDetectionAttempts[i].detectionState=(NATDetectionState)((int)natDetectionAttempts[i].detectionState+1);
			natDetectionAttempts[i].nextStateTime=time+natDetectionAttempts[i].timeBetweenAttempts;
			SystemAddress saOut;
			unsigned char c;
			bs.Reset();
			switch (natDetectionAttempts[i].detectionState)
			{
			case STATE_TESTING_NONE_1:
			case STATE_TESTING_NONE_2:
				c = NAT_TYPE_NONE;
				printf("Testing NAT_TYPE_NONE\n");
				// S4P5 sends to C2. If arrived, no NAT. Done. (Else S4P5 potentially banned, do not use again).
				saOut=natDetectionAttempts[i].systemAddress;
				saOut.SetPort(natDetectionAttempts[i].c2Port);
				SocketLayer::SendTo_PC( s4p5, (const char*) &c, 1, saOut, __FILE__, __LINE__  );
				break;
			case STATE_TESTING_FULL_CONE_1:
			case STATE_TESTING_FULL_CONE_2:
				printf("Testing NAT_TYPE_FULL_CONE\n");
				rakPeerInterface->WriteOutOfBandHeader(&bs);
				bs.Write((unsigned char) ID_NAT_TYPE_DETECT);
				bs.Write((unsigned char) NAT_TYPE_FULL_CONE);
				// S2P3 sends to C1 (Different address, different port, to previously used port on client). If received, Full-cone nat. Done.  (Else S2P3 potentially banned, do not use again).
				saOut=natDetectionAttempts[i].systemAddress;
				saOut.SetPort(natDetectionAttempts[i].systemAddress.GetPort());
				SocketLayer::SendTo_PC( s2p3, (const char*) bs.GetData(), bs.GetNumberOfBytesUsed(), saOut, __FILE__, __LINE__  );
				break;
			case STATE_TESTING_ADDRESS_RESTRICTED_1:
			case STATE_TESTING_ADDRESS_RESTRICTED_2:
				printf("Testing NAT_TYPE_ADDRESS_RESTRICTED\n");
				rakPeerInterface->WriteOutOfBandHeader(&bs);
				bs.Write((unsigned char) ID_NAT_TYPE_DETECT);
				bs.Write((unsigned char) NAT_TYPE_ADDRESS_RESTRICTED);
				// S1P2 sends to C1 (Same address, different port, to previously used port on client). If received, address-restricted cone nat. Done.
				saOut=natDetectionAttempts[i].systemAddress;
				saOut.SetPort(natDetectionAttempts[i].systemAddress.GetPort());
				SocketLayer::SendTo_PC( s1p2, (const char*) bs.GetData(), bs.GetNumberOfBytesUsed(), saOut, __FILE__, __LINE__  );
				break;
			case STATE_TESTING_PORT_RESTRICTED_1:
			case STATE_TESTING_PORT_RESTRICTED_2:
				// C1 sends to S3P4. If address of C1 as seen by S3P4 is the same as the address of C1 as seen by S1P1, then port-restricted cone nat. Done
				printf("Testing NAT_TYPE_PORT_RESTRICTED\n");
				bs.Write((unsigned char) ID_NAT_TYPE_DETECTION_REQUEST);
				bs.Write(RakString::NonVariadic(s3p4Address));
				bs.Write(s3p4Port);
				rakPeerInterface->Send(&bs,HIGH_PRIORITY,RELIABLE,0,natDetectionAttempts[i].systemAddress,false);
				break;
			default:
				printf("Warning, exceeded final check STATE_TESTING_PORT_RESTRICTED_2.\nExpected that client would have sent NAT_TYPE_PORT_RESTRICTED on s3p4.\nDefaulting to Symmetric\n");
				bs.Write((unsigned char) ID_NAT_TYPE_DETECTION_RESULT);
				bs.Write((unsigned char) NAT_TYPE_SYMMETRIC);
				rakPeerInterface->Send(&bs,HIGH_PRIORITY,RELIABLE,0,natDetectionAttempts[i].systemAddress,false);
				natDetectionAttempts.RemoveAtIndexFast(i);
				i--;
				break;
			}

		}
		i++;
	}
}
PluginReceiveResult NatTypeDetectionServer::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_NAT_TYPE_DETECTION_REQUEST:
		OnDetectionRequest(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	return RR_CONTINUE_PROCESSING;
}
void NatTypeDetectionServer::OnClosedConnection(const SystemAddress &systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) rakNetGUID;

	unsigned int i = GetDetectionAttemptIndex(systemAddress);
	if (i==(unsigned int)-1)
		return;
	natDetectionAttempts.RemoveAtIndexFast(i);
}
void NatTypeDetectionServer::OnDetectionRequest(Packet *packet)
{
	unsigned int i = GetDetectionAttemptIndex(packet->systemAddress);

	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(1);
	bool isRequest=false;
	bsIn.Read(isRequest);
	if (isRequest)
	{
		if (i!=(unsigned int)-1)
			return; // Already in progress

		NATDetectionAttempt nda;
		nda.detectionState=STATE_NONE;
		nda.systemAddress=packet->systemAddress;
		nda.guid=packet->guid;
		bsIn.Read(nda.c2Port);
		nda.nextStateTime=0;
		nda.timeBetweenAttempts=rakPeerInterface->GetLastPing(nda.systemAddress)*3+50;
		natDetectionAttempts.Push(nda, _FILE_AND_LINE_);
	}
	else
	{
		if (i==(unsigned int)-1)
			return; // Unknown
		// They are done
		natDetectionAttempts.RemoveAtIndexFast(i);
	}

}
unsigned int NatTypeDetectionServer::GetDetectionAttemptIndex(const SystemAddress &sa)
{
	for (unsigned int i=0; i < natDetectionAttempts.Size(); i++)
	{
		if (natDetectionAttempts[i].systemAddress==sa)
			return i;
	}
	return (unsigned int) -1;
}
unsigned int NatTypeDetectionServer::GetDetectionAttemptIndex(RakNetGUID guid)
{
	for (unsigned int i=0; i < natDetectionAttempts.Size(); i++)
	{
		if (natDetectionAttempts[i].guid==guid)
			return i;
	}
	return (unsigned int) -1;
}

#endif // _RAKNET_SUPPORT_*
