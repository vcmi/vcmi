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

}
