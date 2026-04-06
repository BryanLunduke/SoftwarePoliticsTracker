#ifndef __SOCKET_DEFINES_H
#define __SOCKET_DEFINES_H

/// Internal







































	#if   defined(_WIN32)
		#define closesocket__ closesocket
		#define select__ select
	#else
		#define closesocket__ close
		#define select__ select
	#endif
#define accept__ accept
#define connect__ connect
#define socket__ socket
#define bind__ bind
#define getsockname__ getsockname
#define getsockopt__ getsockopt
#define inet_addr__ inet_addr
#define ioctlsocket__ ioctlsocket
#define listen__ listen
#define recv__ recv
#define recvfrom__ recvfrom
#define sendto__ sendto
#define send__ send
#define setsockopt__ setsockopt
#define shutdown__ shutdown
#define WSASendTo__ WSASendTo


#endif
