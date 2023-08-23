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
#include "CampaignHandler.h"

#include "CampaignState.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CCompressedStream.h"
#include "../filesystem/CMemoryStream.h"
#include "../filesystem/CBinaryReader.h"
#include "../modding/IdentifierStorage.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../TextOperations.h"
#include "../Languages.h"
#include "../constants/StringConstants.h"
#include "../mapping/CMapHeader.h"
#include "../mapping/CMapService.h"
#include "../modding/CModHandler.h"
#include "../modding/ModScope.h"

VCMI_LIB_NAMESPACE_BEGIN

void CampaignHandler::readCampaign(Campaign * ret, const std::vector<ui8> & input, std::string filename, std::string modName, std::string encoding)
{
	if (input.front() < uint8_t(' ')) // binary format
	{
		CMemoryStream stream(input.data(), input.size());
		CBinaryReader reader(&stream);

		readHeaderFromMemory(*ret, reader, filename, modName, encoding);

		for(int g = 0; g < ret->numberOfScenarios; ++g)
		{
			auto scenarioID = static_cast<CampaignScenarioID>(ret->scenarios.size());
			ret->scenarios[scenarioID] = readScenarioFromMemory(reader, *ret);
		}
	}
	else // text format (json)
	{
		JsonNode jsonCampaign((const char*)input.data(), input.size());
		readHeaderFromJson(*ret, jsonCampaign, filename, modName, encoding);

		for(auto & scenario : jsonCampaign["scenarios"].Vector())
		{
			auto scenarioID = static_cast<CampaignScenarioID>(ret->scenarios.size());
			ret->scenarios[scenarioID] = readScenarioFromJson(scenario);
		}
	}
}

std::unique_ptr<Campaign> CampaignHandler::getHeader( const std::string & name)
{
	ResourcePath resourceID(name, EResType::CAMPAIGN);
	std::string modName = VLC->modh->findResourceOrigin(resourceID);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;
	
	auto ret = std::make_unique<Campaign>();
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);
	std::vector<ui8> cmpgn = getFile(std::move(fileStream), true)[0];

	readCampaign(ret.get(), cmpgn, resourceID.getName(), modName, encoding);

	return ret;
}

