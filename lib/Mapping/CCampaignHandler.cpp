#include "StdInc.h"
#include "CCampaignHandler.h"

#include "../Filesystem/CResourceLoader.h"
#include "../Filesystem/CCompressedStream.h"
#include "../VCMI_Lib.h"
#include "../vcmi_endian.h"
#include "../CGeneralTextHandler.h"
#include "../StartInfo.h"
#include "../CArtHandler.h" //for hero crossover
#include "../CObjectHandler.h" //for hero crossover
#include "../CHeroHandler.h"

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

	int it = 0;//iterator for reading
	CCampaignHeader ret = readHeaderFromMemory(cmpgn.data(), it);
	ret.filename = name;

	return ret;
}

unique_ptr<CCampaign> CCampaignHandler::getCampaign( const std::string & name )
{
	auto ret = make_unique<CCampaign>();

	std::vector<std::vector<ui8>> file = getFile(name, false);

	int it = 0; //iterator for reading
	ret->header = readHeaderFromMemory(file[0].data(), it);
	ret->header.filename = name;

	int howManyScenarios = VLC->generaltexth->campaignRegionNames[ret->header.mapVersion].size();
	for(int g=0; g<howManyScenarios; ++g)
	{
		CCampaignScenario sc = readScenarioFromMemory(file[0].data(), it, ret->header.version, ret->header.mapVersion);
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

		//set map piece appropriately, convert vector to string
		ret->mapPieces[scenarioID].assign(reinterpret_cast< const char* >(file[g].data()), file[g].size());
		scenarioID++;
	}

	return ret;
}

CCampaignHeader CCampaignHandler::readHeaderFromMemory( const ui8 *buffer, int & outIt )
{
	CCampaignHeader ret;
	ret.version = read_le_u32(buffer + outIt); outIt+=4;
	ret.mapVersion = buffer[outIt++]; //1 byte only
	ret.mapVersion -= 1; //change range of it from [1, 20] to [0, 19]
	ret.name = readString(buffer, outIt);
	ret.description = readString(buffer, outIt);
	if (ret.version > CampaignVersion::RoE)
		ret.difficultyChoosenByPlayer = readChar(buffer, outIt);
	else
		ret.difficultyChoosenByPlayer = 0;
	ret.music = readChar(buffer, outIt);
	return ret;
}

