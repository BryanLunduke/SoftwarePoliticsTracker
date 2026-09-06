#ifndef STRINGUTILS_H__
#define STRINGUTILS_H__

#include <string>

namespace Util {

/// @return true if <s> starts with <start>, false if not
bool startsWith(const std::string& s, const std::string& start);

/// @return A reference to s
std::string& stringReplace(std::string& s, const std::string& src, const std::string& dst, int maxCount = -1);

/// @return A string trimmed from white space characters on both ends
std::string stringTrim(const std::string& S);

/// @return A string trimmed from given characters on any end
std::string stringTrim(const std::string& S, const std::string& chars, bool left = true, bool right = true);

void removeAll(std::string& s, const char** rep, int repCount);

/// @return The "Java" implementation for string hash codes
int hashCode(const std::string& s);

}; // end namespace Util

#endif /*STRINGUTILS_H__*/
