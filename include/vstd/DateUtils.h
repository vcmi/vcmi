#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{

	DLL_LINKAGE std::string getFormattedDateTime(std::time_t dt, std::string format);
	DLL_LINKAGE std::string getDateTimeISO8601Basic(std::time_t dt);

	inline std::tm safeLocalTime(std::time_t dt)
	{
		std::tm tm{};
#ifdef _WIN32
		localtime_s(&tm, &dt);
#else
		localtime_r(&dt, &tm);
#endif
		return tm;
	}

}

VCMI_LIB_NAMESPACE_END
