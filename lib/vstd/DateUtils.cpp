#include "StdInc.h"
#include <vstd/DateUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::string getFormattedDateTime(std::time_t dt)
	{
		std::tm tm = *std::localtime(&dt);
		std::stringstream s;
		s.imbue(std::locale(""));
		s << std::put_time(&tm, "%x %X");
		return s.str();
	}

}

VCMI_LIB_NAMESPACE_END
