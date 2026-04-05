#include "RakNetSocket.h"
#include "SocketIncludes.h"
#include "SocketDefines.h"

using namespace RakNet;

RakNetSocket::RakNetSocket() {
	s = (unsigned int)-1;
#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
	recvEvent=INVALID_HANDLE_VALUE;
#endif
}
RakNetSocket::~RakNetSocket() 
{
	if ((SOCKET)s != (SOCKET)-1)
		closesocket__(s);

#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
	if (recvEvent!=INVALID_HANDLE_VALUE)
	{
		CloseHandle( recvEvent );
		recvEvent = INVALID_HANDLE_VALUE;
	}
#endif
}
