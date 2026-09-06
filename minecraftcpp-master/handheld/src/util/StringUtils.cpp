#include "StringUtils.h"
#include <algorithm>
#include "../platform/log.h"

namespace Util
{

/// @return true if <s> starts with <start>, false if not
bool startsWith(const std::string& s, const std::string& start) {
    const unsigned int len = start.length();
    return len <= s.length()
        && s.substr(0, len) == start;
}

// Naive (e.g. slow and stupid) implementation. @todo: fix
/// @return A reference to s
std::string& stringReplace(std::string& s, const std::string& src, const std::string& dst, int maxCount /*= -1*/) {
	int srcLength = src.length();

	while(maxCount--) {
		std::string::size_type pos = s.find(src);
		if (pos == std::string::npos)
			break;
		s.replace(pos, srcLength, dst);
	}
	return s;
}

/// @return A string trimmed from white space characters on both ends
std::string stringTrim(const std::string& s) {
	return stringTrim(s, " \t\n\r", true, true);
}

/// @return A string trimmed from given characters on any end
std::string stringTrim(const std::string& s, const std::string& chars, bool left, bool right) {
	const int len = s.length();
	const int lenChars = chars.length();

	if (len == 0 || lenChars == 0 || ((left || right) == false))
		return "";

	int i = 0, j = len-1;

	if (left) {
		for (; i < len; ++i)
			if (std::find(chars.begin(), chars.end(), s[i]) == chars.end())
				break;
	}
	if (right) {
		for (; j >= i; --j)
			if (std::find(chars.begin(), chars.end(), s[j]) == chars.end())
				break;
	}
	return s.substr(i, j - i + 1);
}

/// @return The "Java" implementation for string hash codes
int hashCode(const std::string& s) {
	const int len = s.length();

	int hash = 0;
	for (int i = 0; i < len; i++) {
		hash = ((hash << 5) - hash) + s[i];
	}
	return hash;
}

void removeAll(std::string& s, const char** rep, int repCount) {
	for (int i = 0; i < repCount; ++i)
		stringReplace(s, rep[i], "");
}

}; // end namespace Util
