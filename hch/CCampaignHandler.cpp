#define VCMI_DLL

#include "CCampaignHandler.h"
#include <boost/filesystem.hpp>
#include <stdio.h>
#include <boost/algorithm/string/predicate.hpp>
#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include "CGeneralTextHandler.h"

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


std::vector<CCampaignHeader> CCampaignHandler::getCampaignHeaders(GetMode mode)
{
	std::vector<CCampaignHeader> ret;

	std::string dirname = DATA_DIR "/Maps";
	std::string ext = ".H3C";

	if(!boost::filesystem::exists(dirname))
	{
		tlog1 << "Cannot find " << dirname << " directory!\n";
	}

	if (mode == Custom || mode == ALL) //add custom campaigns
	{
		fs::path tie(dirname);
		fs::directory_iterator end_iter;
		for ( fs::directory_iterator file (tie); file!=end_iter; ++file )
		{
			if(fs::is_regular_file(file->status())
				&& boost::ends_with(file->path().filename(), ext))
			{
				ret.push_back( getHeader( file->path().string(), false ) );
			}
		}
	}
	if (mode == ALL) //add all lod campaigns
	{
		for(int g=0; g<bitmaph->entries.size(); ++g)
		{
			std::string rn = bitmaph->entries[g].nameStr;
			if( boost::ends_with(bitmaph->entries[g].nameStr, ext) )
			{
				ret.push_back( getHeader(bitmaph->entries[g].nameStr, true) );
			}
		}

	}


	return ret;
}

CCampaignHeader CCampaignHandler::getHeader( const std::string & name, bool fromLod )
{
	int realSize;
	unsigned char * cmpgn = CLodHandler::getUnpackedFile(name, &realSize);

	int it = 0;//iterator for reading
	CCampaignHeader ret = readHeaderFromMemory(cmpgn, it);
	ret.filename = name;
	ret.loadFromLod = false;

	delete [] cmpgn;

	return ret;
}

CCampaign * CCampaignHandler::getCampaign( const std::string & name, bool fromLod )
{
	CCampaign * ret = new CCampaign();

	int realSize;
	unsigned char * cmpgn;
	if (fromLod)
	{
		cmpgn = bitmaph->giveFile(name, &realSize);
	} 
	else
	{
		cmpgn = CLodHandler::getUnpackedFile(name, &realSize);
	}

	int it = 0; //iterator for reading
	ret->header = readHeaderFromMemory(cmpgn, it);
	ret->header.filename = name;
	ret->header.loadFromLod = fromLod;

	int howManyScenarios = VLC->generaltexth->campaignRegionNames[ret->header.mapVersion].size();
	for(int g=0; g<howManyScenarios; ++g)
	{
		CCampaignScenario sc = readScenarioFromMemory(cmpgn, it, ret->header.version, ret->header.mapVersion);
		ret->scenarios.push_back(sc);
	}

	std::vector<ui32> h3mStarts = locateH3mStarts(cmpgn, it, realSize);

	assert(h3mStarts.size() <= howManyScenarios);
	//it looks like we can have less scenarios than we should..
	//if(h3mStarts.size() != howManyScenarios)
	//{
	//	tlog1<<"Our heuristic for h3m start points gave wrong results for campaign " << name <<std::endl;
	//	tlog1<<"Please send this campaign to VCMI Project team to help us fix this problem" << std::endl;
	//	delete [] cmpgn;
	//	assert(0);
	//	return NULL;
	//}

	for (int g=0; g<h3mStarts.size(); ++g)
	{
		if(g == h3mStarts.size() - 1)
		{
			ret->mapPieces.push_back(std::string( cmpgn + h3mStarts[g], cmpgn + realSize ));
		}
		else
		{
			ret->mapPieces.push_back(std::string( cmpgn + h3mStarts[g], cmpgn + h3mStarts[g+1] ));
		}
	}

	delete [] cmpgn;

	return ret;
}

CCampaignHeader CCampaignHandler::readHeaderFromMemory( const unsigned char *buffer, int & outIt )
{
	CCampaignHeader ret;
	ret.version = readNormalNr(buffer, outIt); outIt+=4;
	ret.mapVersion = readChar(buffer, outIt);
	ret.mapVersion -= 1; //change range of it from [1, 20] to [0, 19]
	ret.name = readString(buffer, outIt);
	ret.description = readString(buffer, outIt);
	ret.difficultyChoosenByPlayer = readChar(buffer, outIt);
	ret.music = readChar(buffer, outIt);
	if(ret.version == 4)	//I saw one campaign with this version, without either difficulty or music - it's  
	{						//not editable by any editor so I'll just go back by one byte.
		outIt--;
	}

	return ret;
}

