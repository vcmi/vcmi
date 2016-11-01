#include "StdInc.h"
#include "CCampaignHandler.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CCompressedStream.h"
#include "../filesystem/CMemoryStream.h"
#include "../filesystem/CBinaryReader.h"
#include "../VCMI_Lib.h"
#include "../vcmi_endian.h"
#include "../CGeneralTextHandler.h"
#include "../StartInfo.h"
#include "../CArtHandler.h" //for hero crossover
#include "../mapObjects/CGHeroInstance.h"//for hero crossover
#include "../CHeroHandler.h"
#include "CMapService.h"
#include "CMap.h"

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


CCampaignHeader CCampaignHandler::getHeader( const std::string & name)
{
	std::vector<ui8> cmpgn = getFile(name, true)[0];

	CMemoryStream stream(cmpgn.data(), cmpgn.size());
	CBinaryReader reader(&stream);
	CCampaignHeader ret = readHeaderFromMemory(reader);
	ret.filename = name;

	return ret;
}

std::unique_ptr<CCampaign> CCampaignHandler::getCampaign( const std::string & name )
{
	auto ret = make_unique<CCampaign>();

	std::vector<std::vector<ui8>> file = getFile(name, false);

	CMemoryStream stream(file[0].data(), file[0].size());
	CBinaryReader reader(&stream);
	ret->header = readHeaderFromMemory(reader);
	ret->header.filename = name;

	int howManyScenarios = VLC->generaltexth->campaignRegionNames[ret->header.mapVersion].size();
	for(int g=0; g<howManyScenarios; ++g)
	{
		CCampaignScenario sc = readScenarioFromMemory(reader, ret->header.version, ret->header.mapVersion);
		ret->scenarios.push_back(sc);
	}

	int scenarioID = 0;

	//first entry is campaign header. start loop from 1
	for (int g=1; g<file.size() && scenarioID<howManyScenarios; ++g)
	{
		while(!ret->scenarios[scenarioID].isNotVoid()) //skip void scenarios
		{
			scenarioID++;
		}

		std::string scenarioName = name.substr(0, name.find('.'));
		boost::to_lower(scenarioName);
		scenarioName += ':' + boost::lexical_cast<std::string>(g-1);

		//set map piece appropriately, convert vector to string
		ret->mapPieces[scenarioID].assign(reinterpret_cast< const char* >(file[g].data()), file[g].size());
		ret->scenarios[scenarioID].scenarioName = CMapService::loadMapHeader((const ui8*)ret->mapPieces[scenarioID].c_str(), ret->mapPieces[scenarioID].size(), scenarioName)->name;
		scenarioID++;
	}

	// handle campaign specific discrepancies
	if(name == "DATA/AB.H3C")
	{
		ret->scenarios[6].keepHeroes.push_back(HeroTypeID(155)); // keep hero Xeron for map 7 To Kill A Hero
	}
	else if(name == "DATA/FINAL.H3C")
	{
		// keep following heroes for map 8 Final H
		ret->scenarios[7].keepHeroes.push_back(HeroTypeID(148)); // Gelu
		ret->scenarios[7].keepHeroes.push_back(HeroTypeID(27)); // Gem
		ret->scenarios[7].keepHeroes.push_back(HeroTypeID(102)); // Crag Hack
		ret->scenarios[7].keepHeroes.push_back(HeroTypeID(96)); // Yog
	}

	return ret;
}

CCampaignHeader CCampaignHandler::readHeaderFromMemory( CBinaryReader & reader )
{
	CCampaignHeader ret;

	ret.version = reader.readUInt32();
	ret.mapVersion = reader.readUInt8() - 1;//change range of it from [1, 20] to [0, 19]
	ret.name = reader.readString();
	ret.description = reader.readString();
	if (ret.version > CampaignVersion::RoE)
		ret.difficultyChoosenByPlayer = reader.readInt8();
	else
		ret.difficultyChoosenByPlayer = 0;
	ret.music = reader.readInt8();
	return ret;
}

CCampaignScenario CCampaignHandler::readScenarioFromMemory( CBinaryReader & reader, int version, int mapVersion )
{
	auto prologEpilogReader = [&]() -> CCampaignScenario::SScenarioPrologEpilog
	{
		CCampaignScenario::SScenarioPrologEpilog ret;
		ret.hasPrologEpilog = reader.readUInt8();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = reader.readUInt8();
			ret.prologMusic = reader.readUInt8();
			ret.prologText = reader.readString();
		}
		return ret;
	};

	CCampaignScenario ret;
	ret.conquered = false;
	ret.mapName = reader.readString();
	ret.packedMapSize = reader.readUInt32();
	if(mapVersion == 18)//unholy alliance
	{
		ret.loadPreconditionRegions(reader.readUInt16());
	}
	else
	{
		ret.loadPreconditionRegions(reader.readUInt8());
	}
	ret.regionColor = reader.readUInt8();
	ret.difficulty = reader.readUInt8();
	ret.regionText = reader.readString();
	ret.prolog = prologEpilogReader();
	ret.epilog = prologEpilogReader();

	ret.travelOptions = readScenarioTravelFromMemory(reader, version);

	return ret;
}

