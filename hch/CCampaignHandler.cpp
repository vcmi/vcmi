#define VCMI_DLL

#include "CCampaignHandler.h"
#include <boost/filesystem.hpp>
#include <stdio.h>
#include <boost/algorithm/string/predicate.hpp>
#include "CLodHandler.h"

namespace fs = boost::filesystem;

/*
 * CCampaignHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


std::vector<CCampaignHeader> CCampaignHandler::getCampaignHeaders()
{
	std::vector<CCampaignHeader> ret;

	std::string dirname = DATA_DIR "/Maps";
	std::string ext = ".h3c";

	if(!boost::filesystem::exists(dirname))
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
			ret.push_back( getHeader( file->path().string(), fs::file_size( file->path() ) ) );
		}
	}

	return ret;
}

CCampaignHeader CCampaignHandler::getHeader( const std::string & name, int size )
{
	int realSize;
	unsigned char * cmpgn = CLodHandler::getUnpackedFile(name, &realSize);

	CCampaignHeader ret;
	int it = 0;//iterator for reading
	ret.version = readNormalNr(cmpgn, it); it+=4;
	ret.mapVersion = readChar(cmpgn, it);
	ret.name = readString(cmpgn, it);
	ret.description = readString(cmpgn, it);
	ret.difficultyChoosenByPlayer = readChar(cmpgn, it);
	ret.music = readChar(cmpgn, it);

	delete [] cmpgn;

	return ret;
}