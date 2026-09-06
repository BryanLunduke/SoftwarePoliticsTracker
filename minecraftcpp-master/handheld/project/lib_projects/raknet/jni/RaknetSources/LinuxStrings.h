#ifndef _GCC_WIN_STRINGS
#define _GCC_WIN_STRINGS









	#if (defined(__GNUC__)  || defined(__GCCXML__) || defined(__S3E__)) && !defined(_WIN32)
		#ifndef _stricmp
			int _stricmp(const char* s1, const char* s2);
		#endif 
		int _strnicmp(const char* s1, const char* s2, size_t n);
#ifndef __APPLE__
		char *_strlwr(char * str ); //this won't compile on OSX for some reason
#endif
		#ifndef _vsnprintf
		#define _vsnprintf vsnprintf
		#endif 
	#endif


#endif // _GCC_WIN_STRINGS
