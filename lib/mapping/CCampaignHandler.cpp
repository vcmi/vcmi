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
#include "../StringConstants.h"
#include "CMapService.h"
#include "CMap.h"
#include "CMapInfo.h"

// For hero crossover
#include "serializer/CSerializer.h"
#include "serializer/JsonDeserializer.h"
#include "serializer/JsonSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

CampaignRegions::RegionDescription CampaignRegions::RegionDescription::fromJson(const JsonNode & node)
{
	CampaignRegions::RegionDescription rd;
	rd.infix = node["infix"].String();
	rd.xpos = static_cast<int>(node["x"].Float());
	rd.ypos = static_cast<int>(node["y"].Float());
	return rd;
}

CampaignRegions CampaignRegions::fromJson(const JsonNode & node)
{
	CampaignRegions cr;
	cr.campPrefix = node["prefix"].String();
	cr.colorSuffixLength = static_cast<int>(node["color_suffix_length"].Float());

	for(const JsonNode & desc : node["desc"].Vector())
		cr.regions.push_back(CampaignRegions::RegionDescription::fromJson(desc));
	
	return cr;
}

CampaignRegions CampaignRegions::getLegacy(int campId)
{
	static std::vector<CampaignRegions> campDescriptions;
	if(campDescriptions.empty()) //read once
	{
		const JsonNode config(ResourceID("config/campaign_regions.json"));
		for(const JsonNode & campaign : config["campaign_regions"].Vector())
			campDescriptions.push_back(CampaignRegions::fromJson(campaign));
	}
	
	return campDescriptions.at(campId);
}


bool CampaignBonus::isBonusForHero() const
{
	return type == CampaignBonusType::SPELL ||
		   type == CampaignBonusType::MONSTER ||
		   type == CampaignBonusType::ARTIFACT ||
		   type == CampaignBonusType::SPELL_SCROLL ||
		   type == CampaignBonusType::PRIMARY_SKILL ||
		   type == CampaignBonusType::SECONDARY_SKILL;
}

void CampaignHeader::loadLegacyData(ui8 campId)
{
	campaignRegions = CampaignRegions::getLegacy(campId);
	numberOfScenarios = VLC->generaltexth->getCampaignLength(campId);
}

CampaignHeader CampaignHandler::getHeader( const std::string & name)
{
	ResourceID resourceID(name, EResType::CAMPAIGN);
	std::string modName = VLC->modh->findResourceOrigin(resourceID);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;
	
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);
	std::vector<ui8> cmpgn = getFile(std::move(fileStream), true)[0];
	JsonNode jsonCampaign((const char*)cmpgn.data(), cmpgn.size());
	if(jsonCampaign.isNull())
	{
		//legacy OH3 campaign (*.h3c)
		CMemoryStream stream(cmpgn.data(), cmpgn.size());
		CBinaryReader reader(&stream);
		return readHeaderFromMemory(reader, resourceID.getName(), modName, encoding);
	}
	
	//VCMI (*.vcmp)
	return readHeaderFromJson(jsonCampaign, resourceID.getName(), modName, encoding);
}

