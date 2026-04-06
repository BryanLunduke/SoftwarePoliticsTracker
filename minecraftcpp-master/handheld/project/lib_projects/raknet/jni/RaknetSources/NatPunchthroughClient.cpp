#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_NatPunchthroughClient==1

#include "NatPunchthroughClient.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "GetTime.h"
#include "PacketLogger.h"
#include "Itoa.h"

using namespace RakNet;

void NatPunchthroughDebugInterface_Printf::OnClientMessage(const char *msg)
{
	printf("%s\n", msg);
}
#if _RAKNET_SUPPORT_PacketLogger==1
void NatPunchthroughDebugInterface_PacketLogger::OnClientMessage(const char *msg)
{
	if (pl)
	{
		pl->WriteMiscellaneous("Nat", msg);
	}
}
#endif

STATIC_FACTORY_DEFINITIONS(NatPunchthroughClient,NatPunchthroughClient);

NatPunchthroughClient::NatPunchthroughClient()
{
	natPunchthroughDebugInterface=0;
	mostRecentNewExternalPort=0;
	sp.nextActionTime=0;
}
NatPunchthroughClient::~NatPunchthroughClient()
{
	rakPeerInterface=0;
	Clear();
}
bool NatPunchthroughClient::OpenNAT(RakNetGUID destination, const SystemAddress &facilitator)
{
	ConnectionState cs = rakPeerInterface->GetConnectionState(facilitator);
	if (cs!=IS_CONNECTED)
		return false;

	SendPunchthrough(destination, facilitator);
	return true;
}
bool NatPunchthroughClient::OpenNATGroup(DataStructures::List<RakNetGUID> destinationSystems, const SystemAddress &facilitator)
{
	ConnectionState cs = rakPeerInterface->GetConnectionState(facilitator);
	if (cs!=IS_CONNECTED)
		return false;

	unsigned long i;
	for (i=0; i < destinationSystems.Size(); i++)
	{
		SendPunchthrough(destinationSystems[i], facilitator);
	}

	GroupPunchRequest *gpr = RakNet::OP_NEW<GroupPunchRequest>(_FILE_AND_LINE_);
	gpr->facilitator=facilitator;
	gpr->pendingList=destinationSystems;
	groupPunchRequests.Push(gpr, _FILE_AND_LINE_);

	return true;
}
void NatPunchthroughClient::SetDebugInterface(NatPunchthroughDebugInterface *i)
{
	natPunchthroughDebugInterface=i;
}
void NatPunchthroughClient::Update(void)
{
	RakNet::Time time = RakNet::GetTime();
	if (sp.nextActionTime && sp.nextActionTime < time)
	{
		RakNet::Time delta = time - sp.nextActionTime;
		if (sp.testMode==SendPing::TESTING_INTERNAL_IPS)
		{
			SendOutOfBand(sp.internalIds[sp.attemptCount],ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			if (++sp.retryCount>=pc.UDP_SENDS_PER_PORT_INTERNAL)
			{
				++sp.attemptCount;
				sp.retryCount=0;
			}

			if (sp.attemptCount>=pc.MAXIMUM_NUMBER_OF_INTERNAL_IDS_TO_CHECK)
			{
				sp.testMode=SendPing::WAITING_FOR_INTERNAL_IPS_RESPONSE;
				if (pc.INTERNAL_IP_WAIT_AFTER_ATTEMPTS>0)
				{
					sp.nextActionTime=time+pc.INTERNAL_IP_WAIT_AFTER_ATTEMPTS-delta;
				}
				else
				{
					// TESTING: Try sending to unused ports on the remote system to reserve our own ports while not getting banned
					sp.testMode=SendPing::SEND_WITH_TTL;
					// sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FACILITATOR_PORT_TO_FACILITATOR_PORT;
					sp.attemptCount=0;
				}
			}
			else
			{
				sp.nextActionTime=time+pc.TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL-delta;
			}
		}
		else if (sp.testMode==SendPing::WAITING_FOR_INTERNAL_IPS_RESPONSE)
		{
			// TESTING: Try sending to unused ports on the remote system to reserve our own ports while not getting banned
			sp.testMode=SendPing::SEND_WITH_TTL;
			// sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FACILITATOR_PORT_TO_FACILITATOR_PORT;
			sp.attemptCount=0;
		}
		else if (sp.testMode==SendPing::SEND_WITH_TTL)
		{
			// See http://www.jenkinssoftware.com/forum/index.php?topic=4021.0
			// For Linux 2.6.32 soft-router (ip-tables)
			/*
			If I understand correctly, getting a datagram on a particular address that was not previously used causes that the port for that address to not be used for when a reply would have otherwise been sent back from that address.

			PHASE 1:
			1. System 1 and 2 send to each other.
			2. Due to latency, system 1 sends first using the server port.
			3. System 2 gets the datagram and no longer uses the server port. Instead, it replies using port 1024.
			4. System 1 gets the reply. The source port is wrong, so it is rejected.

			To put it another way, if a router gets a datagram on a port that was not previously used, it will not reply on that port. However, if it doesn't reply on that port, the message will not be accepted by the remote system.
			*/

			// Send to unused port. We do not want the message to arrive, just to open our router's table
			SystemAddress sa=sp.targetAddress;
			int ttlSendIndex;
			for (ttlSendIndex=0; ttlSendIndex <= pc.MAX_PREDICTIVE_PORT_RANGE; ttlSendIndex++)
			{
				sa.SetPort((unsigned short) (sp.targetAddress.GetPort()+ttlSendIndex));
				SendTTL(sa);
			}

			// Only do this stage once
			// Wait 250 milliseconds for next stage. The delay is so that even with timing errors both systems send out the
			// datagram with TTL before either sends a real one
			sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FACILITATOR_PORT_TO_FACILITATOR_PORT;
			sp.nextActionTime=time-delta+250;
		}
		else if (sp.testMode==SendPing::TESTING_EXTERNAL_IPS_FACILITATOR_PORT_TO_FACILITATOR_PORT)
		{
			SystemAddress sa=sp.targetAddress;
			sa.SetPort((unsigned short) (sa.GetPort()+sp.attemptCount));
			SendOutOfBand(sa,ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			IncrementExternalAttemptCount(time, delta);

			if (sp.attemptCount>pc.MAX_PREDICTIVE_PORT_RANGE)
			{
				sp.testMode=SendPing::WAITING_AFTER_ALL_ATTEMPTS;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS-delta;

				// Skip TESTING_EXTERNAL_IPS_1024_TO_FACILITATOR_PORT, etc.
				/*
				sp.testMode=SendPing::TESTING_EXTERNAL_IPS_1024_TO_FACILITATOR_PORT;
				sp.attemptCount=0;
				*/
			}
		}
		else if (sp.testMode==SendPing::TESTING_EXTERNAL_IPS_1024_TO_FACILITATOR_PORT)
		{
			SystemAddress sa=sp.targetAddress;
			if ( sp.targetGuid < rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS) )
				sa.SetPort((unsigned short) (1024+sp.attemptCount));
			else
				sa.SetPort((unsigned short) (sa.GetPort()+sp.attemptCount));
			SendOutOfBand(sa,ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			IncrementExternalAttemptCount(time, delta);

			if (sp.attemptCount>pc.MAX_PREDICTIVE_PORT_RANGE)
			{
				// From 1024 disabled, never helps as I've seen, but slows down the process by half
				sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FACILITATOR_PORT_TO_1024;
				sp.attemptCount=0;
			}

		}
		else if (sp.testMode==SendPing::TESTING_EXTERNAL_IPS_FACILITATOR_PORT_TO_1024)
		{
			SystemAddress sa=sp.targetAddress;
			if ( sp.targetGuid > rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS) )
				sa.SetPort((unsigned short) (1024+sp.attemptCount));
			else
				sa.SetPort((unsigned short) (sa.GetPort()+sp.attemptCount));
			SendOutOfBand(sa,ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			IncrementExternalAttemptCount(time, delta);

			if (sp.attemptCount>pc.MAX_PREDICTIVE_PORT_RANGE)
			{
				// From 1024 disabled, never helps as I've seen, but slows down the process by half
				sp.testMode=SendPing::TESTING_EXTERNAL_IPS_1024_TO_1024;
				sp.attemptCount=0;
			}
		}
		else if (sp.testMode==SendPing::TESTING_EXTERNAL_IPS_1024_TO_1024)
		{
			SystemAddress sa=sp.targetAddress;
			sa.SetPort((unsigned short) (1024+sp.attemptCount));
			SendOutOfBand(sa,ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			IncrementExternalAttemptCount(time, delta);

			if (sp.attemptCount>pc.MAX_PREDICTIVE_PORT_RANGE)
			{
				if (natPunchthroughDebugInterface)
				{
					char ipAddressString[32];
					sp.targetAddress.ToString(true, ipAddressString);
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Likely bidirectional punchthrough failure to guid %s, system address %s.", guidString, ipAddressString));
				}

				sp.testMode=SendPing::WAITING_AFTER_ALL_ATTEMPTS;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS-delta;
			}
		}
		else if (sp.testMode==SendPing::WAITING_AFTER_ALL_ATTEMPTS)
		{
			// Failed. Tell the user
			OnPunchthroughFailure();
			UpdateGroupPunchOnNatResult(sp.facilitator, sp.targetGuid, sp.targetAddress, 1);
		}

		if (sp.testMode==SendPing::PUNCHING_FIXED_PORT)
		{
			SendOutOfBand(sp.targetAddress,ID_NAT_ESTABLISH_BIDIRECTIONAL);
			if (++sp.retryCount>=sp.punchingFixedPortAttempts)
			{
				if (natPunchthroughDebugInterface)
				{
					char ipAddressString[32];
					sp.targetAddress.ToString(true, ipAddressString);
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Likely unidirectional punchthrough failure to guid %s, system address %s.", guidString, ipAddressString));
				}

				sp.testMode=SendPing::WAITING_AFTER_ALL_ATTEMPTS;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS-delta;
			}
			else
			{
				if ((sp.retryCount%pc.UDP_SENDS_PER_PORT_EXTERNAL)==0)
					sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_BETWEEN_PORTS-delta;
				else
					sp.nextActionTime=time+pc.TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL-delta;
			}
		}
	}

	// Remove elapsed groupRequestsInProgress
	unsigned int i;
	i=0;
	while (i < groupRequestsInProgress.Size())
	{
		if (time > groupRequestsInProgress[i].time)
			groupRequestsInProgress.RemoveAtIndexFast(i);
		else
			i++;
	}
}
void NatPunchthroughClient::PushFailure(void)
{
	Packet *p = AllocatePacketUnified(sizeof(MessageID)+sizeof(unsigned char));
	p->data[0]=ID_NAT_PUNCHTHROUGH_FAILED;
	p->systemAddress=sp.targetAddress;
	p->systemAddress.systemIndex=(SystemIndex)-1;
	p->guid=sp.targetGuid;
	if (sp.weAreSender)
		p->data[1]=1;
	else
		p->data[1]=0;
	p->wasGeneratedLocally=true;
	rakPeerInterface->PushBackPacket(p, true);
}
void NatPunchthroughClient::OnPunchthroughFailure(void)
{
	if (pc.retryOnFailure==false)
	{
		if (natPunchthroughDebugInterface)
		{
			char ipAddressString[32];
			sp.targetAddress.ToString(true, ipAddressString);
			char guidString[128];
			sp.targetGuid.ToString(guidString);
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Failed punchthrough once. Returning failure to guid %s, system address %s to user.", guidString, ipAddressString));
		}

		PushFailure();
		OnReadyForNextPunchthrough();
		return;
	}

	unsigned int i;
	for (i=0; i < failedAttemptList.Size(); i++)
	{
		if (failedAttemptList[i].guid==sp.targetGuid)
		{
			if (natPunchthroughDebugInterface)
			{
				char ipAddressString[32];
				sp.targetAddress.ToString(true, ipAddressString);
				char guidString[128];
				sp.targetGuid.ToString(guidString);
				natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Failed punchthrough twice. Returning failure to guid %s, system address %s to user.", guidString, ipAddressString));
			}

			// Failed a second time, so return failed to user
			PushFailure();

			OnReadyForNextPunchthrough();

			failedAttemptList.RemoveAtIndexFast(i);
			return;
		}
	}

	if (rakPeerInterface->GetConnectionState(sp.facilitator)!=IS_CONNECTED)
	{
		if (natPunchthroughDebugInterface)
		{
			char ipAddressString[32];
			sp.targetAddress.ToString(true, ipAddressString);
			char guidString[128];
			sp.targetGuid.ToString(guidString);
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Not connected to facilitator, so cannot retry punchthrough after first failure. Returning failure onj guid %s, system address %s to user.", guidString, ipAddressString));
		}

		// Failed, and can't try again because no facilitator
		PushFailure();
		return;
	}

	if (natPunchthroughDebugInterface)
	{
		char ipAddressString[32];
		sp.targetAddress.ToString(true, ipAddressString);
		char guidString[128];
		sp.targetGuid.ToString(guidString);
		natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("First punchthrough failure on guid %s, system address %s. Reattempting.", guidString, ipAddressString));
	}

	// Failed the first time. Add to the failure queue and try again
	AddrAndGuid aag;
	aag.addr=sp.targetAddress;
	aag.guid=sp.targetGuid;
	failedAttemptList.Push(aag, _FILE_AND_LINE_);

	// Tell the server we are ready
	OnReadyForNextPunchthrough();

	// If we are the sender, try again, immediately if possible, else added to the queue on the faciltiator
	if (sp.weAreSender)
		SendPunchthrough(sp.targetGuid, sp.facilitator);
}
PluginReceiveResult NatPunchthroughClient::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_NAT_GET_MOST_RECENT_PORT:
		{
			OnGetMostRecentPort(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
	case ID_NAT_PUNCHTHROUGH_FAILED:
	case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
		if (packet->wasGeneratedLocally==false)
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		break;
	case ID_OUT_OF_BAND_INTERNAL:
		if (packet->length>=2 &&
			(packet->data[1]==ID_NAT_ESTABLISH_UNIDIRECTIONAL || packet->data[1]==ID_NAT_ESTABLISH_BIDIRECTIONAL) &&
			sp.nextActionTime!=0)
		{
			RakNet::BitStream bs(packet->data,packet->length,false);
			bs.IgnoreBytes(2);
			uint16_t sessionId;
			bs.Read(sessionId);
//			RakAssert(sessionId<100);
			if (sessionId!=sp.sessionId)
				break;

			char ipAddressString[32];
			packet->systemAddress.ToString(true,ipAddressString);
			// sp.targetGuid==packet->guid is because the internal IP addresses reported may include loopbacks not reported by RakPeer::IsLocalIP()
			if (packet->data[1]==ID_NAT_ESTABLISH_UNIDIRECTIONAL && sp.targetGuid==packet->guid)
			{
				if (natPunchthroughDebugInterface)
				{
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Received ID_NAT_ESTABLISH_UNIDIRECTIONAL from guid %s, system address %s.", guidString, ipAddressString));
				}
				if (sp.testMode!=SendPing::PUNCHING_FIXED_PORT)
				{
					sp.testMode=SendPing::PUNCHING_FIXED_PORT;
					sp.retryCount+=sp.attemptCount*pc.UDP_SENDS_PER_PORT_EXTERNAL;
					sp.targetAddress=packet->systemAddress;
					// Keeps trying until the other side gives up too, in case it is unidirectional
					sp.punchingFixedPortAttempts=pc.UDP_SENDS_PER_PORT_EXTERNAL*(pc.MAX_PREDICTIVE_PORT_RANGE+1);
				}

				SendOutOfBand(sp.targetAddress,ID_NAT_ESTABLISH_BIDIRECTIONAL);
			}
			else if (packet->data[1]==ID_NAT_ESTABLISH_BIDIRECTIONAL &&
				sp.targetGuid==packet->guid)
			{
				// They send back our port
				bs.Read(mostRecentNewExternalPort);

				SendOutOfBand(packet->systemAddress,ID_NAT_ESTABLISH_BIDIRECTIONAL);

				// Tell the user about the success
				sp.targetAddress=packet->systemAddress;
				PushSuccess();
				UpdateGroupPunchOnNatResult(sp.facilitator, sp.targetGuid, sp.targetAddress, 1);
				OnReadyForNextPunchthrough();
				bool removedFromFailureQueue=RemoveFromFailureQueue();

				if (natPunchthroughDebugInterface)
				{
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					if (removedFromFailureQueue)
						natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough to guid %s, system address %s succeeded on 2nd attempt.", guidString, ipAddressString));
					else
						natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough to guid %s, system address %s succeeded on 1st attempt.", guidString, ipAddressString));
				}
			}

	//		mostRecentNewExternalPort=packet->systemAddress.GetPort();
		}
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_NAT_ALREADY_IN_PROGRESS:
		{
			RakNet::BitStream incomingBs(packet->data, packet->length, false);
			incomingBs.IgnoreBytes(sizeof(MessageID));
			RakNetGUID targetGuid;
			incomingBs.Read(targetGuid);
			// Don't update group, just use later message
			// UpdateGroupPunchOnNatResult(packet->systemAddress, targetGuid, UNASSIGNED_SYSTEM_ADDRESS, 2);
			if (natPunchthroughDebugInterface)
			{
				char guidString[128];
				targetGuid.ToString(guidString);
				natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough retry to guid %s failed due to ID_NAT_ALREADY_IN_PROGRESS. Returning failure.", guidString));
			}

		}
		break;
	case ID_NAT_TARGET_NOT_CONNECTED:
	case ID_NAT_CONNECTION_TO_TARGET_LOST:
	case ID_NAT_TARGET_UNRESPONSIVE:
		{
			const char *reason;
			if (packet->data[0]==ID_NAT_TARGET_NOT_CONNECTED)
				reason=(char *)"ID_NAT_TARGET_NOT_CONNECTED";
			else if (packet->data[0]==ID_NAT_CONNECTION_TO_TARGET_LOST)
				reason=(char *)"ID_NAT_CONNECTION_TO_TARGET_LOST";
			else
				reason=(char *)"ID_NAT_TARGET_UNRESPONSIVE";


			RakNet::BitStream incomingBs(packet->data, packet->length, false);
			incomingBs.IgnoreBytes(sizeof(MessageID));

			RakNetGUID targetGuid;
			incomingBs.Read(targetGuid);
			UpdateGroupPunchOnNatResult(packet->systemAddress, targetGuid, UNASSIGNED_SYSTEM_ADDRESS, 2);

			if (packet->data[0]==ID_NAT_CONNECTION_TO_TARGET_LOST ||
				packet->data[0]==ID_NAT_TARGET_UNRESPONSIVE)
			{
				uint16_t sessionId;
				incomingBs.Read(sessionId);
				if (sessionId!=sp.sessionId)
					break;
			}

			unsigned int i;
			for (i=0; i < failedAttemptList.Size(); i++)
			{
				if (failedAttemptList[i].guid==targetGuid)
				{
					if (natPunchthroughDebugInterface)
					{
						char guidString[128];
						targetGuid.ToString(guidString);
						natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough retry to guid %s failed due to %s.", guidString, reason));

					}

					// If the retry target is not connected, or loses connection, or is not responsive, then previous failures cannot be retried.

					// Don't need to return failed, the other messages indicate failure anyway
					/*
					Packet *p = AllocatePacketUnified(sizeof(MessageID));
					p->data[0]=ID_NAT_PUNCHTHROUGH_FAILED;
					p->systemAddress=failedAttemptList[i].addr;
					p->systemAddress.systemIndex=(SystemIndex)-1;
					p->guid=failedAttemptList[i].guid;
					rakPeerInterface->PushBackPacket(p, false);
					*/

					failedAttemptList.RemoveAtIndexFast(i);
					break;
				}
			}

			if (natPunchthroughDebugInterface)
			{
				char guidString[128];
				targetGuid.ToString(guidString);
				natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough attempt to guid %s failed due to %s.", guidString, reason));
			}

			// Stop trying punchthrough
			sp.nextActionTime=0;

			/*
			RakNet::BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBytes(sizeof(MessageID));
			RakNetGUID failedSystem;
			bs.Read(failedSystem);
			bool deletedFirst=false;
			unsigned int i=0;
			while (i < pendingOpenNAT.Size())
			{
				if (pendingOpenNAT[i].destination==failedSystem)
				{
					if (i==0)
						deletedFirst=true;
					pendingOpenNAT.RemoveAtIndex(i);
				}
				else
					i++;
			}
			// Failed while in progress. Go to next in attempt queue
			if (deletedFirst && pendingOpenNAT.Size())
			{
				SendPunchthrough(pendingOpenNAT[0].destination, pendingOpenNAT[0].facilitator);
				sp.nextActionTime=0;
			}
			*/
		}
		break;
	case ID_TIMESTAMP:
		if (packet->data[sizeof(MessageID)+sizeof(RakNet::Time)]==ID_NAT_CONNECT_AT_TIME)
		{
			OnConnectAtTime(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
		break;
	}
	return RR_CONTINUE_PROCESSING;
}
/*
void NatPunchthroughClient::ProcessNextPunchthroughQueue(void)
{
	// Go to the next attempt
	if (pendingOpenNAT.Size())
		pendingOpenNAT.RemoveAtIndex(0);

	// Do next punchthrough attempt
	if (pendingOpenNAT.Size())
		SendPunchthrough(pendingOpenNAT[0].destination, pendingOpenNAT[0].facilitator);

	sp.nextActionTime=0;
}
*/
void NatPunchthroughClient::OnConnectAtTime(Packet *packet)
{
//	RakAssert(sp.nextActionTime==0);

	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(MessageID));
	bs.Read(sp.nextActionTime);
	bs.IgnoreBytes(sizeof(MessageID));
	bs.Read(sp.sessionId);
	bs.Read(sp.targetAddress);
	int j;
//	int k;
//	k=0;
	for (j=0; j < MAXIMUM_NUMBER_OF_INTERNAL_IDS; j++)
		bs.Read(sp.internalIds[j]);

	// Prevents local testing
	/*
	for (j=0; j < MAXIMUM_NUMBER_OF_INTERNAL_IDS; j++)
	{
		SystemAddress id;
		bs.Read(id);
		char str[32];
		id.ToString(false,str);
		if (rakPeerInterface->IsLocalIP(str)==false)
			sp.internalIds[k++]=id;
	}
	*/
	sp.attemptCount=0;
	sp.retryCount=0;
	if (pc.MAXIMUM_NUMBER_OF_INTERNAL_IDS_TO_CHECK>0)
	{
		sp.testMode=SendPing::TESTING_INTERNAL_IPS;
	}
	else
	{
		// TESTING: Try sending to unused ports on the remote system to reserve our own ports while not getting banned
		sp.testMode=SendPing::SEND_WITH_TTL;
		// sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FACILITATOR_PORT_TO_FACILITATOR_PORT;
	}
	bs.Read(sp.targetGuid);
	bs.Read(sp.weAreSender);
}
void NatPunchthroughClient::SendTTL(const SystemAddress &sa)
{
	if (sa==UNASSIGNED_SYSTEM_ADDRESS)
		return;
	if (sa.GetPort()==0)
		return;

	char ipAddressString[32];
	sa.ToString(false, ipAddressString);
	// TTL of 1 doesn't get past the router, 2 might hit the other system on a LAN
	rakPeerInterface->SendTTL(ipAddressString,sa.GetPort(), 2);
}
void NatPunchthroughClient::SendOutOfBand(SystemAddress sa, MessageID oobId)
{
	if (sa==UNASSIGNED_SYSTEM_ADDRESS)
		return;
	if (sa.GetPort()==0)
		return;

	RakNet::BitStream oob;
	oob.Write(oobId);
	oob.Write(sp.sessionId);
//	RakAssert(sp.sessionId<100);
	if (oobId==ID_NAT_ESTABLISH_BIDIRECTIONAL)
		oob.Write(sa.GetPort());
	char ipAddressString[32];
	sa.ToString(false, ipAddressString);
	rakPeerInterface->SendOutOfBand((const char*) ipAddressString,sa.GetPort(),(const char*) oob.GetData(),oob.GetNumberOfBytesUsed());

	if (natPunchthroughDebugInterface)
	{
		sa.ToString(true,ipAddressString);
		char guidString[128];
		sp.targetGuid.ToString(guidString);

		if (oobId==ID_NAT_ESTABLISH_UNIDIRECTIONAL)
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Sent OOB ID_NAT_ESTABLISH_UNIDIRECTIONAL to guid %s, system address %s.", guidString, ipAddressString));
		else
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Sent OOB ID_NAT_ESTABLISH_BIDIRECTIONAL to guid %s, system address %s.", guidString, ipAddressString));
	}
}
void NatPunchthroughClient::OnNewConnection(const SystemAddress &systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) rakNetGUID;
	(void) isIncoming;

	// Try to track new port mappings on the router. Not reliable, but better than nothing.
	SystemAddress ourExternalId = rakPeerInterface->GetExternalID(systemAddress);
	if (ourExternalId!=UNASSIGNED_SYSTEM_ADDRESS)
		mostRecentNewExternalPort=ourExternalId.GetPort();

	unsigned int i;
	i=0;
	while (i < groupRequestsInProgress.Size())
	{
		if (groupRequestsInProgress[i].guid==rakNetGUID)
		{
			groupRequestsInProgress.RemoveAtIndexFast(i);
		}
		else
		{
			i++;
		}
	}
}

void NatPunchthroughClient::OnClosedConnection(const SystemAddress &systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) systemAddress;
	(void) rakNetGUID;
	(void) lostConnectionReason;

	if (sp.facilitator==systemAddress)
	{
		// If we lose the connection to the facilitator, all previous failures not currently in progress are returned as such
		unsigned int i=0;
		while (i < failedAttemptList.Size())
		{
			if (sp.nextActionTime!=0 && sp.targetGuid==failedAttemptList[i].guid)
			{
				i++;
				continue;
			}

			PushFailure();

			failedAttemptList.RemoveAtIndexFast(i);
		}
	}

	unsigned int i;
	i=0;
	while (i < groupPunchRequests.Size())
	{
		if (groupPunchRequests[i]->facilitator==systemAddress)
		{
			RakNet::OP_DELETE(groupPunchRequests[i],_FILE_AND_LINE_);
			groupPunchRequests.RemoveAtIndexFast(i);
		}
		else
		{
			i++;
		}
	}

}
void NatPunchthroughClient::GetUPNPPortMappings(char *externalPort, char *internalPort, const SystemAddress &natPunchthroughServerAddress)
{
	DataStructures::List<RakNet::RakNetSmartPtr<RakNet::RakNetSocket> > sockets;
	rakPeerInterface->GetSockets(sockets);
	Itoa(sockets[0]->boundAddress.GetPort(),internalPort,10);
	if (mostRecentNewExternalPort==0)
		mostRecentNewExternalPort=rakPeerInterface->GetExternalID(natPunchthroughServerAddress).GetPort();
	Itoa(mostRecentNewExternalPort,externalPort,10);
}
void NatPunchthroughClient::OnFailureNotification(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data,packet->length,false);
	incomingBs.IgnoreBytes(sizeof(MessageID));
	RakNetGUID senderGuid;
	incomingBs.Read(senderGuid);

	unsigned int i;
	i=0;
	while (i < groupRequestsInProgress.Size())
	{
		if (groupRequestsInProgress[i].guid==senderGuid)
		{
			groupRequestsInProgress.RemoveAtIndexFast(i);
			break;
		}
		else
		{
			i++;
		}
	}
}
void NatPunchthroughClient::OnGetMostRecentPort(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data,packet->length,false);
	incomingBs.IgnoreBytes(sizeof(MessageID));
	uint16_t sessionId;
	incomingBs.Read(sessionId);

	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_NAT_GET_MOST_RECENT_PORT);
	outgoingBs.Write(sessionId);
	if (mostRecentNewExternalPort==0)
		mostRecentNewExternalPort=rakPeerInterface->GetExternalID(packet->systemAddress).GetPort();
	RakAssert(mostRecentNewExternalPort!=0);
	outgoingBs.Write(mostRecentNewExternalPort);
	rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
	sp.facilitator=packet->systemAddress;
}
/*
unsigned int NatPunchthroughClient::GetPendingOpenNATIndex(RakNetGUID destination, const SystemAddress &facilitator)
{
	unsigned int i;
	for (i=0; i < pendingOpenNAT.Size(); i++)
	{
		if (pendingOpenNAT[i].destination==destination && pendingOpenNAT[i].facilitator==facilitator)
			return i;
	}
	return (unsigned int) -1;
}
*/
void NatPunchthroughClient::SendPunchthrough(RakNetGUID destination, const SystemAddress &facilitator)
{
	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_NAT_PUNCHTHROUGH_REQUEST);
	outgoingBs.Write(destination);
	rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,facilitator,false);

