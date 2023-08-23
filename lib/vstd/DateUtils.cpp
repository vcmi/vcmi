#include "StdInc.h"
#include <vstd/DateUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::string getFormattedDateTime(std::time_t dt)
	{
		std::tm tm = *std::localtime(&dt);
		std::stringstream s;
		try
		{
			s.imbue(std::locale(""));
		}
		catch(const std::runtime_error & e)
		{
			// locale not be available - keep default / global
		}
		s << std::put_time(&tm, "%x %X");
		return s.str();
	}

}

VCMI_LIB_NAMESPACE_END