std::shared_ptr<CampaignState> CampaignHandler::getCampaign( const std::string & name )
{
	ResourceID resourceID(name, EResType::CAMPAIGN);
	std::string modName = VLC->modh->findResourceOrigin(resourceID);
	std::string language = VLC->modh->getModLanguage(modName);
	std::string encoding = Languages::getLanguageOptions(language).encoding;
	
	auto ret = std::make_unique<CampaignState>();
	
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);

	std::vector<std::vector<ui8>> files = getFile(std::move(fileStream), false);

	if (files[0].front() < uint8_t(' ')) // binary format
	{
		CMemoryStream stream(files[0].data(), files[0].size());
		CBinaryReader reader(&stream);
		ret->header = readHeaderFromMemory(reader, resourceID.getName(), modName, encoding);
		
		for(int g = 0; g < ret->header.numberOfScenarios; ++g)
		{
			auto scenarioID = static_cast<CampaignScenarioID>(ret->scenarios.size());
			ret->scenarios[scenarioID] = readScenarioFromMemory(reader, ret->header);
		}
	}
	else // text format (json)
	{
		JsonNode jsonCampaign((const char*)files[0].data(), files[0].size());
		ret->header = readHeaderFromJson(jsonCampaign, resourceID.getName(), modName, encoding);


		for(auto & scenario : jsonCampaign["scenarios"].Vector())
		{
			auto scenarioID = static_cast<CampaignScenarioID>(ret->scenarios.size());
			ret->scenarios[scenarioID] = readScenarioFromJson(scenario);
		}
	}
	
	//first entry is campaign header. start loop from 1
	for(int scenarioID = 0, g = 1; g < files.size() && scenarioID < ret->header.numberOfScenarios; ++g)
	{
		auto id = static_cast<CampaignScenarioID>(scenarioID);

		while(!ret->scenarios[id].isNotVoid()) //skip void scenarios
			scenarioID++;

		std::string scenarioName = resourceID.getName();
		boost::to_lower(scenarioName);
		scenarioName += ':' + std::to_string(g - 1);

		//set map piece appropriately, convert vector to string
		ret->mapPieces[id].assign(reinterpret_cast<const char*>(files[g].data()), files[g].size());
		CMapService mapService;
		auto hdr = mapService.loadMapHeader(
			reinterpret_cast<const ui8 *>(ret->mapPieces[id].c_str()),
			static_cast<int>(ret->mapPieces[id].size()),
			scenarioName,
			modName,
			encoding);
		ret->scenarios[id].scenarioName = hdr->name;
		scenarioID++;
	}

	for(int i = 0; i < ret->scenarios.size(); i++)
	{
		auto scenarioID = static_cast<CampaignScenarioID>(i);

		if(vstd::contains(ret->mapPieces, scenarioID)) //not all maps must be present in a campaign
			ret->mapsRemaining.push_back(scenarioID);
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

CampaignHeader CampaignHandler::readHeaderFromJson(JsonNode & reader, std::string filename, std::string modName, std::string encoding)
{
	CampaignHeader ret;

	ret.version = static_cast<CampaignVersion>(reader["version"].Integer());
	if(ret.version < CampaignVersion::VCMI_MIN || ret.version > CampaignVersion::VCMI_MAX)
	{
		logGlobal->info("VCMP Loading: Unsupported campaign %s version %d", filename, static_cast<int>(ret.version));
		return ret;
	}
	
	ret.version = CampaignVersion::VCMI;
	ret.campaignRegions = CampaignRegions::fromJson(reader["regions"]);
	ret.numberOfScenarios = reader["scenarios"].Vector().size();
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
	ret.conquered = false;
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
		if(auto identifier = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "creature", k.String()))
			ret.monstersKeptByHero.insert(CreatureID(identifier.value()));
		else
			logGlobal->warn("VCMP Loading: keepCreatures contains unresolved identifier %s", k.String());
	}
	for(auto & k : reader["keepArtifacts"].Vector())
	{
		if(auto identifier = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "artifact", k.String()))
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
							if(auto identifier = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "hero", bjson["hero"].String()))
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
								if(auto identifier  = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), bjson["what"].String(), bjson["type"].String()))
									bonus.info2 = identifier.value();
								else
									logGlobal->warn("VCMP Loading: unresolved %s identifier %s", bjson["what"].String(), bjson["type"].String());
								break;
								
							case CampaignBonusType::SPELL_SCROLL:
								if(auto Identifier = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "spell", bjson["type"].String()))
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
					if (auto identifier = VLC->modh->identifiers.getIdentifier(CModHandler::scopeMap(), "hero", bjson["hero"].String()))
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


CampaignHeader CampaignHandler::readHeaderFromMemory( CBinaryReader & reader, std::string filename, std::string modName, std::string encoding )
{
	CampaignHeader ret;

	ret.version = static_cast<CampaignVersion>(reader.readUInt32());
	ui8 campId = reader.readUInt8() - 1;//change range of it from [1, 20] to [0, 19]
	ret.loadLegacyData(campId);
	ret.name = readLocalizedString(reader, filename, modName, encoding, "name");
	ret.description = readLocalizedString(reader, filename, modName, encoding, "description");
	if (ret.version > CampaignVersion::RoE)
		ret.difficultyChoosenByPlayer = reader.readInt8();
	else
		ret.difficultyChoosenByPlayer = false;
	reader.readInt8(); //music - skip as unused
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
	ret.valid = true;
	return ret;
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
	ret.conquered = false;
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

void CampaignScenario::loadPreconditionRegions(ui32 regions)
{
	for (int i=0; i<32; i++) //for each bit in region. h3c however can only hold up to 16
	{
		if ( (1 << i) & regions)
			preconditionRegions.insert(static_cast<CampaignScenarioID>(i));
	}
}

template<typename Identifier>
static void readContainer(std::set<Identifier> container, CBinaryReader & reader, int sizeBytes)
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
	}
	while (!headerOnly && stream.getNextBlock());

	return ret;
}

bool CampaignState::conquerable(CampaignScenarioID whichScenario) const
{
	//check for void scenraio
	if (!scenarios.at(whichScenario).isNotVoid())
	{
		return false;
	}

	if (scenarios.at(whichScenario).conquered)
	{
		return false;
	}
	//check preconditioned regions
	for (auto const & it : scenarios.at(whichScenario).preconditionRegions)
	{
		if (!scenarios.at(it).conquered)
			return false;
	}
	return true;
}

