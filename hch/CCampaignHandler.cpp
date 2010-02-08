#define VCMI_DLL

#include "CCampaignHandler.h"
#include <boost/filesystem.hpp>
#include <stdio.h>
#include <boost/algorithm/string/predicate.hpp>


namespace fs = boost::filesystem;


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
	FILE * input = fopen(name.c_str(), "rb");
	char * tab = new char[size];
	fread(tab, 1, size, input);

	CCampaignHeader ret;



	delete [] tab;

	return ret;
}