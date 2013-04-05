#pragma once

#include "CConsoleHandler.h"

/*
 * CLogger.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern DLL_LINKAGE std::ostream *logfile;
extern DLL_LINKAGE CConsoleHandler *console;

// CLogger, prints log info to console and saves in file
class DLL_LINKAGE CLogger
{
	const int lvl;
#ifdef ANDROID
	std::ostringstream buf;
	int androidloglevel;
	void outputAndroid();
#endif

public:
	static const int CONSOLE_LOGGING_LEVEL;
	static const int FILE_LOGGING_LEVEL;
	
	CLogger& operator<<(std::ostream& (*fun)(std::ostream&));

	template<typename T> 
	CLogger & operator<<(const T & data)
	{
#ifdef ANDROID
		buf << data;
		outputAndroid();
#else
		if(lvl < CLogger::CONSOLE_LOGGING_LEVEL)
		{
			if(console)
                console->print(data, static_cast<EConsoleTextColor::EConsoleTextColor>(lvl));
			else
				std::cout << data << std::flush;
		}
		if((lvl < CLogger::FILE_LOGGING_LEVEL) && logfile)
			*logfile << data << std::flush;
#endif
		return *this;
	}

	CLogger(const int Lvl);
};

extern DLL_LINKAGE CLogger tlog0; //green - standard progress info
extern DLL_LINKAGE CLogger tlog1; //red - big errors
extern DLL_LINKAGE CLogger tlog2; //magenta - major warnings
extern DLL_LINKAGE CLogger tlog3; //yellow - minor warnings
extern DLL_LINKAGE CLogger tlog4; //white - detailed log info
extern DLL_LINKAGE CLogger tlog5; //gray - minor log info
extern DLL_LINKAGE CLogger tlog6; //teal - AI info
