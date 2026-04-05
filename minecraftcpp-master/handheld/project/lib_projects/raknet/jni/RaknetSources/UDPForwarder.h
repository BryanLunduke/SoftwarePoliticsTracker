/// \file
/// \brief Forwards UDP datagrams. Independent of RakNet's protocol.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_UDPForwarder==1

#ifndef __UDP_FORWARDER_H
#define __UDP_FORWARDER_H

#include "Export.h"
#include "RakNetTypes.h"
#include "SocketIncludes.h"
#include "UDPProxyCommon.h"
#include "SimpleMutex.h"
#include "RakString.h"
#include "RakThread.h"
#include "DS_Queue.h"
#include "DS_OrderedList.h"

#define UDP_FORWARDER_EXECUTE_THREADED

namespace RakNet
{

enum UDPForwarderResult
{
	UDPFORWARDER_FORWARDING_ALREADY_EXISTS,
	UDPFORWARDER_NO_SOCKETS,
	UDPFORWARDER_BIND_FAILED,
	UDPFORWARDER_INVALID_PARAMETERS,
	UDPFORWARDER_SUCCESS,

};

/// \brief Forwards UDP datagrams. Independent of RakNet's protocol.
/// \ingroup NAT_PUNCHTHROUGH_GROUP
class RAK_DLL_EXPORT UDPForwarder
{
public:
	UDPForwarder();
	~UDPForwarder();

	/// Starts the system.
	/// Required to call before StartForwarding
	void Startup(void);

	/// Stops the system, and frees all sockets
	void Shutdown(void);

	/// Call on a regular basis, unless using UDP_FORWARDER_EXECUTE_THREADED.
	/// Will call select__() on all sockets and forward messages.
	void Update(void);

	/// Sets the maximum number of forwarding entries allowed
	/// Set according to your available bandwidth and the estimated average bandwidth per forwarded address.
	/// A single connection requires 2 entries, as connections are bi-directional.
	/// \param[in] maxEntries The maximum number of simultaneous forwarding entries. Defaults to 64 (32 connections)
	void SetMaxForwardEntries(unsigned short maxEntries);

	/// \return The \a maxEntries parameter passed to SetMaxForwardEntries(), or the default if it was never called
	int GetMaxForwardEntries(void) const;

	/// \note Each call to StartForwarding uses up two forwarding entries, since communications are bidirectional
	/// \return How many entries have been used
	int GetUsedForwardEntries(void) const;

	/// Forwards datagrams from source to destination, and vice-versa
	/// Does nothing if this forward entry already exists via a previous call
	/// \pre Call Startup()
	/// \note RakNet's protocol will ensure a message is sent at least every 15 seconds, so if routing RakNet messages, it is a reasonable value for timeoutOnNoDataMS, plus an some extra seconds for latency
	/// \param[in] source The source IP and port
	/// \param[in] destination Where to forward to (and vice-versa)
	/// \param[in] timeoutOnNoDataMS If no messages are forwarded for this many MS, then automatically remove this entry. Currently hardcoded to UDP_FORWARDER_MAXIMUM_TIMEOUT (else the call fails)
	/// \param[in] forceHostAddress Force binding on a particular address. 0 to use any.
	/// \param[in] socketFamily IP version: For IPV4, use AF_INET (default). For IPV6, use AF_INET6. To autoselect, use AF_UNSPEC.
	short socketFamily;
	/// \param[out] forwardingPort New opened port for forwarding
	/// \param[out] forwardingSocket New opened socket for forwarding
	/// \return UDPForwarderResult
	UDPForwarderResult StartForwarding(SystemAddress source, SystemAddress destination, RakNet::TimeMS timeoutOnNoDataMS, const char *forceHostAddress, unsigned short socketFamily,
		unsigned short *forwardingPort, SOCKET *forwardingSocket);

	/// No longer forward datagrams from source to destination
	/// \param[in] source The source IP and port
	/// \param[in] destination Where to forward to
	void StopForwarding(SystemAddress source, SystemAddress destination);

	struct SrcAndDest
	{
		SrcAndDest() {}
		SrcAndDest(SystemAddress sa1, SystemAddress sa2)
		{
			if (sa1 < sa2)
			{
				source=sa1;
				dest=sa2;
			}
			else
			{
				source=sa2;
				dest=sa1;
			}
		}
		SystemAddress source;
		SystemAddress dest;
	};

	struct ForwardEntry
	{
		ForwardEntry();
		~ForwardEntry();
		SrcAndDest srcAndDest;
		RakNet::TimeMS timeLastDatagramForwarded;
		SOCKET socket;
		RakNet::TimeMS timeoutOnNoDataMS;
		bool updatedSourcePort, updatedDestPort;
		short socketFamily;
	};


protected:

	static int SrcAndDestForwardEntryComp( const SrcAndDest &inputKey, ForwardEntry * const &cls );


	friend RAK_THREAD_DECLARATION(UpdateUDPForwarder);
	struct ThreadOperation
	{
		enum {
		TO_NONE,
		TO_START_FORWARDING,
		TO_STOP_FORWARDING,
		} operation;

		SystemAddress source;
		SystemAddress destination;
		RakNet::TimeMS timeoutOnNoDataMS;
		RakNet::RakString forceHostAddress;
		unsigned short forwardingPort;
		SOCKET forwardingSocket;
		UDPForwarderResult result;
		unsigned short socketFamily;
	};
	SimpleMutex threadOperationIncomingMutex,threadOperationOutgoingMutex;
	DataStructures::Queue<ThreadOperation> threadOperationIncomingQueue;
	DataStructures::Queue<ThreadOperation> threadOperationOutgoingQueue;

#if RAKNET_SUPPORT_IPV6==1
	void UpdateThreaded(void);
#endif
	void UpdateThreaded_Old(void);
	UDPForwarderResult StartForwardingThreaded(SystemAddress source, SystemAddress destination, RakNet::TimeMS timeoutOnNoDataMS, const char *forceHostAddress, unsigned short socketFamily,
		unsigned short *forwardingPort, SOCKET *forwardingSocket);
	void StopForwardingThreaded(SystemAddress source, SystemAddress destination);

	DataStructures::OrderedList<SrcAndDest, ForwardEntry*, SrcAndDestForwardEntryComp> forwardList;
	unsigned short maxForwardEntries;

	UDPForwarderResult AddForwardingEntry(SrcAndDest srcAndDest, RakNet::TimeMS timeoutOnNoDataMS, unsigned short *port, const char *forceHostAddress, short socketFamily);


	bool isRunning, threadRunning;



};

} // End namespace

#endif

#endif // #if _RAKNET_SUPPORT_UDPForwarder==1