std::shared_ptr<CampaignState> CampaignHandler::getCampaign( const std::string & name )
{
	ResourcePath resourceID(name, EResType::CAMPAIGN);
	std::string modName = VLC->modh->findResourceOrigin(resourceID);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;
	
	auto ret = std::make_unique<CampaignState>();
	
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);

	std::vector<std::vector<ui8>> files = getFile(std::move(fileStream), false);

	readCampaign(ret.get(), files[0], resourceID.getName(), modName, encoding);

	//first entry is campaign header. start loop from 1
	for(int scenarioIndex = 0, fileIndex = 1; fileIndex < files.size() && scenarioIndex < ret->numberOfScenarios; scenarioIndex++)
	{
		auto scenarioID = static_cast<CampaignScenarioID>(scenarioIndex);

		if(!ret->scenarios[scenarioID].isNotVoid()) //skip void scenarios
			continue;

		std::string scenarioName = resourceID.getName();
		boost::to_lower(scenarioName);
		scenarioName += ':' + std::to_string(fileIndex - 1);

		//set map piece appropriately, convert vector to string
		ret->mapPieces[scenarioID] = files[fileIndex];

		auto hdr = ret->getMapHeader(scenarioID);
		ret->scenarios[scenarioID].scenarioName = hdr->name;
		fileIndex++;
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

std::string CampaignHandler::readLocalizedString(CBinaryReader & reader, std::string filename, std::string modName, std::string encoding, std::string identifier)
{
	TextIdentifier stringID( "campaign", convertMapName(filename), identifier);

	std::string input = TextOperations::toUnicode(reader.readBaseString(), encoding);

	if (input.empty())
		return "";

	VLC->generaltexth->registerString(modName, stringID, input);
	return VLC->generaltexth->translate(stringID.get());
}

void CampaignHandler::readHeaderFromJson(CampaignHeader & ret, JsonNode & reader, std::string filename, std::string modName, std::string encoding)
{
	ret.version = static_cast<CampaignVersion>(reader["version"].Integer());
	if(ret.version < CampaignVersion::VCMI_MIN || ret.version > CampaignVersion::VCMI_MAX)
	{
		logGlobal->info("VCMP Loading: Unsupported campaign %s version %d", filename, static_cast<int>(ret.version));
		return;
	}
	
	ret.version = CampaignVersion::VCMI;
	ret.campaignRegions = CampaignRegions::fromJson(reader["regions"]);
	ret.numberOfScenarios = reader["scenarios"].Vector().size();
	ret.name = reader["name"].String();
	ret.description = reader["description"].String();
	ret.difficultyChoosenByPlayer = reader["allowDifficultySelection"].Bool();
	ret.music = reader["music"].String();
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
}

CampaignScenario CampaignHandler::readScenarioFromJson(JsonNode & reader)
{
	auto prologEpilogReader = [](JsonNode & identifier) -> CampaignScenarioPrologEpilog
	{
		CampaignScenarioPrologEpilog ret;
		ret.hasPrologEpilog = !identifier.isNull();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = identifier["video"].String();
			ret.prologMusic = identifier["music"].String();
			ret.prologText = identifier["text"].String();
		}
		return ret;
	};

	CampaignScenario ret;
	ret.mapName = reader["map"].String();
	for(auto & g : reader["preconditions"].Vector())
		ret.preconditionRegions.insert(static_cast<CampaignScenarioID>(g.Integer()));

	ret.regionColor = reader["color"].Integer();
	ret.difficulty = reader["difficulty"].Integer();
	ret.regionText = reader["regionText"].String();
	ret.prolog = prologEpilogReader(reader["prolog"]);
	ret.epilog = prologEpilogReader(reader["epilog"]);

	ret.travelOptions = readScenarioTravelFromJson(reader);

	return ret;
}

