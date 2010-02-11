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

	int it = 0;//iterator for reading
	CCampaignHeader ret = readHeaderFromMemory(cmpgn, it);

	delete [] cmpgn;

	return ret;
}

CCampaign * CCampaignHandler::getCampaign( const std::string & name )
{
	CCampaign * ret = new CCampaign();

	int realSize;
	unsigned char * cmpgn = CLodHandler::getUnpackedFile(name, &realSize);

	int it = 0; //iterator for reading
	ret->header = readHeaderFromMemory(cmpgn, it);

	it += 112; //omitting rubbish
	CCampaignScenario sc = readScenarioFromMemory(cmpgn, it);
	CCampaignScenario sc2 = readScenarioFromMemory(cmpgn, it);

	delete [] cmpgn;

	return ret;
}

CCampaignHeader CCampaignHandler::readHeaderFromMemory( const unsigned char *buffer, int & outIt )
{
	CCampaignHeader ret;
	ret.version = readNormalNr(buffer, outIt); outIt+=4;
	ret.mapVersion = readChar(buffer, outIt);
	ret.name = readString(buffer, outIt);
	ret.description = readString(buffer, outIt);
	ret.difficultyChoosenByPlayer = readChar(buffer, outIt);
	ret.music = readChar(buffer, outIt);

	return ret;
}

CCampaignScenario CCampaignHandler::readScenarioFromMemory( const unsigned char *buffer, int & outIt )
{
	struct HLP
	{
		//reads prolog/epilog info from memory
		static CCampaignScenario::SScenarioPrologEpilog prologEpilogReader( const unsigned char *buffer, int & outIt )
		{
			CCampaignScenario::SScenarioPrologEpilog ret;
			ret.hasPrologEpilog = buffer[outIt++];
			if(ret.hasPrologEpilog)
			{
				ret.prologVideo = buffer[outIt++];
				ret.prologMusic = buffer[outIt++];
				ret.prologText = readString(buffer, outIt);
			}
			return ret;
		}
	};
	CCampaignScenario ret;
	ret.mapName = readString(buffer, outIt);
	ret.packedMapSize = readNormalNr(buffer, outIt); outIt += 4;
	ret.preconditionRegion = buffer[outIt++];
	ret.regionColor = buffer[outIt++];
	ret.difficulty = buffer[outIt++];
	ret.regionText = readString(buffer, outIt);
	ret.prolog = HLP::prologEpilogReader(buffer, outIt);
	ret.epilog = HLP::prologEpilogReader(buffer, outIt);

	ret.travelOptions = readScenarioTravelFromMemory(buffer, outIt);

	return ret;
}

CScenarioTravel CCampaignHandler::readScenarioTravelFromMemory( const unsigned char * buffer, int & outIt )
{
	CScenarioTravel ret;

	ret.whatHeroKeeps = buffer[outIt++];
	memcpy(ret.monstersKeptByHero, buffer+outIt, ARRAY_COUNT(ret.monstersKeptByHero));
	outIt += ARRAY_COUNT(ret.monstersKeptByHero);
	memcpy(ret.artifsKeptByHero, buffer+outIt, ARRAY_COUNT(ret.artifsKeptByHero));
	outIt += ARRAY_COUNT(ret.artifsKeptByHero);

	ret.startOptions = buffer[outIt++];
	
	switch(ret.startOptions)
	{
	case 1: //reading of bonuses player can choose
		{
			ret.playerColor = buffer[outIt++];
			ui8 numOfBonuses = buffer[outIt++];
			for (int g=0; g<numOfBonuses; ++g)
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = buffer[outIt++];
				switch(bonus.type)
				{
				case 0: //spell
					{
						bonus.info1 = readNormalNr(buffer, outIt, 2); outIt += 2; //hero
						bonus.info2 = buffer[outIt++]; //spell ID
						break;
					}
				case 1: //monster
					{
						bonus.info1 = readNormalNr(buffer, outIt, 2); outIt += 2; //hero
						bonus.info2 = readNormalNr(buffer, outIt, 2); outIt += 2; //monster type
						bonus.info3 = readNormalNr(buffer, outIt, 2); outIt += 2; //monster count
						break;
					}
				case 2: //building
					{
						//TODO
						break;
					}
				case 3: //artifact
					{
						bonus.info1 = readNormalNr(buffer, outIt, 2); outIt += 2; //hero
						bonus.info2 = readNormalNr(buffer, outIt, 2); outIt += 2; //artifact ID
						break;
					}
				case 4: //spell scroll
					{
						bonus.info1 = readNormalNr(buffer, outIt, 2); outIt += 2; //hero
						bonus.info2 = readNormalNr(buffer, outIt, 2); outIt += 2; //spell ID
						break;
					}
				case 5: //prim skill
					{
						bonus.info1 = readNormalNr(buffer, outIt, 2); outIt += 2; //hero
						bonus.info2 = readNormalNr(buffer, outIt, 4); outIt += 4; //bonuses (4 bytes for 4 skills)
						break;
					}
				case 6: //sec skills
					{
						bonus.info1 = readNormalNr(buffer, outIt, 2); outIt += 2; //hero
						bonus.info2 = buffer[outIt++]; //skill ID
						bonus.info3 = buffer[outIt++]; //skill level
						break;
					}
				case 7: //resources
					{
						bonus.info1 = buffer[outIt++]; //type
						//FD - wood+ore
						//FE - mercury+sulfur+crystal+gem
						bonus.info2 = readNormalNr(buffer, outIt, 4); outIt += 4; //count
						break;
					}
				}
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case 2: //reading of players (colors / scenarios ?) player can choose
		{
			ui8 numOfBonuses = buffer[outIt++];
			for (int g=0; g<numOfBonuses; ++g)
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = 8;
				bonus.info1 = buffer[outIt++]; //player color
				bonus.info2 = buffer[outIt++]; //from what scenario

				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case 3: //heroes player can choose between
		{
			ui8 numOfBonuses = buffer[outIt++];
			for (int g=0; g<numOfBonuses; ++g)
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = 9;
				bonus.info1 = buffer[outIt++]; //player color
				bonus.info2 = readNormalNr(buffer, outIt, 2); outIt += 2; //hero, FF FF - random

				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	default:
		{
			tlog1<<"Corrupted h3c file"<<std::endl;
			break;
		}
	}

	return ret;
}