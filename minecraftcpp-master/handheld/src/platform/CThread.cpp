/*
 *  CThread.cpp
 *  oxeye
 *
 *  Created by aegzorz on 2007-02-09.
 *  Copyright 2007 Mojang AB. All rights reserved.
 *
 */

#include "CThread.h"



	CThread::CThread( pthread_fn threadFunc, void* threadParam )
	{
	#ifdef WIN32
		mp_threadFunc = (LPTHREAD_START_ROUTINE) threadFunc;
		
		m_threadHandle = CreateThread(
			NULL,				// pointer to security attributes
			NULL,               // initial thread stack size
			mp_threadFunc,		// pointer to thread function
			threadParam,        // argument for new thread
			NULL,               // creation flags
			&m_threadID        // pointer to receive thread ID
		);
	#endif
	#if defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX)
		mp_threadFunc = (pthread_fn)threadFunc;

		pthread_attr_init(&m_attributes);
		pthread_attr_setdetachstate( &m_attributes, PTHREAD_CREATE_DETACHED );
		/*int error =*/ pthread_create(&m_thread, &m_attributes, mp_threadFunc,threadParam);
	#endif
	#ifdef MACOSX
		mp_threadFunc = (TaskProc) threadFunc;
	
		MPCreateTask(
			mp_threadFunc,		// pointer to thread function
			threadParam,		// argument for new thread
			0,					// initial thread stack size
			NULL,				// queue id
			NULL,				// termination param 1
			NULL,				// termination param 2
			0,					// task options
			&m_threadID			// pointer to receive task ID
		);
	#endif
	}
	
	void CThread::sleep( const unsigned int millis )
	{
		#ifdef WIN32
			Sleep( millis );
		#endif
		#if defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX)
			usleep(millis * 1000);
		#endif
	}

	CThread::~CThread()
	{
	#ifdef WIN32
		TerminateThread(m_threadHandle, 0);
	#endif
	#if defined(LINUX) || defined(ANDROID) || defined(__APPLE__) || defined(POSIX)
		pthread_join(m_thread, NULL);
		pthread_attr_destroy(&m_attributes);
	#endif
	}


