#ifndef _MINECRAFT_FOLDERMETHODS_H_
#define _MINECRAFT_FOLDERMETHODS_H_

#include "../../../platform/log.h"

#ifdef WIN32
	#include <io.h>
#else
int _mkdir(const char* name);
int _access(const char* name, int mode);
int _errno();
#endif

bool exists(const char* name);
bool createFolderIfNotExists(const char* name);

int getRemainingFileSize(FILE* fp);
int getFileSize(const char* filename);

bool createTree(const char* base, const char* tree[], int treeLength);

#endif
