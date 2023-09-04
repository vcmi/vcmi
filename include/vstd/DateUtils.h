#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
	DLL_LINKAGE std::string getDateTimeLocalized(std::time_t dt);
	DLL_LINKAGE std::string getDateTimeFormatted(std::time_t dt);
}

VCMI_LIB_NAMESPACE_END
