#include "StrUtils.h"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <regex>
#include <sstream>

#ifndef WIN32
extern "C" {
    #include <strings.h>
}
#endif

bool StrUtils::isInteger(std::string  str)
{
	if (StrUtils::startsWith(str, "-", false))
	{
		str = str.substr(1, str.size() - 1);
	}

	return !str.empty() && str.find_first_not_of("0123456789") == std::string::npos;
}

std::string StrUtils::ltrim(std::string str, std::string toRemove, bool ignoreCase)
{
	while (StrUtils::startsWith(str, toRemove, ignoreCase))
	{
		str = str.substr(toRemove.size());
	}

	return str;
}

std::string StrUtils::rtrim(std::string str, std::string toRemove, bool ignoreCase)
{
	while (StrUtils::endsWith(str, toRemove, ignoreCase))
	{
		str = str.substr(0, str.size() - toRemove.size());
	}

	return str;
}

int StrUtils::hexToInt(std::string str)
{
	if (StrUtils::startsWith(str, "0x", false))
	{
		str = str.substr(2, str.size() - 2);
	}

    int result;
    std::stringstream stream;

	stream << str;

    stream >> std::hex >> result;
    
	return result;
}

bool StrUtils::startsWith(std::string str, std::string prefix, bool ignoreCase)
{
	if (str.size() >= prefix.size())
	{
		std::string stringStart = str.substr(0, prefix.size());

		if (ignoreCase)
		{
		#ifdef _WIN32
			if (_stricmp(stringStart.c_str(), prefix.c_str()) == 0)
			{
				return true;
			}
		#else
			if (strcasecmp(stringStart.c_str(), prefix.c_str()) == 0)
			{
				return true;
			}
		#endif
		}

		if (stringStart == prefix)
		{
			return true;
		}
	}

	return false;
}

bool StrUtils::endsWith(std::string str, std::string suffix, bool ignoreCase)
{
	if (str.size() >= suffix.size())
	{
		std::string stringEnd = str.substr(str.size() - suffix.size(), suffix.size());

		if (ignoreCase)
		{
		#ifdef _WIN32
			if (_stricmp(stringEnd.c_str(), suffix.c_str()) == 0)
			{
				return true;
			}
		#else
			if (strcasecmp(stringEnd.c_str(), suffix.c_str()) == 0)
			{
				return true;
			}
		#endif
		}

		if (stringEnd == suffix)
		{
			return true;
		}
	}

	return false;
}

bool StrUtils::isRegexSubMatch(const std::string str, const std::string regex)
{
	std::regex re = std::regex(regex);
	std::smatch match;

	if (std::regex_search(str, match, re))
	{
		return true;
	}

	return false;
}

bool StrUtils::isHexNumber(std::string str)
{
	if (StrUtils::startsWith(str, "0x", false))
	{
		str = str.substr(2, str.size() - 2);
		return !str.empty() && str.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
	}

	return false;
}

std::string StrUtils::replaceAll(std::string str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;

	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}

	return str;
}
