#ifndef NET_MINECRAFT_SharedConstants_H__
#define NET_MINECRAFT_SharedConstants_H__

#include <string>

enum LevelGeneratorVersion
{
	LGV_ORIGINAL = 0,
};

namespace Common {
	std::string getGameVersionString(const std::string& versionSuffix = "");
}

namespace SharedConstants
{
	// 0.5.0 uses NPv8
	// 0.6.0 uses NPv9
    const int NetworkProtocolVersion = 9;
	const int NetworkProtocolLowestSupportedVersion = 9;
	const int GameProtocolVersion = 1;
	const int GameProtocolLowestSupportedVersion = 1;

	const int StorageVersion = 3;

	const int MaxChatLength = 100;
    
	const int TicksPerSecond = 20;

	const int GeneratorVersion = (int)LGV_ORIGINAL;
	//int FULLBRIGHT_LIGHTVALUE = 15 << 20 | 15 << 4;
}

#endif /*NET_MINECRAFT_SharedConstants_H__*/
