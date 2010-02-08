#ifndef __CCAMPAIGNHANDLER_H__
#define __CCAMPAIGNHANDLER_H__

#include "../global.h"
#include <string>
#include <vector>


class DLL_EXPORT CCampaignHeader
{

};


class DLL_EXPORT CCampaignHandler
{
	CCampaignHeader getHeader( const std::string & name, int size );
public:
	std::vector<CCampaignHeader> getCampaignHeaders();
};


#endif __CCAMPAIGNHANDLER_H__