CCampaignScenario CCampaignHandler::readScenarioFromMemory( const ui8 *buffer, int & outIt, int version, int mapVersion )
{
	struct HLP
	{
		//reads prolog/epilog info from memory
		static CCampaignScenario::SScenarioPrologEpilog prologEpilogReader( const ui8 *buffer, int & outIt )
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
	ret.packedMapSize = read_le_u32(buffer + outIt); outIt += 4;
	if(mapVersion == 18)//unholy alliance
	{
		ret.loadPreconditionRegions(read_le_u16(buffer + outIt)); outIt += 2;
	}
	else
	{
		ret.loadPreconditionRegions(buffer[outIt++]);
	}
	ret.regionColor = buffer[outIt++];
	ret.difficulty = buffer[outIt++];
	ret.regionText = readString(buffer, outIt);
	ret.prolog = HLP::prologEpilogReader(buffer, outIt);
	ret.epilog = HLP::prologEpilogReader(buffer, outIt);

	ret.travelOptions = readScenarioTravelFromMemory(buffer, outIt, version);

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

CScenarioTravel CCampaignHandler::readScenarioTravelFromMemory( const ui8 * buffer, int & outIt , int version )
{
	CScenarioTravel ret;

	ret.whatHeroKeeps = buffer[outIt++];
	memcpy(ret.monstersKeptByHero, buffer+outIt, ARRAY_COUNT(ret.monstersKeptByHero));
	outIt += ARRAY_COUNT(ret.monstersKeptByHero);
	int artifBytes;
	if (version < CampaignVersion::SoD)
	{
		artifBytes = 17;
		ret.artifsKeptByHero[17] = 0;
	} 
	else
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
				//hero: FFFD means 'most powerful' and FFFE means 'generated'
				switch(bonus.type)
				{
				case CScenarioTravel::STravelBonus::SPELL:
					{
						bonus.info1 = read_le_u16(buffer + outIt); outIt += 2; //hero
						bonus.info2 = buffer[outIt++]; //spell ID
						break;
					}
				case CScenarioTravel::STravelBonus::MONSTER:
					{
						bonus.info1 = read_le_u16(buffer + outIt); outIt += 2; //hero
						bonus.info2 = read_le_u16(buffer + outIt); outIt += 2; //monster type
						bonus.info3 = read_le_u16(buffer + outIt); outIt += 2; //monster count
						break;
					}
				case CScenarioTravel::STravelBonus::BUILDING:
					{
						bonus.info1 = buffer[outIt++]; //building ID (0 - town hall, 1 - city hall, 2 - capitol, etc)
						break;
					}
				case CScenarioTravel::STravelBonus::ARTIFACT:
					{
						bonus.info1 = read_le_u16(buffer + outIt); outIt += 2; //hero
						bonus.info2 = read_le_u16(buffer + outIt); outIt += 2; //artifact ID
						break;
					}
				case CScenarioTravel::STravelBonus::SPELL_SCROLL:
					{
						bonus.info1 = read_le_u16(buffer + outIt); outIt += 2; //hero
						bonus.info2 = buffer[outIt++]; //spell ID
						break;
					}
				case CScenarioTravel::STravelBonus::PRIMARY_SKILL:
					{
						bonus.info1 = read_le_u16(buffer + outIt); outIt += 2; //hero
						bonus.info2 = read_le_u32(buffer + outIt); outIt += 4; //bonuses (4 bytes for 4 skills)
						break;
					}
				case CScenarioTravel::STravelBonus::SECONDARY_SKILL:
					{
						bonus.info1 = read_le_u16(buffer + outIt); outIt += 2; //hero
						bonus.info2 = buffer[outIt++]; //skill ID
						bonus.info3 = buffer[outIt++]; //skill level
						break;
					}
				case CScenarioTravel::STravelBonus::RESOURCE:
					{
						bonus.info1 = buffer[outIt++]; //type
						//FD - wood+ore
						//FE - mercury+sulfur+crystal+gem
						bonus.info2 = read_le_u32(buffer + outIt); outIt += 4; //count
						break;
					}
				default:
					tlog1<<"Corrupted h3c file"<<std::endl;
					break;
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
				bonus.type = CScenarioTravel::STravelBonus::PLAYER_PREV_SCENARIO;
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
				bonus.type = CScenarioTravel::STravelBonus::HERO;
				bonus.info1 = buffer[outIt++]; //player color
				bonus.info2 = read_le_u16(buffer + outIt); outIt += 2; //hero, FF FF - random

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

std::vector< std::vector<ui8> > CCampaignHandler::getFile(const std::string & name, bool headerOnly)
{
	CCompressedStream stream(std::move(CResourceHandler::get()->load(ResourceID(name, EResType::CAMPAIGN))), true);

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

void CCampaignScenario::prepareCrossoverHeroes( std::vector<CGHeroInstance *> heroes )
{
	crossoverHeroes = heroes;


	if (!(travelOptions.whatHeroKeeps & 1))
	{
		//trimming experience
		BOOST_FOREACH(CGHeroInstance * cgh, crossoverHeroes)
		{
			cgh->initExp();
		}
	}
	if (!(travelOptions.whatHeroKeeps & 2))
	{
		//trimming prim skills
		BOOST_FOREACH(CGHeroInstance * cgh, crossoverHeroes)
		{
			for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
			{
				cgh->getBonusLocalFirst(Selector::type(Bonus::PRIMARY_SKILL) &&
					Selector::subtype(g) && Selector::sourceType(Bonus::HERO_BASE_SKILL) )->val
					= cgh->type->heroClass->primarySkillInitial[g];
			}
		}
	}
	if (!(travelOptions.whatHeroKeeps & 4))
	{
		//trimming sec skills
		BOOST_FOREACH(CGHeroInstance * cgh, crossoverHeroes)
		{
			cgh->secSkills = cgh->type->secSkillsInit;
		}
	}
	if (!(travelOptions.whatHeroKeeps & 8))
	{
		//trimming spells
		BOOST_FOREACH(CGHeroInstance * cgh, crossoverHeroes)
		{
			cgh->spells.clear();
		}
	}
	if (!(travelOptions.whatHeroKeeps & 16))
	{
		//trimming artifacts
		BOOST_FOREACH(CGHeroInstance * hero, crossoverHeroes)
		{
			size_t totalArts = GameConstants::BACKPACK_START + hero->artifactsInBackpack.size();
			for (size_t i=0; i<totalArts; i++ )
			{
				const ArtSlotInfo *info = hero->getSlot(i);
				if (!info)
					continue;

				const CArtifactInstance *art = info->artifact;
				if (!art)//FIXME: check spellbook and catapult behaviour
					continue;

				int id  = art->artType->id;
				assert( 8*18 > id );//number of arts that fits into h3m format
				bool takeable = travelOptions.artifsKeptByHero[id / 8] & ( 1 << (id%8) );

				if (!takeable)
					hero->eraseArtSlot(i);
			}
		}
	}

	//trimming creatures
	BOOST_FOREACH(CGHeroInstance * cgh, crossoverHeroes)
	{
		vstd::erase_if(cgh->stacks, [&](const std::pair<TSlot, CStackInstance *> & j)
		{
			return !(travelOptions.monstersKeptByHero[j.first / 8] & (1 << (j.first%8)) );
		});
	}
}

bool CScenarioTravel::STravelBonus::isBonusForHero() const
{
	return type == SPELL || type == MONSTER || type == ARTIFACT || type == SPELL_SCROLL || type == PRIMARY_SKILL
		|| type == SECONDARY_SKILL;
}

// void CCampaignState::initNewCampaign( const StartInfo &si )
// {
// 	assert(si.mode == StartInfo::CAMPAIGN);
// 	campaignName = si.mapname;
// 	currentMap = si.campState->currentMap;
// 
// 	camp = CCampaignHandler::getCampaign(campaignName);
// 	for (ui8 i = 0; i < camp->mapPieces.size(); i++)
// 		mapsRemaining.push_back(i);
// }

void CCampaignState::mapConquered( const std::vector<CGHeroInstance*> & heroes )
{
	camp->scenarios[currentMap].prepareCrossoverHeroes(heroes);
	mapsConquered.push_back(currentMap);
	mapsRemaining -= currentMap;
	camp->scenarios[currentMap].conquered = true;
}

boost::optional<CScenarioTravel::STravelBonus> CCampaignState::getBonusForCurrentMap() const
{
	auto bonuses = getCurrentScenario().travelOptions.bonusesToChoose;
	assert(chosenCampaignBonuses.count(currentMap) || bonuses.size() == 0);
	if(bonuses.size())
		return bonuses[currentBonusID()];
	else
		return boost::optional<CScenarioTravel::STravelBonus>();
}

const CCampaignScenario & CCampaignState::getCurrentScenario() const
{
	return camp->scenarios[currentMap];
}

ui8 CCampaignState::currentBonusID() const
{
	return chosenCampaignBonuses[currentMap];
}

CCampaignState::CCampaignState()
{}

CCampaignState::CCampaignState( unique_ptr<CCampaign> _camp ) : camp(std::move(_camp))
{
	for(int i = 0; i < camp->scenarios.size(); i++)
	{
		mapsRemaining.push_back(i);
	}
}
