#ifndef NET_MINECRAFT_CLIENT__OptionsFile_H__
#define NET_MINECRAFT_CLIENT__OptionsFile_H__

//package net.minecraft.client;
#include <string>
#include <vector>
typedef std::vector<std::string> StringVector;
class OptionsFile
{
public:
	OptionsFile();
    void save(const StringVector& settings);
	StringVector getOptionStrings();
	
private:
	std::string settingsPath;
};

#endif /* NET_MINECRAFT_CLIENT__OptionsFile_H__ */
