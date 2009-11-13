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


/* Where to find the various VCMI files. This is mostly usefull for linux. */
class VCMIDirs {
public:
	std::string UserPath;

	VCMIDirs()
	{
#ifdef _WIN32
		UserPath = DATA_DIR;
#else
		// Find vcmi user directory and create it if necessary
		std::string home_dir = getenv("HOME");
		UserPath = path(home_dir + "/.vcmi").string();

		create_directory(UserPath);
		create_directory(UserPath + "/config");
		create_directory(UserPath + "/Games");
#endif
	}
};
extern VCMIDirs GVCMIDirs;
