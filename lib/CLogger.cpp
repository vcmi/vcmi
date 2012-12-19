#include "StdInc.h"
#include "CLogger.h"

/*
 * CLogger.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// Console, file definitions
DLL_LINKAGE CConsoleHandler *console = NULL;
DLL_LINKAGE std::ostream *logfile = NULL;

// CLogger definitions
DLL_LINKAGE CLogger tlog0(0);
DLL_LINKAGE CLogger tlog1(1);
DLL_LINKAGE CLogger tlog2(2);
DLL_LINKAGE CLogger tlog3(3);
DLL_LINKAGE CLogger tlog4(4);
DLL_LINKAGE CLogger tlog5(5);
DLL_LINKAGE CLogger tlog6(-2);

// Logging level settings
const int CLogger::CONSOLE_LOGGING_LEVEL = 5;
const int CLogger::FILE_LOGGING_LEVEL = 6;

CLogger::CLogger(const int Lvl) : lvl(Lvl)
{
#ifdef ANDROID
	androidloglevel = ANDROID_LOG_INFO;
	switch(lvl) {
case 0: androidloglevel = ANDROID_LOG_INFO; break;
case 1: androidloglevel = ANDROID_LOG_FATAL; break;
case 2: androidloglevel = ANDROID_LOG_ERROR; break;
case 3: androidloglevel = ANDROID_LOG_WARN; break;
case 4: androidloglevel = ANDROID_LOG_INFO; break;
case 5: androidloglevel = ANDROID_LOG_DEBUG; break;
case 6: case -2: androidloglevel = ANDROID_LOG_VERBOSE; break;
	}
#endif
}

#ifdef ANDROID
void CLogger::outputAndroid()
{
	int pos = buf.str().find("\n");
	while( pos >= 0 )
	{
		__android_log_print(androidloglevel, "VCMI", "%s", buf.str().substr(0, pos).c_str() );
		buf.str( buf.str().substr(pos+1) );
		pos = buf.str().find("\n");
	}
}
#endif

CLogger& CLogger::operator<<(std::ostream& (*fun)(std::ostream&))
{
#ifdef ANDROID
	buf << fun;
	outputAndroid();
#else
	if(lvl < CLogger::CONSOLE_LOGGING_LEVEL)
		std::cout << fun;
	if((lvl < CLogger::FILE_LOGGING_LEVEL) && logfile)
		*logfile << fun;
#endif
	return *this;
}