void CCampaignScenario::loadPreconditionRegions(ui32 regions)
{
	for (int i=0; i<32; i++) //for each bit in region. h3c however can only hold up to 16
	{
		if ( (1 << i) & regions)
			preconditionRegions.insert(i);
	}
}

CScenarioTravel CCampaignHandler::readScenarioTravelFromMemory(CBinaryReader & reader, int version )
{
	CScenarioTravel ret;

	ret.whatHeroKeeps = reader.readUInt8();
	reader.getStream()->read(ret.monstersKeptByHero.data(), ret.monstersKeptByHero.size());

	if (version < CampaignVersion::SoD)
	{
		ret.artifsKeptByHero.fill(0);
		reader.getStream()->read(ret.artifsKeptByHero.data(), ret.artifsKeptByHero.size() - 1);
	} 
	else
	{
		reader.getStream()->read(ret.artifsKeptByHero.data(), ret.artifsKeptByHero.size());
	}

	ret.startOptions = reader.readUInt8();

	switch(ret.startOptions)
	{
	case 0:
		//no bonuses. Seems to be OK
		break;
	case 1: //reading of bonuses player can choose
		{
			ret.playerColor = reader.readUInt8();
			ui8 numOfBonuses = reader.readUInt8();
			for (int g=0; g<numOfBonuses; ++g)
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = static_cast<CScenarioTravel::STravelBonus::EBonusType>(reader.readUInt8());
				//hero: FFFD means 'most powerful' and FFFE means 'generated'
				switch(bonus.type)
				{
				case CScenarioTravel::STravelBonus::SPELL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt8(); //spell ID
						break;
					}
				case CScenarioTravel::STravelBonus::MONSTER:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt16(); //monster type
						bonus.info3 = reader.readUInt16(); //monster count
						break;
					}
				case CScenarioTravel::STravelBonus::BUILDING:
					{
						bonus.info1 = reader.readUInt8(); //building ID (0 - town hall, 1 - city hall, 2 - capitol, etc)
						break;
					}
				case CScenarioTravel::STravelBonus::ARTIFACT:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt16(); //artifact ID
						break;
					}
				case CScenarioTravel::STravelBonus::SPELL_SCROLL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt8(); //spell ID
						break;
					}
				case CScenarioTravel::STravelBonus::PRIMARY_SKILL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt32(); //bonuses (4 bytes for 4 skills)
						break;
					}
				case CScenarioTravel::STravelBonus::SECONDARY_SKILL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt8(); //skill ID
						bonus.info3 = reader.readUInt8(); //skill level
						break;
					}
				case CScenarioTravel::STravelBonus::RESOURCE:
					{
						bonus.info1 = reader.readUInt8(); //type
						//FD - wood+ore
						//FE - mercury+sulfur+crystal+gem
						bonus.info2 = reader.readUInt32(); //count
						break;
					}
				default:
					logGlobal->warnStream() << "Corrupted h3c file";
					break;
				}
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case 2: //reading of players (colors / scenarios ?) player can choose
		{
			ui8 numOfBonuses = reader.readUInt8();
			for (int g=0; g<numOfBonuses; ++g)
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = CScenarioTravel::STravelBonus::HEROES_FROM_PREVIOUS_SCENARIO;
				bonus.info1 = reader.readUInt8(); //player color
				bonus.info2 = reader.readUInt8(); //from what scenario

				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case 3: //heroes player can choose between
		{
			ui8 numOfBonuses = reader.readUInt8();
			for (int g=0; g<numOfBonuses; ++g)
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = CScenarioTravel::STravelBonus::HERO;
				bonus.info1 = reader.readUInt8(); //player color
				bonus.info2 = reader.readUInt16(); //hero, FF FF - random

				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	default:
		{
			logGlobal->warnStream() << "Corrupted h3c file";
			break;
		}
	}

	return ret;
}

