#include "SharedConstants.h"

namespace Common {

std::string getGameVersionString(const std::string& versionSuffix /* = "" */)
{
	return std::string("v0.6.1") + versionSuffix + " alpha";
}

};
