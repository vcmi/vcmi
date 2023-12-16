#include "StdInc.h"
#include <vstd/DateUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::string getFormattedDateTime(std::time_t dt, std::string format)
	{
		std::tm tm = *std::localtime(&dt);
		std::stringstream s;
		s << std::put_time(&tm, format.c_str());
		return s.str();
	}

	DLL_LINKAGE std::string getDateTimeISO8601Basic(std::time_t dt)
	{
		return getFormattedDateTime(dt, "%Y%m%dT%H%M%S");
	}

}

VCMI_LIB_NAMESPACE_END
