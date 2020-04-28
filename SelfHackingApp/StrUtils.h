#pragma once
#include <string>
#include <vector>

class StrUtils
{
public:
	static bool isInteger(std::string str);
	static std::string ltrim(std::string str, std::string toRemove, bool ignoreCase = false);
	static std::string rtrim(std::string str, std::string toRemove, bool ignoreCase = false);
	static bool startsWith(std::string str, std::string prefix, bool ignoreCase);
	static bool endsWith(std::string str, std::string suffix, bool ignoreCase);
	static bool isRegexSubMatch(const std::string str, const std::string regex);
	static int hexToInt(std::string str);
	static bool isHexNumber(std::string str);
	static std::string replaceAll(std::string str, const std::string& from, const std::string& to);
};