std::vector< std::vector<ui8> > CCampaignHandler::getFile(const std::string & name, bool headerOnly)
{
	CCompressedStream stream(CResourceHandler::get()->load(ResourceID(name, EResType::CAMPAIGN)), true);

	std::vector< std::vector<ui8> > ret;
	do
	{
		std::vector<ui8> block(stream.getSize());
		stream.read(block.data(), block.size());
		ret.push_back(block);
	}
	while (!headerOnly && stream.getNextBlock());

	return ret;
}

bool CCampaign::conquerable( int whichScenario ) const
{
	//check for void scenraio
	if (!scenarios[whichScenario].isNotVoid())
	{
		return false;
	}

	if (scenarios[whichScenario].conquered)
	{
		return false;
	}
	//check preconditioned regions
	for (int g=0; g<scenarios.size(); ++g)
	{
		if( vstd::contains(scenarios[whichScenario].preconditionRegions, g) && !scenarios[g].conquered)
			return false; //prerequisite does not met
			
	}
	return true;
}

CCampaign::CCampaign()
{

}

bool CCampaignScenario::isNotVoid() const
{
	return mapName.size() > 0;
}

const CGHeroInstance * CCampaignScenario::strongestHero( PlayerColor owner ) const
{
	using boost::adaptors::filtered;
	std::function<bool(CGHeroInstance*)> isOwned =  [=](const CGHeroInstance *h){ return h->tempOwner == owner; };
	auto ownedHeroes = crossoverHeroes | filtered(isOwned);

	auto i = vstd::maxElementByFun(ownedHeroes,
									[](const CGHeroInstance * h) {return h->getHeroStrength();});
	return i == ownedHeroes.end() ? nullptr : *i;
}

std::vector<CGHeroInstance *> CCampaignScenario::getLostCrossoverHeroes() const
{
	std::vector<CGHeroInstance *> lostCrossoverHeroes;
	if(conquered)
	{
		for(auto hero : placedCrossoverHeroes)
		{
			auto it = range::find_if(crossoverHeroes, CGObjectInstanceBySubIdFinder(hero));
			if(it == crossoverHeroes.end())
			{
				lostCrossoverHeroes.push_back(hero);
			}
		}
	}
	return lostCrossoverHeroes;
}

bool CScenarioTravel::STravelBonus::isBonusForHero() const
{
	return type == SPELL || type == MONSTER || type == ARTIFACT || type == SPELL_SCROLL || type == PRIMARY_SKILL
		|| type == SECONDARY_SKILL;
}

void CCampaignState::setCurrentMapAsConquered( const std::vector<CGHeroInstance*> & heroes )
{
	camp->scenarios[*currentMap].crossoverHeroes = heroes;
	mapsConquered.push_back(*currentMap);
	mapsRemaining -= *currentMap;
	camp->scenarios[*currentMap].conquered = true;
}

boost::optional<CScenarioTravel::STravelBonus> CCampaignState::getBonusForCurrentMap() const
{
	auto bonuses = getCurrentScenario().travelOptions.bonusesToChoose;
	assert(chosenCampaignBonuses.count(*currentMap) || bonuses.size() == 0);

	if(bonuses.empty())
		return boost::optional<CScenarioTravel::STravelBonus>();
	else
		return bonuses[currentBonusID()];
}

const CCampaignScenario & CCampaignState::getCurrentScenario() const
{
	return camp->scenarios[*currentMap];
}

CCampaignScenario & CCampaignState::getCurrentScenario()
{
	return camp->scenarios[*currentMap];
}

ui8 CCampaignState::currentBonusID() const
{
	return chosenCampaignBonuses.at(*currentMap);
}

CCampaignState::CCampaignState()
{

}

CCampaignState::CCampaignState( std::unique_ptr<CCampaign> _camp ) : camp(std::move(_camp))
{
	for(int i = 0; i < camp->scenarios.size(); i++)
	{
		if(vstd::contains(camp->mapPieces, i)) //not all maps must be present in a campaign
			mapsRemaining.push_back(i);
	}
}

std::string CCampaignHandler::prologVideoName(ui8 index)
{
	JsonNode config(ResourceID(std::string("CONFIG/campaignMedia"), EResType::TEXT));
	auto vids = config["videos"].Vector();
	if(index < vids.size())
		return vids[index].String();
	return "";
}

std::string CCampaignHandler::prologMusicName(ui8 index)
{
	std::vector<std::string> music;

	VLC->generaltexth->readToVector("Data/CmpMusic.txt", music);
	if(index < music.size())
		return music[index];
	return "";
}

std::string CCampaignHandler::prologVoiceName(ui8 index)
{
	JsonNode config(ResourceID(std::string("CONFIG/campaignMedia"), EResType::TEXT));
	auto audio = config["voice"].Vector();
	if(index < audio.size())
		return audio[index].String();
	return "";
}



