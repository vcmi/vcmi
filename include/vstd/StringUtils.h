#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::vector<std::string> split(std::string s, std::string separators);
	DLL_LINKAGE std::pair<std::string, std::string> splitStringToPair(std::string input, char separator);

}

VCMI_LIB_NAMESPACE_END
