#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::string getFormattedDateTime(std::time_t dt);
	DLL_LINKAGE std::string getDateTimeISO8601Basic(std::time_t dt);

}

VCMI_LIB_NAMESPACE_END
