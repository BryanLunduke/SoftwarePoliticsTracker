#include "NatTypeDetectionCommon.h"
#include "SocketLayer.h"
#include "SocketIncludes.h"
#include "SocketDefines.h"

using namespace RakNet;

bool RakNet::CanConnect(NATTypeDetectionResult type1, NATTypeDetectionResult type2)
{
	/// If one system is NAT_TYPE_SYMMETRIC, the other must be NAT_TYPE_ADDRESS_RESTRICTED or less
	/// If one system is NAT_TYPE_PORT_RESTRICTED, the other must be NAT_TYPE_PORT_RESTRICTED or less
	bool connectionGraph[NAT_TYPE_COUNT][NAT_TYPE_COUNT] =
	{
		// None,	Full Cone,	Address Restricted,		Port Restricted,	Symmetric,	Unknown,	InProgress,	Supports_UPNP
		{true, 		true, 		true, 					true, 				true,		false,		false,		false},		// None
		{true, 		true, 		true, 					true, 				true,		false,		false,		false},		// Full Cone
		{true, 		true, 		true, 					true, 				true,		false,		false,		false},		// Address restricted
		{true, 		true, 		true, 					true, 				false,		false,		false,		false},		// Port restricted
		{true, 		true, 		true, 					false, 				false,		false,		false,		false},		// Symmetric
		{false,		false,		false,					false,				false,		false,		false,		false},		// Unknown
		{false,		false,		false,					false,				false,		false,		false,		false},		// InProgress
		{false,		false,		false,					false,				false,		false,		false,		false}		// Supports_UPNP
	};

	return connectionGraph[(int) type1][(int) type2];
}

const char *RakNet::NATTypeDetectionResultToString(NATTypeDetectionResult type)
{
	switch (type)
	{
	case NAT_TYPE_NONE:
		return "None";
	case NAT_TYPE_FULL_CONE:
		return "Full cone";
	case NAT_TYPE_ADDRESS_RESTRICTED:
		return "Address restricted";
	case NAT_TYPE_PORT_RESTRICTED:
		return "Port restricted";
	case NAT_TYPE_SYMMETRIC:
		return "Symmetric";
	case NAT_TYPE_UNKNOWN:
		return "Unknown";
	case NAT_TYPE_DETECTION_IN_PROGRESS:
		return "In Progress";
	case NAT_TYPE_SUPPORTS_UPNP:
		return "Supports UPNP";
	case NAT_TYPE_COUNT:
		return "NAT_TYPE_COUNT";
	}
	return "Error, unknown enum in NATTypeDetectionResult";
}

// None and relaxed can connect to anything
// Moderate can connect to moderate or less
// Strict can connect to relaxed or less
const char *RakNet::NATTypeDetectionResultToStringFriendly(NATTypeDetectionResult type)
{
	switch (type)
	{
	case NAT_TYPE_NONE:
		return "Open";
	case NAT_TYPE_FULL_CONE:
		return "Relaxed";
	case NAT_TYPE_ADDRESS_RESTRICTED:
		return "Relaxed";
	case NAT_TYPE_PORT_RESTRICTED:
		return "Moderate";
	case NAT_TYPE_SYMMETRIC:
		return "Strict";
	case NAT_TYPE_UNKNOWN:
		return "Unknown";
	case NAT_TYPE_DETECTION_IN_PROGRESS:
		return "In Progress";
	case NAT_TYPE_SUPPORTS_UPNP:
		return "Supports UPNP";
	case NAT_TYPE_COUNT:
		return "NAT_TYPE_COUNT";
	}
	return "Error, unknown enum in NATTypeDetectionResult";
}


SOCKET RakNet::CreateNonblockingBoundSocket(const char *bindAddr )
{
	SOCKET s = SocketLayer::CreateBoundSocket( 0, false, bindAddr, true, 0, AF_INET );
	#ifdef _WIN32
		unsigned long nonblocking = 1;
		ioctlsocket__( s, FIONBIO, &nonblocking );



	#else
		fcntl( s, F_SETFL, O_NONBLOCK );
	#endif
	return s;
}

int RakNet::NatTypeRecvFrom(char *data, SOCKET socket, SystemAddress &sender)
{
	sockaddr_in sa;
	socklen_t len2;
	const int flag=0;
	len2 = sizeof( sa );
	sa.sin_family = AF_INET;
	sa.sin_port=0;
	int len = recvfrom__( socket, data, MAXIMUM_MTU_SIZE, flag, ( sockaddr* ) & sa, ( socklen_t* ) & len2 );
	if (len>0)
	{
		sender.address.addr4.sin_family=AF_INET;
		sender.address.addr4.sin_addr.s_addr = sa.sin_addr.s_addr;
		//sender.SetPort( ntohs( sa.sin_port ) );
		sender.SetPort( ntohs( sa.sin_port ) );
	}
	return len;
}
