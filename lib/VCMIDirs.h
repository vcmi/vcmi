#ifndef __VCMIDIRS_H__
#define __VCMIDIRS_H__

/*
 * UserHome.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#ifndef _WIN32 //we need boost here only on non-win platforms
	#include <boost/filesystem.hpp> 
	using namespace boost::filesystem;
#endif


/// Where to find the various VCMI files. This is mostly useful for linux. 
class VCMIDirs {
public:
	std::string UserPath;

	VCMIDirs()
	{
#ifdef _WIN32
		UserPath = DATA_DIR;
#else
		try {
#ifdef ANDROID
			UserPath = DATA_DIR;
#else
			// Find vcmi user directory and create it if necessary
			std::string home_dir = ".";
			if( getenv("HOME") != NULL )
				home_dir = getenv("HOME");

			UserPath = path(home_dir + "/.vcmi").string();
#endif
			create_directory(UserPath);
			create_directory(UserPath + "/config");
			create_directory(UserPath + "/Games");

			/* Home directory can contain some extra maps. */
			create_directory(UserPath + "/Maps");
		}
		catch(const std::exception & e)
		{
		}
#endif
	}
};
extern VCMIDirs GVCMIDirs;

#endif	// __VCMIDIRS_H__