bool CampaignScenario::isNotVoid() const
{
	return !mapName.empty();
}

const CGHeroInstance * CampaignScenario::strongestHero(const PlayerColor & owner)
{
	std::function<bool(JsonNode & node)> isOwned = [owner](JsonNode & node)
	{
		auto * h = CampaignState::crossoverDeserialize(node);
		bool result = h->tempOwner == owner;
		vstd::clear_pointer(h);
		return result;
	};
	auto ownedHeroes = crossoverHeroes | boost::adaptors::filtered(isOwned);

	auto i = vstd::maxElementByFun(ownedHeroes, [](JsonNode & node)
	{
		auto * h = CampaignState::crossoverDeserialize(node);
		double result = h->getHeroStrength();
		vstd::clear_pointer(h);
		return result;
	});
	return i == ownedHeroes.end() ? nullptr : CampaignState::crossoverDeserialize(*i);
}

std::vector<CGHeroInstance *> CampaignScenario::getLostCrossoverHeroes()
{
	std::vector<CGHeroInstance *> lostCrossoverHeroes;
	if(conquered)
	{
		for(auto node2 : placedCrossoverHeroes)
		{
			auto * hero = CampaignState::crossoverDeserialize(node2);
			auto it = range::find_if(crossoverHeroes, [hero](JsonNode node)
			{
				auto * h = CampaignState::crossoverDeserialize(node);
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

void CampaignState::setCurrentMapAsConquered(const std::vector<CGHeroInstance *> & heroes)
{
	scenarios.at(*currentMap).crossoverHeroes.clear();
	for(CGHeroInstance * hero : heroes)
	{
		scenarios.at(*currentMap).crossoverHeroes.push_back(crossoverSerialize(hero));
	}

	mapsConquered.push_back(*currentMap);
	mapsRemaining -= *currentMap;
	scenarios.at(*currentMap).conquered = true;
}

std::optional<CampaignBonus> CampaignState::getBonusForCurrentMap() const
{
	auto bonuses = getCurrentScenario().travelOptions.bonusesToChoose;
	assert(chosenCampaignBonuses.count(*currentMap) || bonuses.size() == 0);

	if(bonuses.empty())
		return std::optional<CampaignBonus>();
	else
		return bonuses[currentBonusID()];
}

const CampaignScenario & CampaignState::getCurrentScenario() const
{
	return scenarios.at(*currentMap);
}

CampaignScenario & CampaignState::getCurrentScenario()
{
	return scenarios.at(*currentMap);
}

ui8 CampaignState::currentBonusID() const
{
	return chosenCampaignBonuses.at(*currentMap);
}

std::unique_ptr<CMap> CampaignState::getMap(CampaignScenarioID scenarioId) const
{
	// FIXME: there is certainly better way to handle maps inside campaigns
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();
	
	CMapService mapService;
	std::string scenarioName = header.filename.substr(0, header.filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const std::string & mapContent = mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMap(buffer, static_cast<int>(mapContent.size()), scenarioName, header.modName, header.encoding);
}

std::unique_ptr<CMapHeader> CampaignState::getHeader(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();
	
	CMapService mapService;
	std::string scenarioName = header.filename.substr(0, header.filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const std::string & mapContent = mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMapHeader(buffer, static_cast<int>(mapContent.size()), scenarioName, header.modName, header.encoding);
}

std::shared_ptr<CMapInfo> CampaignState::getMapInfo(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->fileURI = header.filename;
	mapInfo->mapHeader = getHeader(scenarioId);
	mapInfo->countPlayers();
	return mapInfo;
}

JsonNode CampaignState::crossoverSerialize(CGHeroInstance * hero)
{
	JsonNode node;
	JsonSerializer handler(nullptr, node);
	hero->serializeJsonOptions(handler);
	return node;
}

CGHeroInstance * CampaignState::crossoverDeserialize(JsonNode & node)
{
	JsonDeserializer handler(nullptr, node);
	auto * hero = new CGHeroInstance();
	hero->ID = Obj::HERO;
	hero->serializeJsonOptions(handler);
	return hero;
}

std::string CampaignHandler::prologVideoName(ui8 index)
{
	JsonNode config(ResourceID(std::string("CONFIG/campaignMedia"), EResType::TEXT));
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
	JsonNode config(ResourceID(std::string("CONFIG/campaignMedia"), EResType::TEXT));
	auto audio = config["voice"].Vector();
	if(index < audio.size())
		return audio[index].String();
	return "";
}

VCMI_LIB_NAMESPACE_END
