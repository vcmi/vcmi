#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::vector<std::string> split(std::string s, const std::string& separators);
	DLL_LINKAGE std::pair<std::string, std::string> splitStringToPair(const std::string& input, char separator);

}

VCMI_LIB_NAMESPACE_END