//	RakAssert(rakPeerInterface->GetSystemAddressFromGuid(destination)==UNASSIGNED_SYSTEM_ADDRESS);

	if (natPunchthroughDebugInterface)
	{
		char guidString[128];
		destination.ToString(guidString);
		natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Starting ID_NAT_PUNCHTHROUGH_REQUEST to guid %s.", guidString));
	}
}
void NatPunchthroughClient::OnAttach(void)
{
	Clear();
}
void NatPunchthroughClient::OnDetach(void)
{
	Clear();
}
void NatPunchthroughClient::OnRakPeerShutdown(void)
{
	Clear();
}
void NatPunchthroughClient::Clear(void)
{
	OnReadyForNextPunchthrough();

	failedAttemptList.Clear(false, _FILE_AND_LINE_);
	groupRequestsInProgress.Clear(false, _FILE_AND_LINE_);
	unsigned int i;
	for (i=0; i < groupPunchRequests.Size(); i++)
	{
		RakNet::OP_DELETE(groupPunchRequests[i],_FILE_AND_LINE_);
	}
	groupPunchRequests.Clear(true, _FILE_AND_LINE_);
}
PunchthroughConfiguration* NatPunchthroughClient::GetPunchthroughConfiguration(void)
{
	return &pc;
}
void NatPunchthroughClient::OnReadyForNextPunchthrough(void)
{
	if (rakPeerInterface==0)
		return;

	sp.nextActionTime=0;

	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_NAT_CLIENT_READY);
	rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,sp.facilitator,false);
}

