#include "StdInc.h"
#include <vstd/DateUtils.h>

#if defined(VCMI_ANDROID)
#include "../CAndroidVMHelper.h"
#endif

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::string getFormattedDateTime(std::time_t dt)
	{
#if defined(VCMI_ANDROID)
		CAndroidVMHelper vmHelper;
		return vmHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "getFormattedDateTime");
#endif

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

	DLL_LINKAGE std::string getDateTimeISO8601Basic(std::time_t dt)
	{
		std::tm tm = *std::localtime(&dt);
		std::stringstream s;
		s << std::put_time(&tm, "%Y%m%dT%H%M%S");
		return s.str();
	}

	DLL_LINKAGE std::string getDateTimeInAutoSaveFormat(std::time_t dt)
	{
		std::tm tm = *std::localtime(&dt);
		std::stringstream s;
		s << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
		return s.str();
	}

}

VCMI_LIB_NAMESPACE_END
