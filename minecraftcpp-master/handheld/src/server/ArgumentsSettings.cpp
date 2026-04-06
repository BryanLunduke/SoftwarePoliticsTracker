#include "ArgumentsSettings.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
ArgumentsSettings::ArgumentsSettings(int numArguments, char** arguments)
: externalPath("."), levelName("level"), showHelp(false), port(19132), serverKey(""), cachePath("."), levelDir("level") {
	for(int a = 0; a < numArguments; ++a) {
		if(strcmp(arguments[a], "--help") == 0) {
			showHelp = true;
		} else if(strcmp(arguments[a], "--externalpath") == 0) {
			if(a + 1 < numArguments) {
				externalPath = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--levelname") == 0) {
			if(a + 1 < numArguments) {
				levelName = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--leveldir") == 0) {
			if(a + 1 < numArguments) {
				levelDir = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--port") == 0) {
			if(a + 1 < numArguments) {
				port = atoi(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--serverkey") == 0) {
			if(a + 1 < numArguments) {
				serverKey = std::string(arguments[a+1]);
				a++; // Skip the next argument since it's part of this one.
			}
		} else if(strcmp(arguments[a], "--cachepath") == 0) {
			if(a + 1 < numArguments) {
				cachePath = std::string(arguments[a+1]);
			}
		}
	}
}
std::string ArgumentsSettings::getExternalPath() {
	return externalPath;
}
std::string ArgumentsSettings::getLevelName() {
	return levelName;
}
std::string ArgumentsSettings::getServerKey() {
	return serverKey;
}
std::string ArgumentsSettings::getCachePath() {
	return cachePath;
}
std::string ArgumentsSettings::getLevelDir() {
	return levelDir;
}
bool ArgumentsSettings::getShowHelp() {
	return showHelp;
}
int ArgumentsSettings::getPort() {
	return port;
}
