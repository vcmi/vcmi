#define VCMI_DLL
#include "CFileUtility.h"
#include <boost/filesystem.hpp>   // includes all needed Boost.Filesystem declarations
#include <boost/algorithm/string/predicate.hpp>

/*
 * CFileUtility.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


namespace fs = boost::filesystem;

CFileUtility::CFileUtility(void)
{
}


CFileUtility::~CFileUtility(void)
{
}

void CFileUtility::getFilesWithExt(std::vector<FileInfo> &out, const std::string &dirname, const std::string &ext)
{
	if(!fs::exists(dirname))
	{
		tlog1 << "Cannot find " << dirname << " directory!\n";
	}
	fs::path tie(dirname);
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator file (tie); file!=end_iter; ++file )
	{
		if(fs::is_regular_file(file->status())
			&& boost::ends_with(file->path().filename(), ext))
		{
			std::time_t date = 0;
			try
			{
				date = fs::last_write_time(file->path());

				out.resize(out.size()+1);
				out.back().date = date;
				out.back().name = file->path().string();
			}
			catch(...)
			{
				tlog2 << "\t\tWarning: very corrupted file: " << file->path().string() << std::endl; 
			}

		}
	}
}