void NatPunchthroughClient::PushSuccess(void)
{
	Packet *p = AllocatePacketUnified(sizeof(MessageID)+sizeof(unsigned char));
	p->data[0]=ID_NAT_PUNCHTHROUGH_SUCCEEDED;
	p->systemAddress=sp.targetAddress;
	p->systemAddress.systemIndex=(SystemIndex)-1;
	p->guid=sp.targetGuid;
	if (sp.weAreSender)
		p->data[1]=1;
	else
		p->data[1]=0;
	p->wasGeneratedLocally=true;
	rakPeerInterface->PushBackPacket(p, true);
}
bool NatPunchthroughClient::RemoveFromFailureQueue(void)
{
	unsigned int i;
	for (i=0; i < failedAttemptList.Size(); i++)
	{
		if (failedAttemptList[i].guid==sp.targetGuid)
		{
			// Remove from failure queue
			failedAttemptList.RemoveAtIndexFast(i);
			return true;
		}
	}
	return false;
}

void NatPunchthroughClient::IncrementExternalAttemptCount(RakNet::Time time, RakNet::Time delta)
{
	if (++sp.retryCount>=pc.UDP_SENDS_PER_PORT_EXTERNAL)
	{
		++sp.attemptCount;
		sp.retryCount=0;
		sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_BETWEEN_PORTS-delta;
	}
	else
	{
		sp.nextActionTime=time+pc.TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL-delta;
	}
}
// 0=failed, 1=success, 2=ignore
void NatPunchthroughClient::UpdateGroupPunchOnNatResult(SystemAddress facilitator, RakNetGUID targetSystem, SystemAddress targetSystemAddress, int result)
{
	GroupPunchRequest *gpr;
	unsigned long i,j,k;
	i=0;
	while (i < groupPunchRequests.Size())
	{
		gpr = groupPunchRequests[i];
		if (gpr->facilitator==facilitator)
		{
			j=0;
			while (j < gpr->pendingList.Size())
			{
				if (gpr->pendingList[j]==targetSystem)
				{
					if (result==0)
					{
						gpr->failedList.Push(targetSystem, _FILE_AND_LINE_);
					}
					else if (result==1)
					{
						gpr->passedListGuid.Push(targetSystem, _FILE_AND_LINE_);
						gpr->passedListAddress.Push(targetSystemAddress, _FILE_AND_LINE_);
					}
					else
					{
						gpr->ignoredList.Push(targetSystem, _FILE_AND_LINE_);
					}
					gpr->pendingList.RemoveAtIndex(j);
				}
				else
					j++;
			}
		}
		if (gpr->pendingList.Size()==0)
		{
			RakNet::BitStream output;
			if (gpr->failedList.Size()==0)
			{
				output.Write(ID_NAT_GROUP_PUNCH_SUCCEEDED);
			}
			else
			{
				output.Write(ID_NAT_GROUP_PUNCH_FAILED);
			}

			output.WriteCasted<unsigned char>(gpr->passedListGuid.Size());
			for (k=0; k < gpr->passedListGuid.Size(); k++)
			{
				output.Write(gpr->passedListGuid[k]);
				output.Write(gpr->passedListAddress[k]);
			}
			output.WriteCasted<unsigned char>(gpr->ignoredList.Size());
			for (k=0; k < gpr->ignoredList.Size(); k++)
			{
				output.Write(gpr->ignoredList[k]);
			}
			output.WriteCasted<unsigned char>(gpr->failedList.Size());
			for (k=0; k < gpr->failedList.Size(); k++)
			{
				output.Write(gpr->failedList[k]);
			}			

			Packet *p = AllocatePacketUnified(output.GetNumberOfBytesUsed());
			p->systemAddress=gpr->facilitator;
			p->systemAddress.systemIndex=(SystemIndex)-1;
			p->guid=rakPeerInterface->GetGuidFromSystemAddress(gpr->facilitator);
			p->wasGeneratedLocally=true;
			memcpy(p->data, output.GetData(), output.GetNumberOfBytesUsed());
			rakPeerInterface->PushBackPacket(p, true);

			groupPunchRequests.RemoveAtIndex(i);
			RakNet::OP_DELETE(gpr, _FILE_AND_LINE_);
		}
		else
			i++;
	}
}

#endif // _RAKNET_SUPPORT_*