CCampaignScenario CCampaignHandler::readScenarioFromMemory( const unsigned char *buffer, int & outIt, int version, int mapVersion )
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
	ret.conquered = false;
	ret.mapName = readString(buffer, outIt);
	ret.packedMapSize = readNormalNr(buffer, outIt); outIt += 4;
	if(mapVersion == 18)//unholy alliance
	{
		ret.preconditionRegion = readNormalNr(buffer, outIt, 2); outIt += 2;
	}
	else
	{
		ret.preconditionRegion = buffer[outIt++];
	}
	ret.regionColor = buffer[outIt++];
	ret.difficulty = buffer[outIt++];
	ret.regionText = readString(buffer, outIt);
	ret.prolog = HLP::prologEpilogReader(buffer, outIt);
	ret.epilog = HLP::prologEpilogReader(buffer, outIt);

	ret.travelOptions = readScenarioTravelFromMemory(buffer, outIt, version);

	return ret;
}

CScenarioTravel CCampaignHandler::readScenarioTravelFromMemory( const unsigned char * buffer, int & outIt , int version )
{
	CScenarioTravel ret;

	ret.whatHeroKeeps = buffer[outIt++];
	memcpy(ret.monstersKeptByHero, buffer+outIt, ARRAY_COUNT(ret.monstersKeptByHero));
	outIt += ARRAY_COUNT(ret.monstersKeptByHero);
	int artifBytes;
	if (version == 5) //AB
	{
		artifBytes = 17;
		ret.artifsKeptByHero[17] = 0;
	} 
	else //SoD+
	{
		artifBytes = 18;
	}
	memcpy(ret.artifsKeptByHero, buffer+outIt, artifBytes);
	outIt += artifBytes;

	ret.startOptions = buffer[outIt++];
	
	switch(ret.startOptions)
	{
	case 0:
		//no bonuses. Seems to be OK
		break;
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
						bonus.info1 = buffer[outIt++]; //building ID (0 - town hall, 1 - city hall, 2 - capitol, etc)
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
						bonus.info2 = buffer[outIt++]; //spell ID
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

std::vector<ui32> CCampaignHandler::locateH3mStarts( const unsigned char * buffer, int start, int size )
{
	std::vector<ui32> ret;
	for(int g=start; g<size; ++g)
	{
		if(startsAt(buffer, size, g))
		{
			ret.push_back(g);
		}
	}

	return ret;
}

bool CCampaignHandler::startsAt( const unsigned char * buffer, int size, int pos )
{
	struct HLP
	{
		static unsigned char at(const unsigned char * buffer, int size, int place)
		{
			if(place < size)
				return buffer[place];

			throw std::string("Out of bounds!");
		}
	};
	try
	{
		//check minimal length of given region
		HLP::at(buffer, size, 100);
		//check version

		unsigned char tmp = HLP::at(buffer, size, pos);
		if(!(tmp == 0x0e || tmp == 0x15 || tmp == 0x1c || tmp == 0x33))
		{
			return false;
		}
		//3 bytes following version
		if(HLP::at(buffer, size, pos+1) != 0 || HLP::at(buffer, size, pos+2) != 0 || HLP::at(buffer, size, pos+3) != 0)
		{
			return false;
		}
		//unknown strange byte
		tmp = HLP::at(buffer, size, pos+4);
		if(tmp != 0 && tmp != 1 )
		{
			return false;
		}
		//size of map
		int mapsize = readNormalNr(buffer, pos+5);
		if(mapsize < 10 || mapsize > 530) 
		{
			return false;
		}

		//underground or not
		tmp = HLP::at(buffer, size, pos+9);
		if( tmp != 0 && tmp != 1 )
		{
			return false;
		}

		//map name
		int len = readNormalNr(buffer, pos+10);
		if(len < 0 || len > 100)
		{
			return false;
		}
		for(int t=0; t<len; ++t)
		{
			tmp = HLP::at(buffer, size, pos+14+t);
			if(tmp == 0 || (tmp > 15 && tmp < 32)) //not a valid character
			{
				return false;
			}
		}

	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool CCampaign::conquerable( int whichScenario ) const
{
	if (scenarios[whichScenario].conquered)
	{
		return false;
	}
	//check preconditioned regions
	for (int g=0; g<scenarios.size(); ++g)
	{
		if(( (1 << g) & scenarios[whichScenario].preconditionRegion ) && !scenarios[g].conquered)
			return false; //prerequisite does not met
			
	}
	return true;
}
