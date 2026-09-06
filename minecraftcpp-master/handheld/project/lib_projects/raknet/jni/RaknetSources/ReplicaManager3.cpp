#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_ReplicaManager3==1

#include "ReplicaManager3.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "NetworkIDManager.h"

using namespace RakNet;

// DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS(LastSerializationResult,Replica3*,replica);

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PRO::operator==( const PRO& right ) const
{
	return priority == right.priority && reliability == right.reliability && orderingChannel == right.orderingChannel && sendReceipt == right.sendReceipt;
}

bool PRO::operator!=( const PRO& right ) const
{
	return priority != right.priority || reliability != right.reliability || orderingChannel != right.orderingChannel || sendReceipt != right.sendReceipt;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int Connection_RM3::Replica3LSRComp( Replica3 * const &replica3, LastSerializationResult * const &data )
{
	if (replica3->GetNetworkID() < data->replica->GetNetworkID())
		return -1;
	if (replica3->GetNetworkID() > data->replica->GetNetworkID())
		return 1;
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

LastSerializationResult::LastSerializationResult()
{
	replica=0;
	lastSerializationResultBS=0;
}
LastSerializationResult::~LastSerializationResult()
{
	if (lastSerializationResultBS)
		RakNet::OP_DELETE(lastSerializationResultBS,_FILE_AND_LINE_);
}
void LastSerializationResult::AllocBS(void)
{
	if (lastSerializationResultBS==0)
	{
		lastSerializationResultBS=RakNet::OP_NEW<LastSerializationResultBS>(_FILE_AND_LINE_);
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ReplicaManager3::ReplicaManager3()
{
	defaultSendParameters.orderingChannel=0;
	defaultSendParameters.priority=HIGH_PRIORITY;
	defaultSendParameters.reliability=RELIABLE_ORDERED;
	defaultSendParameters.sendReceipt=0;
	autoSerializeInterval=30;
	lastAutoSerializeOccurance=0;
	worldId=0;
	autoCreateConnections=true;
	autoDestroyConnections=true;
	networkIDManager=0;
	currentlyDeallocatingReplica=0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ReplicaManager3::~ReplicaManager3()
{
	if (autoDestroyConnections)
	{
		// Clear() calls DeallocConnection(), which is pure virtual and cannot be called from the destructor
		RakAssert(connectionList.Size()==0);
	}
	Clear();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetAutoManageConnections(bool autoCreate, bool autoDestroy)
{
	autoCreateConnections=autoCreate;
	autoDestroyConnections=autoDestroy;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool ReplicaManager3::PushConnection(RakNet::Connection_RM3 *newConnection)
{
	if (newConnection==0)
		return false;
	if (GetConnectionByGUID(newConnection->GetRakNetGUID()))
		return false;
	unsigned int index = connectionList.GetIndexOf(newConnection);
	if (index==(unsigned int)-1)
	{
		connectionList.Push(newConnection,_FILE_AND_LINE_);

		// Send message to validate the connection
		newConnection->SendValidation(rakPeerInterface, worldId);

		Connection_RM3::ConstructionMode constructionMode = newConnection->QueryConstructionMode();
		if (constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
		{
			unsigned int pushIdx;
			for (pushIdx=0; pushIdx < userReplicaList.Size(); pushIdx++)
				newConnection->OnLocalReference(userReplicaList[pushIdx], this);
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::DeallocReplicaNoBroadcastDestruction(RakNet::Connection_RM3 *connection, RakNet::Replica3 *replica3)
{
	currentlyDeallocatingReplica=replica3;
	replica3->DeallocReplica(connection);
	currentlyDeallocatingReplica=0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RakNet::Connection_RM3 * ReplicaManager3::PopConnection(unsigned int index)
{
	DataStructures::List<Replica3*> replicaList;
	DataStructures::List<Replica3*> destructionList;
	DataStructures::List<Replica3*> broadcastList;
	RakNet::Connection_RM3 *connection;
	unsigned int index2;
	RM3ActionOnPopConnection action;

	connection=connectionList[index];

	// Clear out downloadGroup
	connection->ClearDownloadGroup(rakPeerInterface);

	RakNetGUID guid = connection->GetRakNetGUID();
	// This might be wrong, I am relying on the variable creatingSystemGuid which is transmitted
	// automatically from the first system to reference the object. However, if an object changes
	// owners then it is not going to be returned here, and therefore QueryActionOnPopConnection()
	// will not be called for the new owner.
	GetReplicasCreatedByGuid(guid, replicaList);

	for (index2=0; index2 < replicaList.Size(); index2++)
	{
		action = replicaList[index2]->QueryActionOnPopConnection(connection);
		replicaList[index2]->OnPoppedConnection(connection);
		if (action==RM3AOPC_DELETE_REPLICA)
		{
			destructionList.Push( replicaList[index2], _FILE_AND_LINE_  );
		}
		else if (action==RM3AOPC_DELETE_REPLICA_AND_BROADCAST_DESTRUCTION)
		{
			destructionList.Push( replicaList[index2], _FILE_AND_LINE_  );

			broadcastList.Push( replicaList[index2], _FILE_AND_LINE_  );
		}
	}

	BroadcastDestructionList(broadcastList, connection->GetSystemAddress());
	for (index2=0; index2 < destructionList.Size(); index2++)
	{
		destructionList[index2]->PreDestruction(connection);
		destructionList[index2]->DeallocReplica(connection);
	}

	connectionList.RemoveAtIndex(index);
	return connection;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RakNet::Connection_RM3 * ReplicaManager3::PopConnection(RakNetGUID guid)
{
	unsigned int index;

	for (index=0; index < connectionList.Size(); index++)
	{
		if (connectionList[index]->GetRakNetGUID()==guid)
		{
			return PopConnection(index);
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::Reference(RakNet::Replica3 *replica3)
{
	unsigned int index = ReferenceInternal(replica3);

	if (index!=(unsigned int)-1)
	{
		unsigned int pushIdx;
		for (pushIdx=0; pushIdx < connectionList.Size(); pushIdx++)
		{
			Connection_RM3::ConstructionMode constructionMode = connectionList[pushIdx]->QueryConstructionMode();
			if (constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
			{
				connectionList[pushIdx]->OnLocalReference(replica3, this);
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unsigned int ReplicaManager3::ReferenceInternal(RakNet::Replica3 *replica3)
{
	unsigned int index;
	index = userReplicaList.GetIndexOf(replica3);
	if (index==(unsigned int)-1)
	{
		RakAssert(networkIDManager);
		replica3->SetNetworkIDManager(networkIDManager);
		if (replica3->creatingSystemGUID==UNASSIGNED_RAKNET_GUID)
			replica3->creatingSystemGUID=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
		replica3->replicaManager=this;
		userReplicaList.Push(replica3,_FILE_AND_LINE_);
		index=userReplicaList.Size()-1;
	}
	return index;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::Dereference(RakNet::Replica3 *replica3)
{
	unsigned int index, index2;
	for (index=0; index < userReplicaList.Size(); index++)
	{
		if (userReplicaList[index]==replica3)
		{
			userReplicaList.RemoveAtIndex(index);
			break;
		}
	}

	// Remove from all connections
	for (index2=0; index2 < connectionList.Size(); index2++)
	{
		connectionList[index2]->OnDereference(replica3, this);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::DereferenceList(DataStructures::List<Replica3*> &replicaListIn)
{
	unsigned int index;
	for (index=0; index < replicaListIn.Size(); index++)
		Dereference(replicaListIn[index]);
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetReplicasCreatedByMe(DataStructures::List<Replica3*> &replicaListOut)
{
	RakNetGUID myGuid = rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	GetReplicasCreatedByGuid(rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS), replicaListOut);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetReferencedReplicaList(DataStructures::List<Replica3*> &replicaListOut)
{
	replicaListOut=userReplicaList;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetReplicasCreatedByGuid(RakNetGUID guid, DataStructures::List<Replica3*> &replicaListOut)
{
	replicaListOut.Clear(false,_FILE_AND_LINE_);
	unsigned int index;
	for (index=0; index < userReplicaList.Size(); index++)
	{
		if (userReplicaList[index]->creatingSystemGUID==guid)
			replicaListOut.Push(userReplicaList[index],_FILE_AND_LINE_);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unsigned ReplicaManager3::GetReplicaCount(void) const
{
	return userReplicaList.Size();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3 *ReplicaManager3::GetReplicaAtIndex(unsigned index)
{
	return userReplicaList[index];
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unsigned int ReplicaManager3::GetConnectionCount(void) const
{
	return connectionList.Size();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3* ReplicaManager3::GetConnectionAtIndex(unsigned index) const
{
	return connectionList[index];
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3* ReplicaManager3::GetConnectionBySystemAddress(const SystemAddress &sa) const
{
	unsigned int index;
	for (index=0; index < connectionList.Size(); index++)
	{
		if (connectionList[index]->GetSystemAddress()==sa)
		{
			return connectionList[index];
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3* ReplicaManager3::GetConnectionByGUID(RakNetGUID guid) const
{
	unsigned int index;
	for (index=0; index < connectionList.Size(); index++)
	{
		if (connectionList[index]->GetRakNetGUID()==guid)
		{
			return connectionList[index];
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetDefaultOrderingChannel(char def)
{
	defaultSendParameters.orderingChannel=def;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetDefaultPacketPriority(PacketPriority def)
{
	defaultSendParameters.priority=def;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetDefaultPacketReliability(PacketReliability def)
{
	defaultSendParameters.reliability=def;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetAutoSerializeInterval(RakNet::Time intervalMS)
{
	autoSerializeInterval=intervalMS;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetConnectionsThatHaveReplicaConstructed(Replica3 *replica, DataStructures::List<Connection_RM3*> &connectionsThatHaveConstructedThisReplica)
{
	connectionsThatHaveConstructedThisReplica.Clear(false,_FILE_AND_LINE_);
	unsigned int index;
	for (index=0; index < connectionList.Size(); index++)
	{
		if (connectionList[index]->HasReplicaConstructed(replica))
			connectionsThatHaveConstructedThisReplica.Push(connectionList[index],_FILE_AND_LINE_);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::Clear(void)
{
	if (autoDestroyConnections)
	{
		for (unsigned int i=0; i < connectionList.Size(); i++)
			DeallocConnection(connectionList[i]);
	}
	else
	{
		// Clear out downloadGroup even if not auto destroying the connection, since the packets need to go back to RakPeer
		for (unsigned int i=0; i < connectionList.Size(); i++)
			connectionList[i]->ClearDownloadGroup(rakPeerInterface);
	}



	connectionList.Clear(true,_FILE_AND_LINE_);
	userReplicaList.Clear(true,_FILE_AND_LINE_);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PRO ReplicaManager3::GetDefaultSendParameters(void) const
{
	return defaultSendParameters;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetWorldID(unsigned char id)
{
	worldId=id;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unsigned char ReplicaManager3::GetWorldID(void) const
{
	return worldId;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

NetworkIDManager *ReplicaManager3::GetNetworkIDManager(void) const
{
	return networkIDManager;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetNetworkIDManager(NetworkIDManager *_networkIDManager)
{
	networkIDManager=_networkIDManager;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnReceive(Packet *packet)
{
	if (packet->length<2)
		return RR_CONTINUE_PROCESSING;

	unsigned char incomingWorldId;

	RakNet::Time timestamp=0;
	unsigned char packetIdentifier, packetDataOffset;
	if ( ( unsigned char ) packet->data[ 0 ] == ID_TIMESTAMP )
	{
		if ( packet->length > sizeof( unsigned char ) + sizeof( RakNet::Time ) )
		{
			packetIdentifier = ( unsigned char ) packet->data[ sizeof( unsigned char ) + sizeof( RakNet::Time ) ];
			// Required for proper endian swapping
			RakNet::BitStream tsBs(packet->data+sizeof(MessageID),packet->length-1,false);
			tsBs.Read(timestamp);
			incomingWorldId=packet->data[sizeof( unsigned char )*2 + sizeof( RakNet::Time )];
			packetDataOffset=sizeof( unsigned char )*3 + sizeof( RakNet::Time );
		}
		else
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	else
	{
		packetIdentifier = ( unsigned char ) packet->data[ 0 ];
		incomingWorldId=packet->data[sizeof( unsigned char )];
		packetDataOffset=sizeof( unsigned char )*2;
	}

	switch (packetIdentifier)
	{
	case ID_REPLICA_MANAGER_CONSTRUCTION:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		return OnConstruction(packet, packet->data, packet->length, packet->guid, packetDataOffset);
	case ID_REPLICA_MANAGER_SERIALIZE:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		return OnSerialize(packet, packet->data, packet->length, packet->guid, timestamp, packetDataOffset);
	case ID_REPLICA_MANAGER_DOWNLOAD_STARTED:
		if (packet->wasGeneratedLocally==false)
		{
			if (incomingWorldId!=worldId)
				return RR_CONTINUE_PROCESSING;
			return OnDownloadStarted(packet, packet->data, packet->length, packet->guid, packetDataOffset);
		}
		else
			break;
	case ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE:
		if (packet->wasGeneratedLocally==false)
		{
			if (incomingWorldId!=worldId)
				return RR_CONTINUE_PROCESSING;
			return OnDownloadComplete(packet, packet->data, packet->length, packet->guid, packetDataOffset);
		}
		else
			break;
	case ID_REPLICA_MANAGER_SCOPE_CHANGE:
		{
			if (incomingWorldId!=worldId)
				return RR_CONTINUE_PROCESSING;

			Connection_RM3 *connection = GetConnectionByGUID(packet->guid);
			if (connection && connection->isValidated==false)
			{
				// This connection is now confirmed bidirectional
				connection->isValidated=true;
				// Reply back on validation
				connection->SendValidation(rakPeerInterface,worldId);
			}
		}
	}

	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::AutoConstructByQuery(ReplicaManager3 *replicaManager3)
{
	ValidateLists(replicaManager3);

	ConstructionMode constructionMode = QueryConstructionMode();

	unsigned int index;
	RM3ConstructionState constructionState;
	LastSerializationResult *lsr;
	index=0;

	constructedReplicasCulled.Clear(false,_FILE_AND_LINE_);
	destroyedReplicasCulled.Clear(false,_FILE_AND_LINE_);

	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		while (index < queryToConstructReplicaList.Size())
		{
			lsr=queryToConstructReplicaList[index];
			constructionState=lsr->replica->QueryConstruction(this, replicaManager3);
			if (constructionState==RM3CS_ALREADY_EXISTS_REMOTELY || constructionState==RM3CS_ALREADY_EXISTS_REMOTELY_DO_NOT_CONSTRUCT)
			{
				OnReplicaAlreadyExists(index, replicaManager3);
				if (constructionState==RM3CS_ALREADY_EXISTS_REMOTELY)
					constructedReplicasCulled.Push(lsr->replica,_FILE_AND_LINE_);

				/*
				if (constructionState==RM3CS_ALREADY_EXISTS_REMOTELY)
				{
					// Serialize construction data to this connection
					RakNet::BitStream bsOut;
					bsOut.Write((MessageID)ID_REPLICA_MANAGER_3_SERIALIZE_CONSTRUCTION_EXISTING);
					bsOut.Write(replicaManager3->GetWorldID());
					NetworkID networkId;
					networkId=lsr->replica->GetNetworkID();
					bsOut.Write(networkId);
					BitSize_t bitsWritten = bsOut.GetNumberOfBitsUsed();
					lsr->replica->SerializeConstructionExisting(&bsOut, this);
					if (bsOut.GetNumberOfBitsUsed()!=bitsWritten)
						replicaManager3->SendUnified(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,GetSystemAddress(), false);
				}

				// Serialize first serialization to this connection.
				// This is done here, as it isn't done in PushConstruction
				SerializeParameters sp;
				RakNet::BitStream emptyBs;
				for (index=0; index < (unsigned int) RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; index++)
				{
					sp.lastSentBitstream[index]=&emptyBs;
					sp.pro[index]=replicaManager3->GetDefaultSendParameters();
				}
				sp.bitsWrittenSoFar=0;
				sp.destinationConnection=this;
				sp.messageTimestamp=0;
				sp.whenLastSerialized=0;

				RakNet::Replica3 *replica = lsr->replica;

				RM3SerializationResult res = replica->Serialize(&sp);
				if (res!=RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION &&
					res!=RM3SR_DO_NOT_SERIALIZE &&
					res!=RM3SR_SERIALIZED_UNIQUELY)
				{
					bool allIndices[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
					for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
					{
						sp.bitsWrittenSoFar+=sp.outputBitstream[z].GetNumberOfBitsUsed();
						allIndices[z]=true;
					}
					if (SendSerialize(replica, allIndices, sp.outputBitstream, sp.messageTimestamp, sp.pro, replicaManager3->GetRakPeerInterface(), replicaManager3->GetWorldID())==SSICR_SENT_DATA)
						lsr->replica->whenLastSerialized=RakNet::GetTimeMS();
				}
				*/
			}
			else if (constructionState==RM3CS_SEND_CONSTRUCTION)
			{
				OnConstructToThisConnection(index, replicaManager3);
				constructedReplicasCulled.Push(lsr->replica,_FILE_AND_LINE_);
			}
			else if (constructionState==RM3CS_NEVER_CONSTRUCT)
			{
				OnNeverConstruct(index, replicaManager3);
			}
			else//  if (constructionState==RM3CS_NO_ACTION)
			{
				// Do nothing
				index++;
			}
		}

		if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
		{
			RM3DestructionState destructionState;
			index=0;
			while (index < queryToDestructReplicaList.Size())
			{
				lsr=queryToDestructReplicaList[index];
				destructionState=lsr->replica->QueryDestruction(this, replicaManager3);
				if (destructionState==RM3DS_SEND_DESTRUCTION)
				{
					OnSendDestructionFromQuery(index, replicaManager3);
					destroyedReplicasCulled.Push(lsr->replica,_FILE_AND_LINE_);
				}
				else if (destructionState==RM3DS_DO_NOT_QUERY_DESTRUCTION)
				{
					OnDoNotQueryDestruction(index, replicaManager3);
				}
				else//  if (destructionState==RM3CS_NO_ACTION)
				{
					// Do nothing
					index++;
				}
			}
		}
	}
	else if (constructionMode==QUERY_CONNECTION_FOR_REPLICA_LIST)
	{
		QueryReplicaList(constructedReplicasCulled,destroyedReplicasCulled);

		unsigned int idx1, idx2;

		// Create new
		for (idx2=0; idx2 < constructedReplicasCulled.Size(); idx2++)
			OnConstructToThisConnection(constructedReplicasCulled[idx2], replicaManager3);

		bool exists;
		for (idx2=0; idx2 < destroyedReplicasCulled.Size(); idx2++)
		{
			exists=false;
			bool objectExists;
			idx1=constructedReplicaList.GetIndexFromKey(destroyedReplicasCulled[idx2], &objectExists);
			RakAssert(objectExists);
			if (objectExists)
			{
				OnSendDestructionFromQuery(idx1,replicaManager3);
			}

			// If this assert hits, the user tried to destroy a replica that doesn't exist on the remote system
			RakAssert(exists);
		}
	}

	SendConstruction(constructedReplicasCulled,destroyedReplicasCulled,replicaManager3->defaultSendParameters,replicaManager3->rakPeerInterface,replicaManager3->worldId,replicaManager3);
}
void ReplicaManager3::Update(void)
{
	unsigned int index,index2;

	for (index=0; index < connectionList.Size(); index++)
	{
		if (connectionList[index]->isValidated==false)
			continue;
		connectionList[index]->AutoConstructByQuery(this);
	}

	RakNet::Time time = RakNet::GetTimeMS();

	if (time - lastAutoSerializeOccurance >= autoSerializeInterval)
	{
		for (index=0; index < userReplicaList.Size(); index++)
		{
			userReplicaList[index]->forceSendUntilNextUpdate=false;
			userReplicaList[index]->OnUserReplicaPreSerializeTick();
		}


		unsigned int index;
		SerializeParameters sp;
		sp.curTime=time;
		Connection_RM3 *connection;
		SendSerializeIfChangedResult ssicr;
		sp.messageTimestamp=0;
		for (int i=0; i < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; i++)
			sp.pro[i]=defaultSendParameters;
		index2=0;
		for (index=0; index < connectionList.Size(); index++)
		{
			connection = connectionList[index];
			sp.bitsWrittenSoFar=0;
			index2=0;
			while (index2 < connection->queryToSerializeReplicaList.Size())
			{
				sp.destinationConnection=connection;
				sp.whenLastSerialized=connection->queryToSerializeReplicaList[index2]->replica->whenLastSerialized;
				ssicr=connection->SendSerializeIfChanged(index2, &sp, GetRakPeerInterface(), GetWorldID(), this);
				if (ssicr==SSICR_SENT_DATA)
				{
					connection->queryToSerializeReplicaList[index2]->replica->whenLastSerialized=time;
					index2++;
				}
				else if (ssicr==SSICR_NEVER_SERIALIZE)
				{
					// Removed from the middle of the list
				}
				else
					index2++;
			}
		}

		lastAutoSerializeOccurance=time;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnClosedConnection(const SystemAddress &systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) systemAddress;
	if (autoDestroyConnections)
	{
		Connection_RM3 *connection = PopConnection(rakNetGUID);
		if (connection)
			DeallocConnection(connection);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnNewConnection(const SystemAddress &systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) isIncoming;
	if (autoCreateConnections)
	{
		Connection_RM3 *connection = AllocConnection(systemAddress, rakNetGUID);
		if (connection)
			PushConnection(connection);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnRakPeerShutdown(void)
{
	if (autoDestroyConnections)
	{
		while (connectionList.Size())
		{
			Connection_RM3 *connection = PopConnection(connectionList.Size()-1);
			if (connection)
				DeallocConnection(connection);
		}
	}

	Clear();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnDetach(void)
{
	OnRakPeerShutdown();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnConstruction(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
	{
		// Almost certainly a bug
		RakAssert("Got OnConstruction but no connection yet" && 0);
		return RR_CONTINUE_PROCESSING;
	}
	if (connection->groupConstructionAndSerialize)
	{
		connection->downloadGroup.Push(packet, __FILE__, __LINE__);
		return RR_STOP_PROCESSING;
	}

	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	uint16_t constructionObjectListSize, destructionObjectListSize, index, index2;
	BitSize_t streamEnd, writeAllocationIDEnd;
	Replica3 *replica;
	NetworkID networkId;
	RakNetGUID creatingSystemGuid;
	bool actuallyCreateObject=false;

	DataStructures::List<bool> actuallyCreateObjectList;
	DataStructures::List<Replica3*> constructionTickStack;

	RakAssert(networkIDManager);

	bsIn.Read(constructionObjectListSize);
	for (index=0; index < constructionObjectListSize; index++)
	{
		bsIn.Read(streamEnd);
		bsIn.Read(networkId);
		Replica3* existingReplica = networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId);
		bsIn.Read(actuallyCreateObject);
		actuallyCreateObjectList.Push(actuallyCreateObject, _FILE_AND_LINE_);
		bsIn.AlignReadToByteBoundary();

		if (actuallyCreateObject)
		{
			bsIn.Read(creatingSystemGuid);
			bsIn.Read(writeAllocationIDEnd);

			//printf("OnConstruction: %i\n",networkId.guid.g); // Removeme
			if (existingReplica)
			{
				existingReplica->replicaManager=this;

				// Network ID already in use
				connection->OnDownloadExisting(existingReplica, this);

				constructionTickStack.Push(0, _FILE_AND_LINE_);
				bsIn.SetReadOffset(streamEnd);
				continue;
			}

			bsIn.AlignReadToByteBoundary();
			replica = connection->AllocReplica(&bsIn, this);
			if (replica==0)
			{
				constructionTickStack.Push(0, _FILE_AND_LINE_);
				bsIn.SetReadOffset(streamEnd);
				continue;
			}

			// Go past the bitStream written to with WriteAllocationID(). Necessary in case the user didn't read out the bitStream the same way it was written
			// bitOffset2 is already aligned
			bsIn.SetReadOffset(writeAllocationIDEnd);

			replica->SetNetworkIDManager(networkIDManager);
			replica->SetNetworkID(networkId);

			replica->replicaManager=this;
			replica->creatingSystemGUID=creatingSystemGuid;

			if (!replica->QueryRemoteConstruction(connection) ||
				!replica->DeserializeConstruction(&bsIn, connection))
			{
				DeallocReplicaNoBroadcastDestruction(connection, replica);
				bsIn.SetReadOffset(streamEnd);
				constructionTickStack.Push(0, _FILE_AND_LINE_);
				continue;
			}

			constructionTickStack.Push(replica, _FILE_AND_LINE_);

			// Register the replica
			ReferenceInternal(replica);
		}
		else
		{
			if (existingReplica)
			{
				existingReplica->DeserializeConstructionExisting(&bsIn, connection);
				constructionTickStack.Push(existingReplica, _FILE_AND_LINE_);
			}
			else
			{
				constructionTickStack.Push(0, _FILE_AND_LINE_);
			}
		}


		bsIn.SetReadOffset(streamEnd);
		bsIn.AlignReadToByteBoundary();
	}

    RakAssert(constructionTickStack.Size()==constructionObjectListSize);
	RakAssert(actuallyCreateObjectList.Size()==constructionObjectListSize);

	RakNet::BitStream empty;
	for (index=0; index < constructionObjectListSize; index++)
	{
		bool pdcWritten=false;
		bsIn.Read(pdcWritten);
		if (pdcWritten)
		{
			bsIn.AlignReadToByteBoundary();
			bsIn.Read(streamEnd);
			bsIn.Read(networkId);
			if (constructionTickStack[index]!=0)
			{
				bsIn.AlignReadToByteBoundary();
				if (actuallyCreateObjectList[index])
					constructionTickStack[index]->PostDeserializeConstruction(&bsIn, connection);
				else
					constructionTickStack[index]->PostDeserializeConstructionExisting(&bsIn, connection);
			}
			bsIn.SetReadOffset(streamEnd);
		}
		else
		{
			if (constructionTickStack[index]!=0)
				constructionTickStack[index]->PostDeserializeConstruction(&empty, connection);
		}
	}

	for (index=0; index < constructionObjectListSize; index++)
	{
		if (constructionTickStack[index]!=0)
		{
			if (actuallyCreateObjectList[index])
			{
				// Tell the connection(s) that this object exists since they just sent it to us
				connection->OnDownloadFromThisSystem(constructionTickStack[index], this);

				for (index2=0; index2 < connectionList.Size(); index2++)
				{
					if (connectionList[index2]!=connection)
						connectionList[index2]->OnDownloadFromOtherSystem(constructionTickStack[index], this);
				}
			}
		}
	}

	// Destructions
	bool b = bsIn.Read(destructionObjectListSize);
	(void) b;
	RakAssert(b);
	for (index=0; index < destructionObjectListSize; index++)
	{
		bsIn.Read(networkId);
		bsIn.Read(streamEnd);
		replica = networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId);
		if (replica==0)
		{
			// Unknown object
			bsIn.SetReadOffset(streamEnd);
			continue;
		}
		bsIn.Read(replica->deletingSystemGUID);
		if (replica->DeserializeDestruction(&bsIn,connection))
		{
			// Make sure it wasn't deleted in DeserializeDestruction
			if (networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId))
			{
				replica->PreDestruction(connection);

				// Forward deletion by remote system
				if (replica->QueryRelayDestruction(connection))
					BroadcastDestruction(replica,connection->GetSystemAddress());
				Dereference(replica);
				DeallocReplicaNoBroadcastDestruction(connection, replica);
			}
		}
		else
		{
			replica->PreDestruction(connection);
			connection->OnDereference(replica, this);
		}

		bsIn.AlignReadToByteBoundary();
	}
	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnSerialize(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, RakNet::Time timestamp, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return RR_CONTINUE_PROCESSING;
	if (connection->groupConstructionAndSerialize)
	{
		connection->downloadGroup.Push(packet, __FILE__, __LINE__);
		return RR_STOP_PROCESSING;
	}
	RakAssert(networkIDManager);
	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);

	struct DeserializeParameters ds;
	ds.timeStamp=timestamp;
	ds.sourceConnection=connection;

	Replica3 *replica;
	NetworkID networkId;
	BitSize_t bitsUsed;
	bsIn.Read(networkId);
	//printf("OnSerialize: %i\n",networkId.guid.g); // Removeme
	replica = networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId);
	if (replica)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			bsIn.Read(ds.bitstreamWrittenTo[z]);
			if (ds.bitstreamWrittenTo[z])
			{
				bsIn.ReadCompressed(bitsUsed);
				bsIn.AlignReadToByteBoundary();
				bsIn.Read(ds.serializationBitstream[z], bitsUsed);
			}
		}
		replica->Deserialize(&ds);
	}
	return RR_CONTINUE_PROCESSING;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnDownloadStarted(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return RR_CONTINUE_PROCESSING;
	if (connection->QueryGroupDownloadMessages() &&
		// ID_DOWNLOAD_STARTED will be processed twice, being processed the second time once ID_DOWNLOAD_COMPLETE arrives.
		// However, the second time groupConstructionAndSerialize will be set to true so it won't be processed a third time
		connection->groupConstructionAndSerialize==false
		)
	{
		// These messages will be held by the plugin and returned when the download is complete
		connection->groupConstructionAndSerialize=true;
		RakAssert(connection->downloadGroup.Size()==0);
		connection->downloadGroup.Push(packet, __FILE__, __LINE__);
		return RR_STOP_PROCESSING;
	}

	connection->groupConstructionAndSerialize=false;
	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	connection->DeserializeOnDownloadStarted(&bsIn);
	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnDownloadComplete(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return RR_CONTINUE_PROCESSING;

	if (connection->groupConstructionAndSerialize==true && connection->downloadGroup.Size()>0)
	{
		// Push back buffered packets in front of this one
		unsigned int i;
		for (i=0; i < connection->downloadGroup.Size(); i++)
			rakPeerInterface->PushBackPacket(connection->downloadGroup[i],false);

		// Push this one to be last too. It will be processed again, but the second time 
		// groupConstructionAndSerialize will be false and downloadGroup will be empty, so it will go past this block
		connection->downloadGroup.Clear(__FILE__,__LINE__);
		rakPeerInterface->PushBackPacket(packet,false);

		return RR_STOP_PROCESSING;
	}

	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	connection->DeserializeOnDownloadComplete(&bsIn);
	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3* ReplicaManager3::GetReplicaByNetworkID(NetworkID networkId)
{
	unsigned int i;
	for (i=0; i < userReplicaList.Size(); i++)
	{
		if (userReplicaList[i]->GetNetworkID()==networkId)
			return userReplicaList[i];
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void ReplicaManager3::BroadcastDestructionList(DataStructures::List<Replica3*> &replicaListSource, const SystemAddress &exclusionAddress)
{
	RakNet::BitStream bsOut;
	unsigned int i,j;

	DataStructures::List<Replica3*> replicaList;

	for (i=0; i < replicaListSource.Size(); i++)
	{
		if (replicaListSource[i]==currentlyDeallocatingReplica)
			continue;
		replicaList.Push(replicaListSource[i], __FILE__, __LINE__);
	}

	if (replicaList.Size()==0)
		return;

	for (i=0; i < replicaList.Size(); i++)
	{
		if (replicaList[i]->deletingSystemGUID==UNASSIGNED_RAKNET_GUID)
			replicaList[i]->deletingSystemGUID=GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	}

	for (j=0; j < connectionList.Size(); j++)
	{
		if (connectionList[j]->GetSystemAddress()==exclusionAddress)
			continue;

		bsOut.Reset();
		bsOut.Write((MessageID)ID_REPLICA_MANAGER_CONSTRUCTION);
		bsOut.Write(worldId);
		uint16_t cnt=0;
		bsOut.Write(cnt); // No construction
		cnt=(uint16_t) replicaList.Size();
		BitSize_t cntOffset=bsOut.GetWriteOffset();;
		bsOut.Write(cnt); // Overwritten at send call
		cnt=0;

		for (i=0; i < replicaList.Size(); i++)
		{
			if (connectionList[j]->HasReplicaConstructed(replicaList[i])==false)
				continue;
			cnt++;

			NetworkID networkId;
			networkId=replicaList[i]->GetNetworkID();
			bsOut.Write(networkId);
			BitSize_t offsetStart, offsetEnd;
			offsetStart=bsOut.GetWriteOffset();
			bsOut.Write(offsetStart);
			bsOut.Write(replicaList[i]->deletingSystemGUID);
			replicaList[i]->SerializeDestruction(&bsOut, connectionList[j]);
			bsOut.AlignWriteToByteBoundary();
			offsetEnd=bsOut.GetWriteOffset();
			bsOut.SetWriteOffset(offsetStart);
			bsOut.Write(offsetEnd);
			bsOut.SetWriteOffset(offsetEnd);
		}

		if (cnt>0)
		{
			BitSize_t curOffset=bsOut.GetWriteOffset();
			bsOut.SetWriteOffset(cntOffset);
			bsOut.Write(cnt);
			bsOut.SetWriteOffset(curOffset);
			rakPeerInterface->Send(&bsOut,defaultSendParameters.priority,defaultSendParameters.reliability,defaultSendParameters.orderingChannel,connectionList[j]->GetSystemAddress(),false, defaultSendParameters.sendReceipt);
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void ReplicaManager3::BroadcastDestruction(Replica3 *replica, const SystemAddress &exclusionAddress)
{
	DataStructures::List<Replica3*> replicaList;
	replicaList.Push(replica, _FILE_AND_LINE_ );
	BroadcastDestructionList(replicaList,exclusionAddress);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3::Connection_RM3(const SystemAddress &_systemAddress, RakNetGUID _guid)
: systemAddress(_systemAddress), guid(_guid)
{
	isValidated=false;
	isFirstConstruction=true;
	groupConstructionAndSerialize=false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3::~Connection_RM3()
{
	unsigned int i;
	for (i=0; i < constructedReplicaList.Size(); i++)
		RakNet::OP_DELETE(constructedReplicaList[i], _FILE_AND_LINE_);
	for (i=0; i < queryToConstructReplicaList.Size(); i++)
		RakNet::OP_DELETE(queryToConstructReplicaList[i], _FILE_AND_LINE_);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::GetConstructedReplicas(DataStructures::List<Replica3*> &objectsTheyDoHave)
{
	objectsTheyDoHave.Clear(true,_FILE_AND_LINE_);
	for (unsigned int idx=0; idx < constructedReplicaList.Size(); idx++)
	{
		objectsTheyDoHave.Push(constructedReplicaList[idx]->replica, _FILE_AND_LINE_ );
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Connection_RM3::HasReplicaConstructed(RakNet::Replica3 *replica)
{
	bool objectExists;
	constructedReplicaList.GetIndexFromKey(replica, &objectExists);
	return objectExists;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Connection_RM3::SendSerializeHeader(RakNet::Replica3 *replica, RakNet::Time timestamp, RakNet::BitStream *bs, unsigned char worldId)
{
	bs->Reset();

	if (timestamp!=0)
	{
		bs->Write((MessageID)ID_TIMESTAMP);
		bs->Write(timestamp);
	}
	bs->Write((MessageID)ID_REPLICA_MANAGER_SERIALIZE);
	bs->Write(worldId);
	bs->Write(replica->GetNetworkID());
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Connection_RM3::ClearDownloadGroup(RakPeerInterface *rakPeerInterface)
{
	unsigned int i;
	for (i=0; i < downloadGroup.Size(); i++)
		rakPeerInterface->DeallocatePacket(downloadGroup[i]);
	downloadGroup.Clear(__FILE__,__LINE__);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
SendSerializeIfChangedResult Connection_RM3::SendSerialize(RakNet::Replica3 *replica, bool indicesToSend[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakNet::BitStream serializationData[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakNet::Time timestamp, PRO sendParameters[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakPeerInterface *rakPeer, unsigned char worldId)
{
	bool channelHasData;
	BitSize_t sum=0;
	for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
	{
		if (indicesToSend[z])
			sum+=serializationData[z].GetNumberOfBitsUsed();
	}
	if (sum==0)
		return SSICR_DID_NOT_SEND_DATA;

	RakAssert(replica->GetNetworkID()!=UNASSIGNED_NETWORK_ID);

	RakNet::BitStream out;
	BitSize_t bitsUsed;

	int channelIndex;
	PRO lastPro=sendParameters[0];

	for (channelIndex=0; channelIndex < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; channelIndex++)
	{
		if (channelIndex==0)
		{
			SendSerializeHeader(replica, timestamp, &out, worldId);
		}
		else if (lastPro!=sendParameters[channelIndex])
		{
			// Write out remainder
			for (int channelIndex2=channelIndex; channelIndex2 < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; channelIndex2++)
				out.Write(false);

			// Send remainder
			replica->OnSerializeTransmission(&out, systemAddress);
			rakPeer->Send(&out,lastPro.priority,lastPro.reliability,lastPro.orderingChannel,systemAddress,false,lastPro.sendReceipt);

			// If no data left to send, quit out
			bool anyData=false;
			for (int channelIndex2=channelIndex; channelIndex2 < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; channelIndex2++)
			{
				if (serializationData[channelIndex2].GetNumberOfBitsUsed()>0)
				{
					anyData=true;
					break;
				}
			}
			if (anyData==false)
				return SSICR_SENT_DATA;

			// Restart stream
			SendSerializeHeader(replica, timestamp, &out, worldId);

			for (int channelIndex2=0; channelIndex2 < channelIndex; channelIndex2++)
				out.Write(false);
			lastPro=sendParameters[channelIndex];
		}

		bitsUsed=serializationData[channelIndex].GetNumberOfBitsUsed();
		channelHasData = indicesToSend[channelIndex]==true && bitsUsed>0;
		out.Write(channelHasData);
		if (channelHasData)
		{
			out.WriteCompressed(bitsUsed);
			out.AlignWriteToByteBoundary();
			out.Write(serializationData[channelIndex]);
			// Crap, forgot this line, was a huge bug in that I'd only send to the first 3 systems
			serializationData[channelIndex].ResetReadPointer();
		}
	}
	replica->OnSerializeTransmission(&out, systemAddress);
	rakPeer->Send(&out,lastPro.priority,lastPro.reliability,lastPro.orderingChannel,systemAddress,false,lastPro.sendReceipt);
	return SSICR_SENT_DATA;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

SendSerializeIfChangedResult Connection_RM3::SendSerializeIfChanged(unsigned int queryToSerializeIndex, SerializeParameters *sp, RakNet::RakPeerInterface *rakPeer, unsigned char worldId, ReplicaManager3 *replicaManager)
{
	RakNet::Replica3 *replica = queryToSerializeReplicaList[queryToSerializeIndex]->replica;

	if (replica->GetNetworkID()==UNASSIGNED_NETWORK_ID)
		return SSICR_DID_NOT_SEND_DATA;

	RM3QuerySerializationResult rm3qsr = replica->QuerySerialization(this);
	if (rm3qsr==RM3QSR_NEVER_CALL_SERIALIZE)
	{
		// Never again for this connection and replica pair
		OnNeverSerialize(queryToSerializeIndex, replicaManager);
		return SSICR_NEVER_SERIALIZE;
	}

	if (rm3qsr==RM3QSR_DO_NOT_CALL_SERIALIZE)
		return SSICR_DID_NOT_SEND_DATA;

	if (replica->forceSendUntilNextUpdate)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			if (replica->lastSentSerialization.indicesToSend[z])
				sp->bitsWrittenSoFar+=replica->lastSentSerialization.bitStream[z].GetNumberOfBitsUsed();
		}
		return SendSerialize(replica, replica->lastSentSerialization.indicesToSend, replica->lastSentSerialization.bitStream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
	}

	for (int i=0; i < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; i++)
	{
		sp->outputBitstream[i].Reset();
		if (queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS)
			sp->lastSentBitstream[i]=&queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[i];
		else
			sp->lastSentBitstream[i]=&replica->lastSentSerialization.bitStream[i];
	}

	RM3SerializationResult serializationResult = replica->Serialize(sp);

	if (serializationResult==RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION)
	{
		// Never again for this connection and replica pair
		OnNeverSerialize(queryToSerializeIndex, replicaManager);
		return SSICR_NEVER_SERIALIZE;
	}

	if (serializationResult==RM3SR_DO_NOT_SERIALIZE)
	{
		// Don't serialize this tick only
		return SSICR_DID_NOT_SEND_DATA;
	}

	// This is necessary in case the user in the Serialize() function for some reason read the bitstream they also wrote
	// WIthout this code, the Write calls to another bitstream would not write the entire bitstream
	BitSize_t sum=0;
	for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
	{
		sp->outputBitstream[z].ResetReadPointer();
		sum+=sp->outputBitstream[z].GetNumberOfBitsUsed();
	}

	if (sum==0)
	{
		// Don't serialize this tick only
		return SSICR_DID_NOT_SEND_DATA;
	}

	if (serializationResult==RM3SR_SERIALIZED_ALWAYS)
	{
		bool allIndices[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
			allIndices[z]=true;

			queryToSerializeReplicaList[queryToSerializeIndex]->AllocBS();
			queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Reset();
			queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Write(&sp->outputBitstream[z]);
			sp->outputBitstream[z].ResetReadPointer();
		}
		return SendSerialize(replica, allIndices, sp->outputBitstream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
	}

	if (serializationResult==RM3SR_SERIALIZED_ALWAYS_IDENTICALLY)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			replica->lastSentSerialization.indicesToSend[z]=sp->outputBitstream[z].GetNumberOfBitsUsed()>0;
			sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
			replica->lastSentSerialization.bitStream[z].Reset();
			replica->lastSentSerialization.bitStream[z].Write(&sp->outputBitstream[z]);
			sp->outputBitstream[z].ResetReadPointer();
			replica->forceSendUntilNextUpdate=true;
		}
		return SendSerialize(replica, replica->lastSentSerialization.indicesToSend, sp->outputBitstream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
	}

	bool indicesToSend[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
	if (serializationResult==RM3SR_BROADCAST_IDENTICALLY || serializationResult==RM3SR_BROADCAST_IDENTICALLY_FORCE_SERIALIZATION)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			if (sp->outputBitstream[z].GetNumberOfBitsUsed() > 0 &&
				(serializationResult==RM3SR_BROADCAST_IDENTICALLY_FORCE_SERIALIZATION ||
				((sp->outputBitstream[z].GetNumberOfBitsUsed()!=replica->lastSentSerialization.bitStream[z].GetNumberOfBitsUsed() ||
				memcmp(sp->outputBitstream[z].GetData(), replica->lastSentSerialization.bitStream[z].GetData(), sp->outputBitstream[z].GetNumberOfBytesUsed())!=0))))
			{
				indicesToSend[z]=true;
				replica->lastSentSerialization.indicesToSend[z]=true;
				sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
				replica->lastSentSerialization.bitStream[z].Reset();
				replica->lastSentSerialization.bitStream[z].Write(&sp->outputBitstream[z]);
				sp->outputBitstream[z].ResetReadPointer();
				replica->forceSendUntilNextUpdate=true;
			}
			else
			{
				indicesToSend[z]=false;
				replica->lastSentSerialization.indicesToSend[z]=false;
			}
		}
	}
	else
	{
		queryToSerializeReplicaList[queryToSerializeIndex]->AllocBS();

		// RM3SR_SERIALIZED_UNIQUELY
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			if (sp->outputBitstream[z].GetNumberOfBitsUsed() > 0 &&
				(sp->outputBitstream[z].GetNumberOfBitsUsed()!=queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].GetNumberOfBitsUsed() ||
				memcmp(sp->outputBitstream[z].GetData(), queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].GetData(), sp->outputBitstream[z].GetNumberOfBytesUsed())!=0)
				)
			{
				indicesToSend[z]=true;
				sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
				queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Reset();
				queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Write(&sp->outputBitstream[z]);
				sp->outputBitstream[z].ResetReadPointer();
			}
			else
			{
				indicesToSend[z]=false;
			}
		}
	}


	if (serializationResult==RM3SR_BROADCAST_IDENTICALLY || serializationResult==RM3SR_BROADCAST_IDENTICALLY_FORCE_SERIALIZATION)
		replica->forceSendUntilNextUpdate=true;

	// Send out the data
	return SendSerialize(replica, indicesToSend, sp->outputBitstream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Connection_RM3::OnLocalReference(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION);
	(void) replicaManager;
	(void) constructionMode;

	LastSerializationResult* lsr=RakNet::OP_NEW<LastSerializationResult>(_FILE_AND_LINE_);
	lsr->replica=replica3;
	queryToConstructReplicaList.Push(lsr,_FILE_AND_LINE_);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDereference(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);

	LastSerializationResult* lsr=0;
	unsigned int idx;

	bool objectExists;
	idx=constructedReplicaList.GetIndexFromKey(replica3, &objectExists);
	if (objectExists)
	{
		lsr=constructedReplicaList[idx];
		constructedReplicaList.RemoveAtIndex(idx);
	}

	for (idx=0; idx < queryToConstructReplicaList.Size(); idx++)
	{
		if (queryToConstructReplicaList[idx]->replica==replica3)
		{
			lsr=queryToConstructReplicaList[idx];
			queryToConstructReplicaList.RemoveAtIndex(idx);
			break;
		}
	}

	for (idx=0; idx < queryToSerializeReplicaList.Size(); idx++)
	{
		if (queryToSerializeReplicaList[idx]->replica==replica3)
		{
			lsr=queryToSerializeReplicaList[idx];
			queryToSerializeReplicaList.RemoveAtIndex(idx);
			break;
		}
	}

	for (idx=0; idx < queryToDestructReplicaList.Size(); idx++)
	{
		if (queryToDestructReplicaList[idx]->replica==replica3)
		{
			lsr=queryToDestructReplicaList[idx];
			queryToDestructReplicaList.RemoveAtIndex(idx);
			break;
		}
	}

	ValidateLists(replicaManager);

	if (lsr)
		RakNet::OP_DELETE(lsr,_FILE_AND_LINE_);

	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDownloadFromThisSystem(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);
	LastSerializationResult* lsr=RakNet::OP_NEW<LastSerializationResult>(_FILE_AND_LINE_);
	lsr->replica=replica3;

	ConstructionMode constructionMode = QueryConstructionMode();
	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		unsigned int j;
		for (j=0; j < queryToConstructReplicaList.Size(); j++)
		{
			if (queryToConstructReplicaList[j]->replica->GetNetworkID()==replica3->GetNetworkID() )
			{
				queryToConstructReplicaList.RemoveAtIndex(j);
				break;
			}
		}

		queryToDestructReplicaList.Push(lsr,_FILE_AND_LINE_);
	}

	constructedReplicaList.Insert(lsr->replica, lsr, true, _FILE_AND_LINE_);
	//assert(queryToSerializeReplicaList.GetIndexOf(replica3)==(unsigned int)-1);
	queryToSerializeReplicaList.Push(lsr,_FILE_AND_LINE_);

	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDownloadFromOtherSystem(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		unsigned int j;
		for (j=0; j < queryToConstructReplicaList.Size(); j++)
		{
			if (queryToConstructReplicaList[j]->replica->GetNetworkID()==replica3->GetNetworkID() )
			{
				return;
			}
		}

		OnLocalReference(replica3, replicaManager);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnNeverConstruct(unsigned int queryToConstructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION);
	(void) constructionMode;

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToConstructReplicaList[queryToConstructIdx];
	queryToConstructReplicaList.RemoveAtIndex(queryToConstructIdx);
	RakNet::OP_DELETE(lsr,_FILE_AND_LINE_);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnConstructToThisConnection(unsigned int queryToConstructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION);
	(void) constructionMode;

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToConstructReplicaList[queryToConstructIdx];
	queryToConstructReplicaList.RemoveAtIndex(queryToConstructIdx);
	//assert(constructedReplicaList.GetIndexOf(lsr->replica)==(unsigned int)-1);
	constructedReplicaList.Insert(lsr->replica,lsr,true,_FILE_AND_LINE_);
	//assert(queryToDestructReplicaList.GetIndexOf(lsr->replica)==(unsigned int)-1);
	queryToDestructReplicaList.Push(lsr,_FILE_AND_LINE_);
	//assert(queryToSerializeReplicaList.GetIndexOf(lsr->replica)==(unsigned int)-1);
	queryToSerializeReplicaList.Push(lsr,_FILE_AND_LINE_);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnConstructToThisConnection(Replica3 *replica, ReplicaManager3 *replicaManager)
{
	RakAssert(QueryConstructionMode()==QUERY_CONNECTION_FOR_REPLICA_LIST);
	(void) replicaManager;

	LastSerializationResult* lsr=RakNet::OP_NEW<LastSerializationResult>(_FILE_AND_LINE_);
	lsr->replica=replica;
	constructedReplicaList.Insert(replica,lsr,true,_FILE_AND_LINE_);
	queryToSerializeReplicaList.Push(lsr,_FILE_AND_LINE_);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnNeverSerialize(unsigned int queryToSerializeIndex, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);
	queryToSerializeReplicaList.RemoveAtIndex(queryToSerializeIndex);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnReplicaAlreadyExists(unsigned int queryToConstructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION);
	(void) constructionMode;

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToConstructReplicaList[queryToConstructIdx];
	queryToConstructReplicaList.RemoveAtIndex(queryToConstructIdx);
	//assert(constructedReplicaList.GetIndexOf(lsr->replica)==(unsigned int)-1);
	constructedReplicaList.Insert(lsr->replica,lsr,true,_FILE_AND_LINE_);
	//assert(queryToDestructReplicaList.GetIndexOf(lsr->replica)==(unsigned int)-1);
	queryToDestructReplicaList.Push(lsr,_FILE_AND_LINE_);
	//assert(queryToSerializeReplicaList.GetIndexOf(lsr->replica)==(unsigned int)-1);
	queryToSerializeReplicaList.Push(lsr,_FILE_AND_LINE_);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDownloadExisting(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);

	ConstructionMode constructionMode = QueryConstructionMode();
	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		unsigned int idx;
		for (idx=0; idx < queryToConstructReplicaList.Size(); idx++)
		{
			if (queryToConstructReplicaList[idx]->replica==replica3)
			{
				OnConstructToThisConnection(idx, replicaManager);
				return;
			}
		}
	}
	else
	{
		OnConstructToThisConnection(replica3, replicaManager);
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnSendDestructionFromQuery(unsigned int queryToDestructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION);
	(void) constructionMode;

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToDestructReplicaList[queryToDestructIdx];
	queryToDestructReplicaList.RemoveAtIndex(queryToDestructIdx);
	unsigned int j;
	for (j=0; j < queryToSerializeReplicaList.Size(); j++)
	{
		if (queryToSerializeReplicaList[j]->replica->GetNetworkID()==lsr->replica->GetNetworkID() )
		{
			queryToSerializeReplicaList.RemoveAtIndex(j);
			break;
		}
	}
	for (j=0; j < constructedReplicaList.Size(); j++)
	{
		if (constructedReplicaList[j]->replica->GetNetworkID()==lsr->replica->GetNetworkID() )
		{
			constructedReplicaList.RemoveAtIndex(j);
			break;
		}
	}
	//assert(queryToConstructReplicaList.GetIndexOf(lsr->replica)==(unsigned int)-1);
	queryToConstructReplicaList.Push(lsr,_FILE_AND_LINE_);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDoNotQueryDestruction(unsigned int queryToDestructIdx, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);
	queryToDestructReplicaList.RemoveAtIndex(queryToDestructIdx);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::ValidateLists(ReplicaManager3 *replicaManager) const
{
	(void) replicaManager;
	/*
#ifdef _DEBUG
	// Each object should exist only once in either constructedReplicaList or queryToConstructReplicaList
	// replicaPointer from LastSerializationResult should be same among all lists
	unsigned int idx, idx2;
	for (idx=0; idx < constructedReplicaList.Size(); idx++)
	{
		idx2=queryToConstructReplicaList.GetIndexOf(constructedReplicaList[idx]->replica);
		if (idx2!=(unsigned int)-1)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}

	for (idx=0; idx < queryToConstructReplicaList.Size(); idx++)
	{
		idx2=constructedReplicaList.GetIndexOf(queryToConstructReplicaList[idx]->replica);
		if (idx2!=(unsigned int)-1)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}

	LastSerializationResult *lsr, *lsr2;
	for (idx=0; idx < constructedReplicaList.Size(); idx++)
	{
		lsr=constructedReplicaList[idx];

		idx2=queryToSerializeReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(unsigned int)-1)
		{
			lsr2=queryToSerializeReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}

		idx2=queryToDestructReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(unsigned int)-1)
		{
			lsr2=queryToDestructReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}
	}
	for (idx=0; idx < queryToConstructReplicaList.Size(); idx++)
	{
		lsr=queryToConstructReplicaList[idx];

		idx2=queryToSerializeReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(unsigned int)-1)
		{
			lsr2=queryToSerializeReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}

		idx2=queryToDestructReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(unsigned int)-1)
		{
			lsr2=queryToDestructReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}
	}

	// Verify pointer integrity
	for (idx=0; idx < constructedReplicaList.Size(); idx++)
	{
		if (constructedReplicaList[idx]->replica->replicaManager!=replicaManager)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}

	// Verify pointer integrity
	for (idx=0; idx < queryToConstructReplicaList.Size(); idx++)
	{
		if (queryToConstructReplicaList[idx]->replica->replicaManager!=replicaManager)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}
#endif
	*/
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::SendConstruction(DataStructures::List<Replica3*> &newObjects, DataStructures::List<Replica3*> &deletedObjects, PRO sendParameters, RakNet::RakPeerInterface *rakPeer, unsigned char worldId, ReplicaManager3 *replicaManager3)
{
	if (newObjects.Size()==0 && deletedObjects.Size()==0)
		return;

	// All construction and destruction takes place in the same network message
	// Otherwise, if objects rely on each other being created the same tick to be valid, this won't always be true
	//	DataStructures::List<LastSerializationResult* > serializedObjects;
	BitSize_t offsetStart, offsetStart2, offsetEnd;
	unsigned int newListIndex, oldListIndex;
	RakNet::BitStream bsOut;
	NetworkID networkId;
	if (isFirstConstruction)
	{
		bsOut.Write((MessageID)ID_REPLICA_MANAGER_DOWNLOAD_STARTED);
		bsOut.Write(worldId);
		SerializeOnDownloadStarted(&bsOut);
		rakPeer->Send(&bsOut,sendParameters.priority,RELIABLE_ORDERED,sendParameters.orderingChannel,systemAddress,false,sendParameters.sendReceipt);
	}

	//	LastSerializationResult* lsr;
	bsOut.Reset();
	bsOut.Write((MessageID)ID_REPLICA_MANAGER_CONSTRUCTION);
	bsOut.Write(worldId);
	uint16_t objectSize = (uint16_t) newObjects.Size();
	bsOut.Write(objectSize);

	// Construction
	for (newListIndex=0; newListIndex < newObjects.Size(); newListIndex++)
	{
		offsetStart=bsOut.GetWriteOffset();
		bsOut.Write(offsetStart); // overwritten to point to the end of the stream
		networkId=newObjects[newListIndex]->GetNetworkID();
		bsOut.Write(networkId);

		RM3ConstructionState cs = newObjects[newListIndex]->QueryConstruction(this, replicaManager3);
		bool actuallyCreateObject = cs==RM3CS_SEND_CONSTRUCTION;
		bsOut.Write(actuallyCreateObject);
		bsOut.AlignWriteToByteBoundary();

		if (actuallyCreateObject)
		{
			// Actually create the object
			bsOut.Write(newObjects[newListIndex]->creatingSystemGUID);
			offsetStart2=bsOut.GetWriteOffset();
			bsOut.Write(offsetStart2); // overwritten to point to after the call to WriteAllocationID
			bsOut.AlignWriteToByteBoundary(); // Give the user an aligned bitStream in case they use memcpy
			newObjects[newListIndex]->WriteAllocationID(this, &bsOut);
			bsOut.AlignWriteToByteBoundary();  // Give the user an aligned bitStream in case they use memcpy
			offsetEnd=bsOut.GetWriteOffset();
			bsOut.SetWriteOffset(offsetStart2);
			bsOut.Write(offsetEnd);
			bsOut.SetWriteOffset(offsetEnd);
			newObjects[newListIndex]->SerializeConstruction(&bsOut, this);
		}
		else
		{
			newObjects[newListIndex]->SerializeConstructionExisting(&bsOut, this);
		}

		bsOut.AlignWriteToByteBoundary();
		offsetEnd=bsOut.GetWriteOffset();
		bsOut.SetWriteOffset(offsetStart);
		bsOut.Write(offsetEnd);
		bsOut.SetWriteOffset(offsetEnd);
	}

	RakNet::BitStream bsOut2;
	for (newListIndex=0; newListIndex < newObjects.Size(); newListIndex++)
	{
		bsOut2.Reset();
		RM3ConstructionState cs = newObjects[newListIndex]->QueryConstruction(this, replicaManager3);
		if (cs==RM3CS_SEND_CONSTRUCTION)
		{
			newObjects[newListIndex]->PostSerializeConstruction(&bsOut2, this);
		}
		else
		{
			RakAssert(cs==RM3CS_ALREADY_EXISTS_REMOTELY);
			newObjects[newListIndex]->PostSerializeConstructionExisting(&bsOut2, this);
		}
		if (bsOut2.GetNumberOfBitsUsed()>0)
		{
			bsOut.Write(true);
			bsOut.AlignWriteToByteBoundary();
			offsetStart=bsOut.GetWriteOffset();
			bsOut.Write(offsetStart); // overwritten to point to the end of the stream
			networkId=newObjects[newListIndex]->GetNetworkID();
			bsOut.Write(networkId);
			bsOut.AlignWriteToByteBoundary(); // Give the user an aligned bitStream in case they use memcpy
			bsOut.Write(&bsOut2);
			bsOut.AlignWriteToByteBoundary(); // Give the user an aligned bitStream in case they use memcpy
			offsetEnd=bsOut.GetWriteOffset();
			bsOut.SetWriteOffset(offsetStart);
			bsOut.Write(offsetEnd);
			bsOut.SetWriteOffset(offsetEnd);
		}
		else
			bsOut.Write(false);
	}
	bsOut.AlignWriteToByteBoundary();

	// Destruction
	objectSize = (uint16_t) deletedObjects.Size();
	bsOut.Write(objectSize);
	for (oldListIndex=0; oldListIndex < deletedObjects.Size(); oldListIndex++)
	{
		networkId=deletedObjects[oldListIndex]->GetNetworkID();
		bsOut.Write(networkId);
		offsetStart=bsOut.GetWriteOffset();
		bsOut.Write(offsetStart);
		deletedObjects[oldListIndex]->deletingSystemGUID=rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
		bsOut.Write(deletedObjects[oldListIndex]->deletingSystemGUID);
		deletedObjects[oldListIndex]->SerializeDestruction(&bsOut, this);
		bsOut.AlignWriteToByteBoundary();
		offsetEnd=bsOut.GetWriteOffset();
		bsOut.SetWriteOffset(offsetStart);
		bsOut.Write(offsetEnd);
		bsOut.SetWriteOffset(offsetEnd);
	}
	rakPeer->Send(&bsOut,sendParameters.priority,RELIABLE_ORDERED,sendParameters.orderingChannel,systemAddress,false,sendParameters.sendReceipt);

	// TODO - shouldn't this be part of construction?

	// Initial Download serialize to a new system
	// Immediately send serialize after construction if the replica object already has saved data
	// If the object was serialized identically, and does not change later on, then the new connection never gets the data
	SerializeParameters sp;
	sp.whenLastSerialized=0;
	RakNet::BitStream emptyBs;
	for (int index=0; index < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; index++)
	{
		sp.lastSentBitstream[index]=&emptyBs;
		sp.pro[index]=sendParameters;
		sp.pro[index].reliability=RELIABLE_ORDERED;
	}

	sp.bitsWrittenSoFar=0;
	RakNet::Time t = RakNet::GetTimeMS();
	for (newListIndex=0; newListIndex < newObjects.Size(); newListIndex++)
	{
		sp.destinationConnection=this;
		sp.messageTimestamp=0;
		RakNet::Replica3 *replica = newObjects[newListIndex];
		// 8/22/09 Forgot ResetWritePointer
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			sp.outputBitstream[z].ResetWritePointer();
		}

		RM3SerializationResult res = replica->Serialize(&sp);
		if (res!=RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION &&
			res!=RM3SR_DO_NOT_SERIALIZE &&
			res!=RM3SR_SERIALIZED_UNIQUELY)
		{
			bool allIndices[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
			for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
			{
				sp.bitsWrittenSoFar+=sp.outputBitstream[z].GetNumberOfBitsUsed();
				allIndices[z]=true;
			}
			SendSerialize(replica, allIndices, sp.outputBitstream, sp.messageTimestamp, sp.pro, rakPeer, worldId);
			newObjects[newListIndex]->whenLastSerialized=t;

		}
		// else wait for construction request accepted before serializing
	}

	if (isFirstConstruction)
	{
		bsOut.Reset();
		bsOut.Write((MessageID)ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE);
		bsOut.Write(worldId);
		SerializeOnDownloadComplete(&bsOut);
		rakPeer->Send(&bsOut,sendParameters.priority,RELIABLE_ORDERED,sendParameters.orderingChannel,systemAddress,false,sendParameters.sendReceipt);
	}

	isFirstConstruction=false;

}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::SendValidation(RakNet::RakPeerInterface *rakPeer, unsigned char worldId)
{
	// Hijack to mean sendValidation
	RakNet::BitStream bsOut;
	bsOut.Write((MessageID)ID_REPLICA_MANAGER_SCOPE_CHANGE);
	bsOut.Write(worldId);
	rakPeer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,false);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3::Replica3()
{
	creatingSystemGUID=UNASSIGNED_RAKNET_GUID;
	deletingSystemGUID=UNASSIGNED_RAKNET_GUID;
	replicaManager=0;
	forceSendUntilNextUpdate=false;
	whenLastSerialized=0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3::~Replica3()
{
	if (replicaManager)
	{
		replicaManager->Dereference(this);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Replica3::BroadcastDestruction(void)
{
	replicaManager->BroadcastDestruction(this,UNASSIGNED_SYSTEM_ADDRESS);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RakNetGUID Replica3::GetCreatingSystemGUID(void) const
{
	return creatingSystemGUID;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ConstructionState Replica3::QueryConstruction_ClientConstruction(RakNet::Connection_RM3 *destinationConnection, bool isThisTheServer)
{
	(void) destinationConnection;
	if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		return RM3CS_SEND_CONSTRUCTION;
	// Send back to the owner client too, because they couldn't assign the network ID
	if (isThisTheServer)
		return RM3CS_SEND_CONSTRUCTION;
	return RM3CS_NEVER_CONSTRUCT;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Replica3::QueryRemoteConstruction_ClientConstruction(RakNet::Connection_RM3 *sourceConnection, bool isThisTheServer)
{
	(void) sourceConnection;
	(void) isThisTheServer;

	// OK to create
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ConstructionState Replica3::QueryConstruction_ServerConstruction(RakNet::Connection_RM3 *destinationConnection, bool isThisTheServer)
{
	(void) destinationConnection;

	if (isThisTheServer)
		return RM3CS_SEND_CONSTRUCTION;
	return RM3CS_NEVER_CONSTRUCT;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Replica3::QueryRemoteConstruction_ServerConstruction(RakNet::Connection_RM3 *sourceConnection, bool isThisTheServer)
{
	(void) sourceConnection;
	if (isThisTheServer)
		return false;
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ConstructionState Replica3::QueryConstruction_PeerToPeer(RakNet::Connection_RM3 *destinationConnection, Replica3P2PMode p2pMode)
{
	(void) destinationConnection;

	if (p2pMode==R3P2PM_SINGLE_OWNER)
	{
		// We send to all, others do nothing
		if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
			return RM3CS_SEND_CONSTRUCTION;

		// RM3CS_NEVER_CONSTRUCT will not send the object, and will not Serialize() it
		return RM3CS_NEVER_CONSTRUCT;
	}
	else if (p2pMode==R3P2PM_MULTI_OWNER_CURRENTLY_AUTHORITATIVE)
	{
		return RM3CS_SEND_CONSTRUCTION;
	}
	else
	{
		RakAssert(p2pMode==R3P2PM_MULTI_OWNER_NOT_CURRENTLY_AUTHORITATIVE);

		// RM3CS_ALREADY_EXISTS_REMOTELY will not send the object, but WILL call QuerySerialization() and Serialize() on it.
		return RM3CS_ALREADY_EXISTS_REMOTELY;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Replica3::QueryRemoteConstruction_PeerToPeer(RakNet::Connection_RM3 *sourceConnection)
{
	(void) sourceConnection;

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3QuerySerializationResult Replica3::QuerySerialization_ClientSerializable(RakNet::Connection_RM3 *destinationConnection, bool isThisTheServer)
{
	// Owner client sends to all
	if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		return RM3QSR_CALL_SERIALIZE;
	// Server sends to all but owner client
	if (isThisTheServer && destinationConnection->GetRakNetGUID()!=creatingSystemGUID)
		return RM3QSR_CALL_SERIALIZE;
	// Remote clients do not send
	return RM3QSR_NEVER_CALL_SERIALIZE;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3QuerySerializationResult Replica3::QuerySerialization_ServerSerializable(RakNet::Connection_RM3 *destinationConnection, bool isThisTheServer)
{
	(void) destinationConnection;
	// Server sends to all
	if (isThisTheServer)
		return RM3QSR_CALL_SERIALIZE;

	// Clients do not send
	return RM3QSR_NEVER_CALL_SERIALIZE;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3QuerySerializationResult Replica3::QuerySerialization_PeerToPeer(RakNet::Connection_RM3 *destinationConnection, Replica3P2PMode p2pMode)
{
	(void) destinationConnection;

	if (p2pMode==R3P2PM_SINGLE_OWNER)
	{
		// Owner peer sends to all
		if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
			return RM3QSR_CALL_SERIALIZE;

		// Remote peers do not send
		return RM3QSR_NEVER_CALL_SERIALIZE;
	}
	else if (p2pMode==R3P2PM_MULTI_OWNER_CURRENTLY_AUTHORITATIVE)
	{
		return RM3QSR_CALL_SERIALIZE;
	}
	else
	{
		RakAssert(p2pMode==R3P2PM_MULTI_OWNER_NOT_CURRENTLY_AUTHORITATIVE);
		return RM3QSR_DO_NOT_CALL_SERIALIZE;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ActionOnPopConnection Replica3::QueryActionOnPopConnection_Client(RakNet::Connection_RM3 *droppedConnection) const
{
	(void) droppedConnection;
	return RM3AOPC_DELETE_REPLICA;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ActionOnPopConnection Replica3::QueryActionOnPopConnection_Server(RakNet::Connection_RM3 *droppedConnection) const
{
	(void) droppedConnection;
	return RM3AOPC_DELETE_REPLICA_AND_BROADCAST_DESTRUCTION;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ActionOnPopConnection Replica3::QueryActionOnPopConnection_PeerToPeer(RakNet::Connection_RM3 *droppedConnection) const
{
	(void) droppedConnection;
	return RM3AOPC_DELETE_REPLICA;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif // _RAKNET_SUPPORT_*
