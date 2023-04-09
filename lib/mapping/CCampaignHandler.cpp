/*
 * CCampaignHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCampaignHandler.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CCompressedStream.h"
#include "../filesystem/CMemoryStream.h"
#include "../filesystem/CBinaryReader.h"
#include "../VCMI_Lib.h"
#include "../vcmi_endian.h"
#include "../CGeneralTextHandler.h"
#include "../TextOperations.h"
#include "../StartInfo.h"
#include "../CModHandler.h"
#include "../CArtHandler.h" //for hero crossover
#include "../mapObjects/CGHeroInstance.h"//for hero crossover
#include "../CHeroHandler.h"
#include "../Languages.h"
#include "CMapService.h"
#include "CMap.h"
#include "CMapInfo.h"

// For hero crossover
#include "serializer/CSerializer.h"
#include "serializer/JsonDeserializer.h"
#include "serializer/JsonSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CScenarioTravel::STravelBonus::isBonusForHero() const
{
	return type == SPELL || type == MONSTER || type == ARTIFACT || type == SPELL_SCROLL || type == PRIMARY_SKILL
		|| type == SECONDARY_SKILL;
}

CCampaignHeader CCampaignHandler::getHeader( const std::string & name)
{
	ResourceID resourceID(name, EResType::CAMPAIGN);
	std::string modName = VLC->modh->findResourceOrigin(resourceID);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;
	
	JsonNode jsonCampaign(resourceID);
	if(jsonCampaign.isNull())
	{
		auto fileStream = CResourceHandler::get(modName)->load(resourceID);
		std::vector<ui8> cmpgn = getFile(std::move(fileStream), true)[0];
		CMemoryStream stream(cmpgn.data(), cmpgn.size());
		CBinaryReader reader(&stream);
		CCampaignHeader ret = readHeaderFromMemory(reader, resourceID.getName(), modName, encoding);
		return ret;
	}
	else
	{
		CCampaignHeader ret = readHeaderFromJson(jsonCampaign, resourceID.getName(), modName, encoding);
		return ret;
	}
}

std::unique_ptr<CCampaign> CCampaignHandler::getCampaign( const std::string & name )
{
	ResourceID resourceID(name, EResType::CAMPAIGN);
	std::string modName = VLC->modh->findResourceOrigin(resourceID);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;
	
	auto ret = std::make_unique<CCampaign>();
	
	JsonNode jsonCampaign(resourceID);
	if(jsonCampaign.isNull())
	{
		auto fileStream = CResourceHandler::get(modName)->load(resourceID);

		std::vector<std::vector<ui8>> file = getFile(std::move(fileStream), false);

		CMemoryStream stream(file[0].data(), file[0].size());
		CBinaryReader reader(&stream);
		ret->header = readHeaderFromMemory(reader, resourceID.getName(), modName, encoding);

		int howManyScenarios = static_cast<int>(VLC->generaltexth->getCampaignLength(ret->header.mapVersion));
		for(int g=0; g<howManyScenarios; ++g)
		{
			CCampaignScenario sc = readScenarioFromMemory(reader, resourceID.getName(), modName, encoding, ret->header.version, ret->header.mapVersion);
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

			std::string scenarioName = resourceID.getName();
			boost::to_lower(scenarioName);
			scenarioName += ':' + std::to_string(g - 1);

			//set map piece appropriately, convert vector to string
			ret->mapPieces[scenarioID].assign(reinterpret_cast< const char* >(file[g].data()), file[g].size());
			CMapService mapService;
			auto hdr = mapService.loadMapHeader(
				reinterpret_cast<const ui8 *>(ret->mapPieces[scenarioID].c_str()),
				static_cast<int>(ret->mapPieces[scenarioID].size()),
				scenarioName,
				modName,
				encoding);
			ret->scenarios[scenarioID].scenarioName = hdr->name;
			scenarioID++;
		}
	}
	else
	{
		ret->header = readHeaderFromJson(jsonCampaign, resourceID.getName(), modName, encoding);
		
		for(auto & scenario : jsonCampaign["scenarios"].Vector())
		{
			CCampaignScenario sc = readScenarioFromJson(scenario, resourceID.getName(), modName, encoding, ret->header.version, ret->header.mapVersion);
			if(sc.isNotVoid())
			{
				CMapService mapService;
				auto hdr = mapService.loadMapHeader(ResourceID(sc.mapName, EResType::MAP));
				sc.scenarioName = hdr->name;
			}
			ret->scenarios.push_back(sc);
		}
	}

	// handle campaign specific discrepancies
	if(resourceID.getName() == "DATA/AB")
	{
		ret->scenarios[6].keepHeroes.emplace_back(155); // keep hero Xeron for map 7 To Kill A Hero
	}
	else if(resourceID.getName() == "DATA/FINAL")
	{
		// keep following heroes for map 8 Final H
		ret->scenarios[7].keepHeroes.emplace_back(148); // Gelu
		ret->scenarios[7].keepHeroes.emplace_back(27); // Gem
		ret->scenarios[7].keepHeroes.emplace_back(102); // Crag Hack
		ret->scenarios[7].keepHeroes.emplace_back(96); // Yog
	}

	return ret;
}

static std::string convertMapName(std::string input)
{
	boost::algorithm::to_lower(input);
	boost::algorithm::trim(input);

	size_t slashPos = input.find_last_of("/");

	if (slashPos != std::string::npos)
		return input.substr(slashPos + 1);

	return input;
}

std::string CCampaignHandler::readLocalizedString(CBinaryReader & reader, std::string filename, std::string modName, std::string encoding, std::string identifier)
{
	TextIdentifier stringID( "campaign", convertMapName(filename), identifier);

	std::string input = TextOperations::toUnicode(reader.readBaseString(), encoding);

	if (input.empty())
		return "";

	VLC->generaltexth->registerString(modName, stringID, input);
	return VLC->generaltexth->translate(stringID.get());
}

CCampaignHeader CCampaignHandler::readHeaderFromJson(JsonNode & reader, std::string filename, std::string modName, std::string encoding )
{
	CCampaignHeader ret;

	ret.version = reader["version"].Integer();
	if(ret.version < CampaignVersion::VCMI_MIN || ret.version > CampaignVersion::VCMI_MAX)
	{
		logGlobal->info("Unsupported campaign %s version %d", filename, ret.version);
		return ret;
	}
	
	ret.version = CampaignVersion::VCMI;
	ret.mapVersion = reader["campaignId"].Integer();
	ret.name = reader["name"].String();
	ret.description = reader["description"].String();
	ret.difficultyChoosenByPlayer = reader["allowDifficultySelection"].Bool();
	//skip ret.music because it's unused in vcmi
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
	ret.valid = true;
	return ret;
}

CCampaignScenario CCampaignHandler::readScenarioFromJson(JsonNode & reader, std::string filename, std::string modName, std::string encoding, int version, int mapVersion)
{
	auto prologEpilogReader = [](JsonNode & identifier) -> CCampaignScenario::SScenarioPrologEpilog
	{
		CCampaignScenario::SScenarioPrologEpilog ret;
		ret.hasPrologEpilog = !identifier.isNull();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = identifier["video"].Integer();
			ret.prologMusic = identifier["music"].Integer();
			ret.prologText = identifier["text"].String();
		}
		return ret;
	};

	CCampaignScenario ret;
	ret.conquered = false;
	ret.mapName = reader["map"].String();
	for(auto & g : reader["precoditions"].Vector())
		ret.preconditionRegions.insert(g.Integer());

	ret.regionColor = reader["color"].Integer();
	ret.difficulty = reader["difficulty"].Integer();
	ret.regionText = reader["regionText"].String();
	ret.prolog = prologEpilogReader(reader["prolog"]);
	ret.epilog = prologEpilogReader(reader["epilog"]);

	ret.travelOptions = readScenarioTravelFromJson(reader, version);

	return ret;
}

CScenarioTravel CCampaignHandler::readScenarioTravelFromJson(JsonNode & reader, int version )
{
	CScenarioTravel ret;

	std::map<std::string, ui8> heroKeepsMap = {
		{"experience", 1},
		{"primary", 2},
		{"skills", 4},
		{"spells", 8},
		{"artifacts", 16}
	};
	std::map<std::string, ui8> startOptionsMap = {
		{"none", 0},
		{"bonus", 1},
		{"crossover", 2},
		{"hero", 3}
	};
	
	std::map<std::string, CScenarioTravel::STravelBonus::EBonusType> bonusTypeMap = {
		{"spell", CScenarioTravel::STravelBonus::EBonusType::SPELL},
		{"creature", CScenarioTravel::STravelBonus::EBonusType::MONSTER},
		{"building", CScenarioTravel::STravelBonus::EBonusType::BUILDING},
		{"artifact", CScenarioTravel::STravelBonus::EBonusType::ARTIFACT},
		{"scroll", CScenarioTravel::STravelBonus::EBonusType::SPELL_SCROLL},
		{"primarySkill", CScenarioTravel::STravelBonus::EBonusType::PRIMARY_SKILL},
		{"secondarySkill", CScenarioTravel::STravelBonus::EBonusType::SECONDARY_SKILL},
		{"resource", CScenarioTravel::STravelBonus::EBonusType::RESOURCE},
		//{"prevHero", CScenarioTravel::STravelBonus::EBonusType::HEROES_FROM_PREVIOUS_SCENARIO},
		//{"hero", CScenarioTravel::STravelBonus::EBonusType::HERO},
	};
	
	std::map<std::string, ui32> primarySkillsMap = {
		{"attack", 0},
		{"defence", 8},
		{"spellpower", 16},
		{"knowledge", 24},
	};
	
	std::map<std::string, ui16> heroSpecialMap = {
		{"strongest", 0xFFFD},
		{"generated", 0xFFFE},
		{"random", 0xFFFF}
	};
	
	std::map<std::string, ui8> resourceTypeMap = {
		//FD - wood+ore
		//FE - mercury+sulfur+crystal+gem
		{"wood", 0},
		{"mercury", 1},
		{"ore", 2},
		{"sulfur", 3},
		{"crystal", 4},
		{"gems", 5},
		{"gold", 6},
		{"common", 0xFD},
		{"rare", 0xFE}
	};
	
	for(auto & k : reader["heroKeeps"].Vector())
		ret.whatHeroKeeps |= heroKeepsMap[k.String()];
	
	//reader.getStream()->read(ret.monstersKeptByHero.data(), ret.monstersKeptByHero.size());
	//reader.getStream()->read(ret.artifsKeptByHero.data(), ret.artifsKeptByHero.size());

	ret.startOptions = startOptionsMap[reader["startOptions"].String()];

	switch(ret.startOptions)
	{
	case 0:
		//no bonuses. Seems to be OK
		break;
	case 1: //reading of bonuses player can choose
		{
			ret.playerColor = reader["playerColor"].Integer();
			for(auto & bjson : reader["bonuses"].Vector())
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = bonusTypeMap[bjson["what"].String()];
				if(bonus.type == CScenarioTravel::STravelBonus::EBonusType::RESOURCE)
				{
					bonus.info1 = resourceTypeMap[bjson["type"].String()];
					bonus.info2 = bjson["amount"].Integer();
				}
				else if(bonus.type == CScenarioTravel::STravelBonus::EBonusType::BUILDING)
					bonus.info1 = bjson["type"].Integer(); //VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "buildings", bjson["hero"].String()).get();
				else
				{
					if(int heroId = heroSpecialMap[bjson["hero"].String()])
						bonus.info1 = heroId;
					else
						bonus.info1 = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "hero", bjson["hero"].String()).get();
					
					if(bonus.type == CScenarioTravel::STravelBonus::EBonusType::SPELL
					   || bonus.type == CScenarioTravel::STravelBonus::EBonusType::MONSTER
					   || bonus.type == CScenarioTravel::STravelBonus::EBonusType::SECONDARY_SKILL
					   || bonus.type == CScenarioTravel::STravelBonus::EBonusType::ARTIFACT)
						bonus.info2 = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), bjson["what"].String(), bjson["type"].String()).get();
					else if(bonus.type == CScenarioTravel::STravelBonus::EBonusType::SPELL_SCROLL)
						bonus.info2 = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "spell", bjson["type"].String()).get();
					else if(bonus.type == CScenarioTravel::STravelBonus::EBonusType::PRIMARY_SKILL)
					{
						for(auto & ps : primarySkillsMap)
							bonus.info2 |= bjson[ps.first].Integer() << ps.second;
					}
					else
						bonus.info2 = bjson["type"].Integer();
					
					bonus.info3 = bjson["amount"].Integer();
				}
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case 2: //reading of players (colors / scenarios ?) player can choose
		{
			for(auto & bjson : reader["bonuses"].Vector())
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = CScenarioTravel::STravelBonus::HEROES_FROM_PREVIOUS_SCENARIO;
				bonus.info1 = bjson["playerColor"].Integer(); //player color
				bonus.info2 = bjson["scenario"].Integer(); //from what scenario
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case 3: //heroes player can choose between
		{
			for(auto & bjson : reader["bonuses"].Vector())
			{
				CScenarioTravel::STravelBonus bonus;
				bonus.type = CScenarioTravel::STravelBonus::HERO;
				bonus.info1 = bjson["playerColor"].Integer(); //player color
				
				if(int heroId = heroSpecialMap[bjson["hero"].String()])
					bonus.info2 = heroId;
				else
					bonus.info2 = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "hero", bjson["hero"].String()).get();
			
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	default:
		{
			logGlobal->warn("Corrupted h3c file");
			break;
		}
	}

	return ret;
}


CCampaignHeader CCampaignHandler::readHeaderFromMemory( CBinaryReader & reader, std::string filename, std::string modName, std::string encoding )
{
	CCampaignHeader ret;

	ret.version = reader.readUInt32();
	ret.mapVersion = reader.readUInt8() - 1;//change range of it from [1, 20] to [0, 19]
	ret.name = readLocalizedString(reader, filename, modName, encoding, "name");
	ret.description = readLocalizedString(reader, filename, modName, encoding, "description");
	if (ret.version > CampaignVersion::RoE)
		ret.difficultyChoosenByPlayer = reader.readInt8();
	else
		ret.difficultyChoosenByPlayer = 0;
	ret.music = reader.readInt8();
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
	ret.valid = true;
	return ret;
}

CCampaignScenario CCampaignHandler::readScenarioFromMemory( CBinaryReader & reader, std::string filename, std::string modName, std::string encoding, int version, int mapVersion )
{
	auto prologEpilogReader = [&](const std::string & identifier) -> CCampaignScenario::SScenarioPrologEpilog
	{
		CCampaignScenario::SScenarioPrologEpilog ret;
		ret.hasPrologEpilog = reader.readUInt8();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = reader.readUInt8();
			ret.prologMusic = reader.readUInt8();
			ret.prologText = readLocalizedString(reader, filename, modName, encoding, identifier);
		}
		return ret;
	};

	CCampaignScenario ret;
	ret.conquered = false;
	ret.mapName = reader.readBaseString();
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
	ret.regionText = readLocalizedString(reader, filename, modName, encoding, ret.mapName + ".region");
	ret.prolog = prologEpilogReader(ret.mapName + ".prolog");
	ret.epilog = prologEpilogReader(ret.mapName + ".epilog");

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
					logGlobal->warn("Corrupted h3c file");
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
			logGlobal->warn("Corrupted h3c file");
			break;
		}
	}

	return ret;
}

std::vector< std::vector<ui8> > CCampaignHandler::getFile(std::unique_ptr<CInputStream> file, bool headerOnly)
{
	CCompressedStream stream(std::move(file), true);

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

bool CCampaignScenario::isNotVoid() const
{
	return !mapName.empty();
}

const CGHeroInstance * CCampaignScenario::strongestHero(const PlayerColor & owner)
{
	std::function<bool(JsonNode & node)> isOwned = [owner](JsonNode & node)
	{
		auto * h = CCampaignState::crossoverDeserialize(node);
		bool result = h->tempOwner == owner;
		vstd::clear_pointer(h);
		return result;
	};
	auto ownedHeroes = crossoverHeroes | boost::adaptors::filtered(isOwned);

	auto i = vstd::maxElementByFun(ownedHeroes, [](JsonNode & node)
	{
		auto * h = CCampaignState::crossoverDeserialize(node);
		double result = h->getHeroStrength();
		vstd::clear_pointer(h);
		return result;
	});
	return i == ownedHeroes.end() ? nullptr : CCampaignState::crossoverDeserialize(*i);
}

std::vector<CGHeroInstance *> CCampaignScenario::getLostCrossoverHeroes()
{
	std::vector<CGHeroInstance *> lostCrossoverHeroes;
	if(conquered)
	{
		for(auto node2 : placedCrossoverHeroes)
		{
			auto * hero = CCampaignState::crossoverDeserialize(node2);
			auto it = range::find_if(crossoverHeroes, [hero](JsonNode node)
			{
				auto * h = CCampaignState::crossoverDeserialize(node);
				bool result = hero->subID == h->subID;
				vstd::clear_pointer(h);
				return result;
			});
			if(it == crossoverHeroes.end())
			{
				lostCrossoverHeroes.push_back(hero);
			}
		}
	}
	return lostCrossoverHeroes;
}

void CCampaignState::setCurrentMapAsConquered(const std::vector<CGHeroInstance *> & heroes)
{
	camp->scenarios[*currentMap].crossoverHeroes.clear();
	for(CGHeroInstance * hero : heroes)
	{
		camp->scenarios[*currentMap].crossoverHeroes.push_back(crossoverSerialize(hero));
	}

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

CCampaignState::CCampaignState( std::unique_ptr<CCampaign> _camp ) : camp(std::move(_camp))
{
	for(int i = 0; i < camp->scenarios.size(); i++)
	{
		if(vstd::contains(camp->mapPieces, i)) //not all maps must be present in a campaign
			mapsRemaining.push_back(i);
	}
}

CMap * CCampaignState::getMap(int scenarioId) const
{
	// FIXME: there is certainly better way to handle maps inside campaigns
	if(scenarioId == -1)
		scenarioId = currentMap.get();
	
	CMapService mapService;
	if(camp->header.version == CampaignVersion::Version::VCMI)
		return mapService.loadMap(ResourceID(camp->scenarios.at(scenarioId).mapName, EResType::MAP)).release();
	
	std::string scenarioName = camp->header.filename.substr(0, camp->header.filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(scenarioId);
	std::string & mapContent = camp->mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMap(buffer, static_cast<int>(mapContent.size()), scenarioName, camp->header.modName, camp->header.encoding).release();
}

std::unique_ptr<CMapHeader> CCampaignState::getHeader(int scenarioId) const
{
	if(scenarioId == -1)
		scenarioId = currentMap.get();
	
	CMapService mapService;
	if(camp->header.version == CampaignVersion::Version::VCMI)
		return mapService.loadMapHeader(ResourceID(camp->scenarios.at(scenarioId).mapName, EResType::MAP));

	std::string scenarioName = camp->header.filename.substr(0, camp->header.filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(scenarioId);
	std::string & mapContent = camp->mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMapHeader(buffer, static_cast<int>(mapContent.size()), scenarioName, camp->header.modName, camp->header.encoding);
}

std::shared_ptr<CMapInfo> CCampaignState::getMapInfo(int scenarioId) const
{
	if(scenarioId == -1)
		scenarioId = currentMap.get();

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->fileURI = camp->header.filename;
	mapInfo->mapHeader = getHeader(scenarioId);
	mapInfo->countPlayers();
	return mapInfo;
}

JsonNode CCampaignState::crossoverSerialize(CGHeroInstance * hero)
{
	JsonNode node;
	JsonSerializer handler(nullptr, node);
	hero->serializeJsonOptions(handler);
	return node;
}

CGHeroInstance * CCampaignState::crossoverDeserialize(JsonNode & node)
{
	JsonDeserializer handler(nullptr, node);
	auto * hero = new CGHeroInstance();
	hero->ID = Obj::HERO;
	hero->serializeJsonOptions(handler);
	return hero;
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
	return VLC->generaltexth->translate("core.cmpmusic." + std::to_string(static_cast<int>(index)));
}

std::string CCampaignHandler::prologVoiceName(ui8 index)
{
	JsonNode config(ResourceID(std::string("CONFIG/campaignMedia"), EResType::TEXT));
	auto audio = config["voice"].Vector();
	if(index < audio.size())
		return audio[index].String();
	return "";
}

VCMI_LIB_NAMESPACE_END
