#include "StdInc.h"
#include <vstd/StringUtils.h>

namespace vstd
{

	DLL_LINKAGE std::vector<std::string> split(std::string s, std::string separators)
	{
		std::vector<std::string> result;
		boost::split(result, s, boost::is_any_of(separators));
		return result;
	}

	DLL_LINKAGE std::pair<std::string, std::string> splitStringToPair(std::string input, char separator)
	{
		std::pair<std::string, std::string> ret;
		size_t splitPos = input.find(separator);

		if (splitPos == std::string::npos)
		{
			ret.first.clear();
			ret.second = input;
		}
		else
		{
			ret.first = input.substr(0, splitPos);
			ret.second = input.substr(splitPos + 1);
		}
		return ret;
	}

}
