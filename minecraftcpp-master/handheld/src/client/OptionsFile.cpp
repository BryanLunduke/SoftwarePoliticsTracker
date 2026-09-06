#include "OptionsFile.h"
#include <stdio.h>
#include <string.h>

OptionsFile::OptionsFile() {
#ifdef __APPLE__
	settingsPath = "./Documents/options.txt";
#elif defined(ANDROID)
	settingsPath = "options.txt";
#else
	settingsPath = "options.txt";
#endif
}

void OptionsFile::save(const StringVector& settings) {
	FILE* pFile = fopen(settingsPath.c_str(), "w");
	if(pFile != NULL) {
		for(StringVector::const_iterator it = settings.begin(); it != settings.end(); ++it) {
			fprintf(pFile, "%s\n", it->c_str());
		}
		fclose(pFile);
	}
}

StringVector OptionsFile::getOptionStrings() {
	StringVector returnVector;
	FILE* pFile = fopen(settingsPath.c_str(), "w");
	if(pFile != NULL) {
		char lineBuff[128];
		while(fgets(lineBuff, sizeof lineBuff, pFile)) {
			if(strlen(lineBuff) > 2)
				returnVector.push_back(std::string(lineBuff));
		}
		fclose(pFile);
	}
	return returnVector;
}