CampaignTravel CampaignHandler::readScenarioTravelFromJson(JsonNode & reader)
{
	CampaignTravel ret;

	std::map<std::string, CampaignStartOptions> startOptionsMap = {
		{"none", CampaignStartOptions::NONE},
		{"bonus", CampaignStartOptions::START_BONUS},
		{"crossover", CampaignStartOptions::HERO_CROSSOVER},
		{"hero", CampaignStartOptions::HERO_OPTIONS}
	};
	
	std::map<std::string, CampaignBonusType> bonusTypeMap = {
		{"spell", CampaignBonusType::SPELL},
		{"creature", CampaignBonusType::MONSTER},
		{"building", CampaignBonusType::BUILDING},
		{"artifact", CampaignBonusType::ARTIFACT},
		{"scroll", CampaignBonusType::SPELL_SCROLL},
		{"primarySkill", CampaignBonusType::PRIMARY_SKILL},
		{"secondarySkill", CampaignBonusType::SECONDARY_SKILL},
		{"resource", CampaignBonusType::RESOURCE},
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
	{
		if(k.String() == "experience") ret.whatHeroKeeps.experience = true;
		if(k.String() == "primarySkills") ret.whatHeroKeeps.primarySkills = true;
		if(k.String() == "secondarySkills") ret.whatHeroKeeps.secondarySkills = true;
		if(k.String() == "spells") ret.whatHeroKeeps.spells = true;
		if(k.String() == "artifacts") ret.whatHeroKeeps.artifacts = true;
	}
	
	for(auto & k : reader["keepCreatures"].Vector())
	{
		if(auto identifier = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), "creature", k.String()))
			ret.monstersKeptByHero.insert(CreatureID(identifier.value()));
		else
			logGlobal->warn("VCMP Loading: keepCreatures contains unresolved identifier %s", k.String());
	}
	for(auto & k : reader["keepArtifacts"].Vector())
	{
		if(auto identifier = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), "artifact", k.String()))
			ret.artifactsKeptByHero.insert(ArtifactID(identifier.value()));
		else
			logGlobal->warn("VCMP Loading: keepArtifacts contains unresolved identifier %s", k.String());
	}

	ret.startOptions = startOptionsMap[reader["startOptions"].String()];
	switch(ret.startOptions)
	{
	case CampaignStartOptions::NONE:
		//no bonuses. Seems to be OK
		break;
	case CampaignStartOptions::START_BONUS: //reading of bonuses player can choose
		{
			ret.playerColor = reader["playerColor"].Integer();
			for(auto & bjson : reader["bonuses"].Vector())
			{
				CampaignBonus bonus;
				bonus.type = bonusTypeMap[bjson["what"].String()];
				switch (bonus.type)
				{
					case CampaignBonusType::RESOURCE:
						bonus.info1 = resourceTypeMap[bjson["type"].String()];
						bonus.info2 = bjson["amount"].Integer();
						break;
						
					case CampaignBonusType::BUILDING:
						bonus.info1 = vstd::find_pos(EBuildingType::names, bjson["type"].String());
						if(bonus.info1 == -1)
							logGlobal->warn("VCMP Loading: unresolved building identifier %s", bjson["type"].String());
						break;
						
					default:
						if(int heroId = heroSpecialMap[bjson["hero"].String()])
							bonus.info1 = heroId;
						else
							if(auto identifier = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), "hero", bjson["hero"].String()))
								bonus.info1 = identifier.value();
							else
								logGlobal->warn("VCMP Loading: unresolved hero identifier %s", bjson["hero"].String());
	
						bonus.info3 = bjson["amount"].Integer();
						
						switch(bonus.type)
						{
							case CampaignBonusType::SPELL:
							case CampaignBonusType::MONSTER:
							case CampaignBonusType::SECONDARY_SKILL:
							case CampaignBonusType::ARTIFACT:
								if(auto identifier  = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), bjson["what"].String(), bjson["type"].String()))
									bonus.info2 = identifier.value();
								else
									logGlobal->warn("VCMP Loading: unresolved %s identifier %s", bjson["what"].String(), bjson["type"].String());
								break;
								
							case CampaignBonusType::SPELL_SCROLL:
								if(auto Identifier = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), "spell", bjson["type"].String()))
									bonus.info2 = Identifier.value();
								else
									logGlobal->warn("VCMP Loading: unresolved spell scroll identifier %s", bjson["type"].String());
								break;
								
							case CampaignBonusType::PRIMARY_SKILL:
								for(auto & ps : primarySkillsMap)
									bonus.info2 |= bjson[ps.first].Integer() << ps.second;
								break;
								
							default:
								bonus.info2 = bjson["type"].Integer();
						}
						break;
				}
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case CampaignStartOptions::HERO_CROSSOVER: //reading of players (colors / scenarios ?) player can choose
		{
			for(auto & bjson : reader["bonuses"].Vector())
			{
				CampaignBonus bonus;
				bonus.type = CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO;
				bonus.info1 = bjson["playerColor"].Integer(); //player color
				bonus.info2 = bjson["scenario"].Integer(); //from what scenario
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case CampaignStartOptions::HERO_OPTIONS: //heroes player can choose between
		{
			for(auto & bjson : reader["bonuses"].Vector())
			{
				CampaignBonus bonus;
				bonus.type = CampaignBonusType::HERO;
				bonus.info1 = bjson["playerColor"].Integer(); //player color
				
				if(int heroId = heroSpecialMap[bjson["hero"].String()])
					bonus.info2 = heroId;
				else
					if (auto identifier = VLC->identifiers()->getIdentifier(ModScope::scopeMap(), "hero", bjson["hero"].String()))
						bonus.info2 = identifier.value();
					else
						logGlobal->warn("VCMP Loading: unresolved hero identifier %s", bjson["hero"].String());
			
				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	default:
		{
			logGlobal->warn("VCMP Loading: Unsupported start options value");
			break;
		}
	}

	return ret;
}


void CampaignHandler::readHeaderFromMemory( CampaignHeader & ret, CBinaryReader & reader, std::string filename, std::string modName, std::string encoding )
{
	ret.version = static_cast<CampaignVersion>(reader.readUInt32());
	ui8 campId = reader.readUInt8() - 1;//change range of it from [1, 20] to [0, 19]
	ret.loadLegacyData(campId);
	ret.name = readLocalizedString(reader, filename, modName, encoding, "name");
	ret.description = readLocalizedString(reader, filename, modName, encoding, "description");
	if (ret.version > CampaignVersion::RoE)
		ret.difficultyChoosenByPlayer = reader.readInt8();
	else
		ret.difficultyChoosenByPlayer = false;

	ret.music = prologMusicName(reader.readInt8());
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
}

CampaignScenario CampaignHandler::readScenarioFromMemory( CBinaryReader & reader, const CampaignHeader & header)
{
	auto prologEpilogReader = [&](const std::string & identifier) -> CampaignScenarioPrologEpilog
	{
		CampaignScenarioPrologEpilog ret;
		ret.hasPrologEpilog = reader.readUInt8();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = CampaignHandler::prologVideoName(reader.readUInt8());
			ret.prologMusic = CampaignHandler::prologMusicName(reader.readUInt8());
			ret.prologText = readLocalizedString(reader, header.filename, header.modName, header.encoding, identifier);
		}
		return ret;
	};

	CampaignScenario ret;
	ret.mapName = reader.readBaseString();
	reader.readUInt32(); //packedMapSize - not used
	if(header.numberOfScenarios > 8) //unholy alliance
	{
		ret.loadPreconditionRegions(reader.readUInt16());
	}
	else
	{
		ret.loadPreconditionRegions(reader.readUInt8());
	}
	ret.regionColor = reader.readUInt8();
	ret.difficulty = reader.readUInt8();
	ret.regionText = readLocalizedString(reader, header.filename, header.modName, header.encoding, ret.mapName + ".region");
	ret.prolog = prologEpilogReader(ret.mapName + ".prolog");
	ret.epilog = prologEpilogReader(ret.mapName + ".epilog");

	ret.travelOptions = readScenarioTravelFromMemory(reader, header.version);

	return ret;
}

template<typename Identifier>
static void readContainer(std::set<Identifier> & container, CBinaryReader & reader, int sizeBytes)
{
	for(int iId = 0, byte = 0; iId < sizeBytes * 8; ++iId)
	{
		if(iId % 8 == 0)
			byte = reader.readUInt8();
		if(byte & (1 << iId % 8))
			container.insert(Identifier(iId));
	}
}

CampaignTravel CampaignHandler::readScenarioTravelFromMemory(CBinaryReader & reader, CampaignVersion version )
{
	CampaignTravel ret;

	ui8 whatHeroKeeps = reader.readUInt8();
	ret.whatHeroKeeps.experience = whatHeroKeeps & 1;
	ret.whatHeroKeeps.primarySkills = whatHeroKeeps & 2;
	ret.whatHeroKeeps.secondarySkills = whatHeroKeeps & 4;
	ret.whatHeroKeeps.spells = whatHeroKeeps & 8;
	ret.whatHeroKeeps.artifacts = whatHeroKeeps & 16;
	
	readContainer(ret.monstersKeptByHero, reader, 19);
	readContainer(ret.artifactsKeptByHero, reader, version < CampaignVersion::SoD ? 17 : 18);

	ret.startOptions = static_cast<CampaignStartOptions>(reader.readUInt8());

	switch(ret.startOptions)
	{
	case CampaignStartOptions::NONE:
		//no bonuses. Seems to be OK
		break;
	case CampaignStartOptions::START_BONUS: //reading of bonuses player can choose
		{
			ret.playerColor = reader.readUInt8();
			ui8 numOfBonuses = reader.readUInt8();
			for (int g=0; g<numOfBonuses; ++g)
			{
				CampaignBonus bonus;
				bonus.type = static_cast<CampaignBonusType>(reader.readUInt8());
				//hero: FFFD means 'most powerful' and FFFE means 'generated'
				switch(bonus.type)
				{
				case CampaignBonusType::SPELL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt8(); //spell ID
						break;
					}
				case CampaignBonusType::MONSTER:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt16(); //monster type
						bonus.info3 = reader.readUInt16(); //monster count
						break;
					}
				case CampaignBonusType::BUILDING:
					{
						bonus.info1 = reader.readUInt8(); //building ID (0 - town hall, 1 - city hall, 2 - capitol, etc)
						break;
					}
				case CampaignBonusType::ARTIFACT:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt16(); //artifact ID
						break;
					}
				case CampaignBonusType::SPELL_SCROLL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt8(); //spell ID
						break;
					}
				case CampaignBonusType::PRIMARY_SKILL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt32(); //bonuses (4 bytes for 4 skills)
						break;
					}
				case CampaignBonusType::SECONDARY_SKILL:
					{
						bonus.info1 = reader.readUInt16(); //hero
						bonus.info2 = reader.readUInt8(); //skill ID
						bonus.info3 = reader.readUInt8(); //skill level
						break;
					}
				case CampaignBonusType::RESOURCE:
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
	case CampaignStartOptions::HERO_CROSSOVER: //reading of players (colors / scenarios ?) player can choose
		{
			ui8 numOfBonuses = reader.readUInt8();
			for (int g=0; g<numOfBonuses; ++g)
			{
				CampaignBonus bonus;
				bonus.type = CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO;
				bonus.info1 = reader.readUInt8(); //player color
				bonus.info2 = reader.readUInt8(); //from what scenario

				ret.bonusesToChoose.push_back(bonus);
			}
			break;
		}
	case CampaignStartOptions::HERO_OPTIONS: //heroes player can choose between
		{
			ui8 numOfBonuses = reader.readUInt8();
			for (int g=0; g<numOfBonuses; ++g)
			{
				CampaignBonus bonus;
				bonus.type = CampaignBonusType::HERO;
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

std::vector< std::vector<ui8> > CampaignHandler::getFile(std::unique_ptr<CInputStream> file, bool headerOnly)
{
	CCompressedStream stream(std::move(file), true);

	std::vector< std::vector<ui8> > ret;
	do
	{
		std::vector<ui8> block(stream.getSize());
		stream.read(block.data(), block.size());
		ret.push_back(block);
		ret.back().shrink_to_fit();
	}
	while (!headerOnly && stream.getNextBlock());

	return ret;
}

std::string CampaignHandler::prologVideoName(ui8 index)
{
	JsonNode config(ResourcePath(std::string("CONFIG/campaignMedia"), EResType::TEXT));
	auto vids = config["videos"].Vector();
	if(index < vids.size())
		return vids[index].String();
	return "";
}

std::string CampaignHandler::prologMusicName(ui8 index)
{
	std::vector<std::string> music;
	return VLC->generaltexth->translate("core.cmpmusic." + std::to_string(static_cast<int>(index)));
}

std::string CampaignHandler::prologVoiceName(ui8 index)
{
	JsonNode config(ResourcePath(std::string("CONFIG/campaignMedia"), EResType::TEXT));
	auto audio = config["voice"].Vector();
	if(index < audio.size())
		return audio[index].String();
	return "";
}

VCMI_LIB_NAMESPACE_END
