#ifndef LOG_H__
#define LOG_H__

#ifdef __cplusplus
	#include <cstdio>
#else
	#include <stdio.h>
#endif

#define __LOG_PUBLISH(...) do { __VA_ARGS__; } while(0)

#ifdef ANDROID
	#include <android/log.h>
	#ifdef ANDROID_PUBLISH
		#define LOGV(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
		#define LOGI(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
		#define LOGW(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
		#define LOGE(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
	#else
		// @todo @fix; Obiously the tag shouldn't be hardcoded in here..
		#define LOGV(...) ((void)__android_log_print( ANDROID_LOG_VERBOSE, "MinecraftPE", __VA_ARGS__ ))
		#define LOGI(...) ((void)__android_log_print( ANDROID_LOG_INFO,  "MinecraftPE", __VA_ARGS__ ))
		#define LOGW(...) ((void)__android_log_print( ANDROID_LOG_WARN,  "MinecraftPE", __VA_ARGS__ ))
		#define LOGE(...) ((void)__android_log_print( ANDROID_LOG_ERROR, "MinecraftPE", __VA_ARGS__ ))
		#define printf LOGI
	#endif
#else
#ifdef PUBLISH
    #define LOGV(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
    #define LOGI(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
    #define LOGW(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
    #define LOGE(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
#else
	#define LOGV(...) (printf(__VA_ARGS__))
	#define LOGI(...) (printf(__VA_ARGS__))
	#define LOGW(...) (printf(__VA_ARGS__))
	#define LOGE(...) (printf(__VA_ARGS__))
#endif
#endif

#ifdef _DEBUG
	#define LOGVV LOGV
#else
	#define LOGVV(fmt, ...) __LOG_PUBLISH(__VA_ARGS__)
#endif

#endif /*LOG_H__*/
