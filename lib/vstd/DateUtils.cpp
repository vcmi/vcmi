#include "StdInc.h"
#include <vstd/DateUtils.h>

#include "../CConfigHandler.h"
#include "../Languages.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::string getFormattedDateTime(std::time_t dt)
	{
		std::string lang = settings["general"]["language"].String();
		std::string locale = Languages::getLanguageOptions(lang).locale;

		std::tm tm = *std::localtime(&dt);
		std::stringstream s;
		try
		{
			if(locale.empty())
				s.imbue(std::locale(""));
			else
				s.imbue(std::locale(locale));
		}
		catch(const std::runtime_error & e)
		{
			// locale not be available - keep default / global
		}
		s << std::put_time(&tm, "%x %X");
		return s.str();
	}

	DLL_LINKAGE std::string getDateTimeISO8601Basic(std::time_t dt)
	{
		std::tm tm = *std::localtime(&dt);
		std::stringstream s;
		s << std::put_time(&tm, "%Y%m%dT%H%M%S");
		return s.str();
	}

}

VCMI_LIB_NAMESPACE_END
