#pragma once

namespace vstd
{

	DLL_LINKAGE std::vector<std::string> split(std::string s, std::string separators);
	DLL_LINKAGE std::pair<std::string, std::string> splitStringToPair(std::string input, char separator);

}
