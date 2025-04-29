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
#include "../filesystem/CZipLoader.h"
#include "../GameLibrary.h"
#include "../constants/StringConstants.h"
#include "../mapping/CMapHeader.h"
#include "../mapping/CMapService.h"
#include "../modding/CModHandler.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextOperations.h"

VCMI_LIB_NAMESPACE_BEGIN

void CampaignHandler::readCampaign(Campaign * ret, const std::vector<ui8> & input, std::string filename, std::string modName, std::string encoding)
{
	if (input.front() < uint8_t(' ')) // binary format
	{
		CMemoryStream stream(input.data(), input.size());
		CBinaryReader reader(&stream);

		readHeaderFromMemory(*ret, reader, filename, modName, encoding);
		ret->overrideCampaign();

		for(int g = 0; g < ret->numberOfScenarios; ++g)
		{
			auto scenarioID = static_cast<CampaignScenarioID>(ret->scenarios.size());
			ret->scenarios[scenarioID] = readScenarioFromMemory(reader, *ret);
		}
		ret->overrideCampaignScenarios();
	}
	else // text format (json)
	{
		JsonNode jsonCampaign(reinterpret_cast<const std::byte*>(input.data()), input.size(), filename);
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
	std::string modName = LIBRARY->modh->findResourceOrigin(resourceID);
	std::string encoding = LIBRARY->modh->findResourceEncoding(resourceID);
	
	auto ret = std::make_unique<Campaign>();
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);
	std::vector<ui8> cmpgn = getFile(std::move(fileStream), name, true)[0];

	readCampaign(ret.get(), cmpgn, resourceID.getName(), modName, encoding);

	return ret;
}

std::shared_ptr<CampaignState> CampaignHandler::getCampaign( const std::string & name )
{
	ResourcePath resourceID(name, EResType::CAMPAIGN);
	std::string modName = LIBRARY->modh->findResourceOrigin(resourceID);
	std::string encoding = LIBRARY->modh->findResourceEncoding(resourceID);
	
	auto ret = std::make_unique<CampaignState>();
	
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);

	std::vector<std::vector<ui8>> files = getFile(std::move(fileStream), name, false);

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

std::string CampaignHandler::readLocalizedString(CampaignHeader & target, CBinaryReader & reader, std::string filename, std::string modName, std::string encoding, std::string identifier)
{
	std::string input = TextOperations::toUnicode(reader.readBaseString(), encoding);

	return readLocalizedString(target, input, filename, modName, identifier);
}

std::string CampaignHandler::readLocalizedString(CampaignHeader & target, std::string text, std::string filename, std::string modName, std::string identifier)
{
	TextIdentifier stringID( "campaign", convertMapName(filename), identifier);

	if (text.empty())
		return "";

	target.getTexts().registerString(modName, stringID, text);
	return stringID.get();
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
	ret.name.appendTextID(readLocalizedString(ret, reader["name"].String(), filename, modName, "name"));
	ret.description.appendTextID(readLocalizedString(ret, reader["description"].String(), filename, modName, "description"));
	ret.author.appendRawString(reader["author"].String());
	ret.authorContact.appendRawString(reader["authorContact"].String());
	ret.campaignVersion.appendRawString(reader["campaignVersion"].String());
	ret.creationDateTime = reader["creationDateTime"].Integer();
	ret.difficultyChosenByPlayer = reader["allowDifficultySelection"].Bool();
	ret.music = AudioPath::fromJson(reader["music"]);
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
	ret.loadingBackground = ImagePath::fromJson(reader["loadingBackground"]);
	ret.videoRim = ImagePath::fromJson(reader["videoRim"]);
	ret.introVideo = VideoPath::fromJson(reader["introVideo"]);
	ret.outroVideo = VideoPath::fromJson(reader["outroVideo"]);
}

JsonNode CampaignHandler::writeHeaderToJson(CampaignHeader & header)
{
	JsonNode node;
	node["version"].Integer() = static_cast<ui64>(CampaignVersion::VCMI);
	node["regions"] = CampaignRegions::toJson(header.campaignRegions);
	node["name"].String() = header.name.toString();
	node["description"].String() = header.description.toString();
	node["author"].String() = header.author.toString();
	node["authorContact"].String() = header.authorContact.toString();
	node["campaignVersion"].String() = header.campaignVersion.toString();
	node["creationDateTime"].Integer() = header.creationDateTime;
	node["allowDifficultySelection"].Bool() = header.difficultyChosenByPlayer;
	node["music"].String() = header.music.getName();
	node["loadingBackground"].String() = header.loadingBackground.getName();
	node["videoRim"].String() = header.videoRim.getName();
	node["introVideo"].String() = header.introVideo.getName();
	node["outroVideo"].String() = header.outroVideo.getName();
	return node;
}

CampaignScenario CampaignHandler::readScenarioFromJson(JsonNode & reader)
{
	auto prologEpilogReader = [](JsonNode & identifier) -> CampaignScenarioPrologEpilog
	{
		CampaignScenarioPrologEpilog ret;
		ret.hasPrologEpilog = !identifier.isNull();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = VideoPath::fromJson(identifier["video"]);
			ret.prologMusic = AudioPath::fromJson(identifier["music"]);
			ret.prologVoice = AudioPath::fromJson(identifier["voice"]);
			ret.prologText.jsonDeserialize(identifier["text"]);
		}
		return ret;
	};

	CampaignScenario ret;
	ret.mapName = reader["map"].String();
	for(auto & g : reader["preconditions"].Vector())
		ret.preconditionRegions.insert(static_cast<CampaignScenarioID>(g.Integer()));

	ret.regionColor = reader["color"].Integer();
	ret.difficulty = reader["difficulty"].Integer();
	ret.regionText.jsonDeserialize(reader["regionText"]);
	ret.prolog = prologEpilogReader(reader["prolog"]);
	ret.epilog = prologEpilogReader(reader["epilog"]);

	ret.travelOptions = readScenarioTravelFromJson(reader);

	return ret;
}

JsonNode CampaignHandler::writeScenarioToJson(const CampaignScenario & scenario)
{
	auto prologEpilogWriter = [](const CampaignScenarioPrologEpilog & elem) -> JsonNode
	{
		JsonNode node;
		if(elem.hasPrologEpilog)
		{
			node["video"].String() = elem.prologVideo.getName();
			node["music"].String() = elem.prologMusic.getName();
			node["voice"].String() = elem.prologVoice.getName();
			node["text"].String() = elem.prologText.toString();
		}
		return node;
	};

	JsonNode node;
	node["map"].String() = scenario.mapName;
	for(auto & g : scenario.preconditionRegions)
		node["preconditions"].Vector().push_back(JsonNode(g.getNum()));
	node["color"].Integer() = scenario.regionColor;
	node["difficulty"].Integer() = scenario.difficulty;
	node["regionText"].String() = scenario.regionText.toString();
	node["prolog"] = prologEpilogWriter(scenario.prolog);
	node["epilog"] = prologEpilogWriter(scenario.epilog);

	writeScenarioTravelToJson(node, scenario.travelOptions);

	return node;
}

static const std::map<std::string, CampaignStartOptions> startOptionsMap = {
	{"none", CampaignStartOptions::NONE},
	{"bonus", CampaignStartOptions::START_BONUS},
	{"crossover", CampaignStartOptions::HERO_CROSSOVER},
	{"hero", CampaignStartOptions::HERO_OPTIONS}
};

static const std::map<std::string, CampaignBonusType> bonusTypeMap = {
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

static const std::map<std::string, ui32> primarySkillsMap = {
	{"attack", 0},
	{"defence", 8},
	{"spellpower", 16},
	{"knowledge", 24},
};

static const std::map<std::string, ui16> heroSpecialMap = {
	{"strongest", HeroTypeID::CAMP_STRONGEST},
	{"generated", HeroTypeID::CAMP_GENERATED},
	{"random", HeroTypeID::CAMP_RANDOM}
};

static const std::map<std::string, ui8> resourceTypeMap = {
	{"wood", EGameResID::WOOD},
	{"mercury", EGameResID::MERCURY},
	{"ore", EGameResID::ORE},
	{"sulfur", EGameResID::SULFUR},
	{"crystal", EGameResID::CRYSTAL},
	{"gems", EGameResID::GEMS},
	{"gold", EGameResID::GOLD},
	{"common", EGameResID::COMMON},
	{"rare", EGameResID::RARE}
};

CampaignTravel CampaignHandler::readScenarioTravelFromJson(JsonNode & reader)
{
	CampaignTravel ret;
	
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
		if(auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "creature", k.String()))
			ret.monstersKeptByHero.insert(CreatureID(identifier.value()));
		else
			logGlobal->warn("VCMP Loading: keepCreatures contains unresolved identifier %s", k.String());
	}
	for(auto & k : reader["keepArtifacts"].Vector())
	{
		if(auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "artifact", k.String()))
			ret.artifactsKeptByHero.insert(ArtifactID(identifier.value()));
		else
			logGlobal->warn("VCMP Loading: keepArtifacts contains unresolved identifier %s", k.String());
	}

	ret.startOptions = startOptionsMap.at(reader["startOptions"].String());
	switch(ret.startOptions)
	{
	case CampaignStartOptions::NONE:
		//no bonuses. Seems to be OK
		break;
	case CampaignStartOptions::START_BONUS: //reading of bonuses player can choose
		{
			ret.playerColor = PlayerColor(PlayerColor::decode(reader["playerColor"].String()));
			for(auto & bjson : reader["bonuses"].Vector())
			{
				CampaignBonus bonus;
				bonus.type = bonusTypeMap.at(bjson["what"].String());
				switch (bonus.type)
				{
					case CampaignBonusType::RESOURCE:
						bonus.info1 = resourceTypeMap.at(bjson["type"].String());
						bonus.info2 = bjson["amount"].Integer();
						break;
						
					case CampaignBonusType::BUILDING:
						bonus.info1 = vstd::find_pos(EBuildingType::names, bjson["type"].String());
						if(bonus.info1 == -1)
							logGlobal->warn("VCMP Loading: unresolved building identifier %s", bjson["type"].String());
						break;
						
					default:
						auto heroIdentifier = bjson["hero"].String();
						auto it = heroSpecialMap.find(heroIdentifier);
						if(it != heroSpecialMap.end())
							bonus.info1 = it->second;
						else
							if(auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "hero", heroIdentifier))
								bonus.info1 = identifier.value();
							else
								logGlobal->warn("VCMP Loading: unresolved hero identifier %s", heroIdentifier);
	
						bonus.info3 = bjson["amount"].Integer();
						
						switch(bonus.type)
						{
							case CampaignBonusType::SPELL:
							case CampaignBonusType::MONSTER:
							case CampaignBonusType::SECONDARY_SKILL:
							case CampaignBonusType::ARTIFACT:
								if(auto identifier  = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), bjson["what"].String(), bjson["type"].String()))
									bonus.info2 = identifier.value();
								else
									logGlobal->warn("VCMP Loading: unresolved %s identifier %s", bjson["what"].String(), bjson["type"].String());
								break;
								
							case CampaignBonusType::SPELL_SCROLL:
								if(auto Identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "spell", bjson["type"].String()))
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

				auto heroIdentifier = bjson["hero"].String();
				auto it = heroSpecialMap.find(heroIdentifier);
				if(it != heroSpecialMap.end())
					bonus.info2 = it->second;
				else
					if (auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "hero", heroIdentifier))
						bonus.info2 = identifier.value();
					else
						logGlobal->warn("VCMP Loading: unresolved hero identifier %s", heroIdentifier);
			
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

void CampaignHandler::writeScenarioTravelToJson(JsonNode & node, const CampaignTravel & travel)
{
	if(travel.whatHeroKeeps.experience)
		node["heroKeeps"].Vector().push_back(JsonNode("experience"));
	if(travel.whatHeroKeeps.primarySkills)
		node["heroKeeps"].Vector().push_back(JsonNode("primarySkills"));
	if(travel.whatHeroKeeps.secondarySkills)
		node["heroKeeps"].Vector().push_back(JsonNode("secondarySkills"));
	if(travel.whatHeroKeeps.spells)
		node["heroKeeps"].Vector().push_back(JsonNode("spells"));
	if(travel.whatHeroKeeps.artifacts)
		node["heroKeeps"].Vector().push_back(JsonNode("artifacts"));
	for(auto & c : travel.monstersKeptByHero)
		node["keepCreatures"].Vector().push_back(JsonNode(CreatureID::encode(c)));
	for(auto & a : travel.artifactsKeptByHero)
		node["keepArtifacts"].Vector().push_back(JsonNode(ArtifactID::encode(a)));
	node["startOptions"].String() = vstd::reverseMap(startOptionsMap)[travel.startOptions];

	switch(travel.startOptions)
	{
	case CampaignStartOptions::NONE:
		break;
	case CampaignStartOptions::START_BONUS:
		{
			node["playerColor"].String() = PlayerColor::encode(travel.playerColor);
			for(auto & bonus : travel.bonusesToChoose)
			{
				JsonNode bnode;
				bnode["what"].String() = vstd::reverseMap(bonusTypeMap)[bonus.type];
				switch (bonus.type)
				{
					case CampaignBonusType::RESOURCE:
						bnode["type"].String() = vstd::reverseMap(resourceTypeMap)[bonus.info1];
						bnode["amount"].Integer() = bonus.info2;
						break;
					case CampaignBonusType::BUILDING:
						bnode["type"].String() = EBuildingType::names[bonus.info1];
						break;
					default:
						if(vstd::contains(vstd::reverseMap(heroSpecialMap), bonus.info1))
							bnode["hero"].String() = vstd::reverseMap(heroSpecialMap)[bonus.info1];
						else
							bnode["hero"].String() = HeroTypeID::encode(bonus.info1);
						bnode["amount"].Integer() = bonus.info3;
						switch(bonus.type)
						{
							case CampaignBonusType::SPELL:
								bnode["type"].String() = SpellID::encode(bonus.info2);
								break;
							case CampaignBonusType::MONSTER:
								bnode["type"].String() = CreatureID::encode(bonus.info2);
								break;
							case CampaignBonusType::SECONDARY_SKILL:
								bnode["type"].String() = SecondarySkill::encode(bonus.info2);
								break;
							case CampaignBonusType::ARTIFACT:
								bnode["type"].String() = ArtifactID::encode(bonus.info2);
								break;
							case CampaignBonusType::SPELL_SCROLL:
								bnode["type"].String() = SpellID::encode(bonus.info2);
								break;
							case CampaignBonusType::PRIMARY_SKILL:
								for(auto & ps : primarySkillsMap)
									bnode[ps.first].Integer() = (bonus.info2 >> ps.second) & 0xff;
								break;
							default:
								bnode["type"].Integer() = bonus.info2;
						}
						break;
				}
				node["bonuses"].Vector().push_back(bnode);
			}
			break;
		}
	case CampaignStartOptions::HERO_CROSSOVER:
		{
			for(auto & bonus : travel.bonusesToChoose)
			{
				JsonNode bnode;
				bnode["playerColor"].Integer() = bonus.info1;
				bnode["scenario"].Integer() = bonus.info2;
				node["bonuses"].Vector().push_back(bnode);
			}
			break;
		}
	case CampaignStartOptions::HERO_OPTIONS:
		{
			for(auto & bonus : travel.bonusesToChoose)
			{
				JsonNode bnode;
				bnode["playerColor"].Integer() = bonus.info1;

				if(vstd::contains(vstd::reverseMap(heroSpecialMap), bonus.info2))
					bnode["hero"].String() = vstd::reverseMap(heroSpecialMap)[bonus.info2];
				else
					bnode["hero"].String() = HeroTypeID::encode(bonus.info2);
				
				node["bonuses"].Vector().push_back(bnode);
			}
			break;
		}
	}
}

void CampaignHandler::readHeaderFromMemory( CampaignHeader & ret, CBinaryReader & reader, std::string filename, std::string modName, std::string encoding )
{
	ret.version = static_cast<CampaignVersion>(reader.readUInt32());

	if (ret.version == CampaignVersion::HotA)
	{
		[[maybe_unused]] int32_t unknownA = reader.readInt32();
		[[maybe_unused]] int32_t unknownB = reader.readInt32();
		[[maybe_unused]] int32_t unknownC = reader.readInt8();
		ret.numberOfScenarios = reader.readInt32();

		assert(unknownA == 1);
		assert(unknownB == 1);
		assert(unknownC == 0);
		assert(ret.numberOfScenarios <= 8);

		// TODO. Or they are hardcoded in this hota version?
		// ret.campaignRegions = ???;
	}

	ui8 campId = reader.readUInt8() - 1;//change range of it from [1, 20] to [0, 19]
	if(ret.version < CampaignVersion::Chr) // For chronicles: Will be overridden later; Chronicles uses own logic (reusing OH3 ID's)
		ret.loadLegacyData(campId);
	ret.name.appendTextID(readLocalizedString(ret, reader, filename, modName, encoding, "name"));
	ret.description.appendTextID(readLocalizedString(ret, reader, filename, modName, encoding, "description"));
	ret.author.appendRawString("");
	ret.authorContact.appendRawString("");
	ret.campaignVersion.appendRawString("");
	ret.creationDateTime = 0;
	if (ret.version > CampaignVersion::RoE)
		ret.difficultyChosenByPlayer = reader.readBool();
	else
		ret.difficultyChosenByPlayer = false;

	ret.music = prologMusicName(reader.readInt8());
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
}

CampaignScenario CampaignHandler::readScenarioFromMemory( CBinaryReader & reader, CampaignHeader & header)
{
	auto prologEpilogReader = [&](const std::string & identifier) -> CampaignScenarioPrologEpilog
	{
		CampaignScenarioPrologEpilog ret;
		ret.hasPrologEpilog = reader.readBool();
		if(ret.hasPrologEpilog)
		{
			bool isOriginalCampaign = boost::starts_with(header.getFilename(), "DATA/");

			ui8 index = reader.readUInt8();
			ret.prologVideo = CampaignHandler::prologVideoName(index);
			ret.prologMusic = CampaignHandler::prologMusicName(reader.readUInt8());
			ret.prologVoice = isOriginalCampaign ? CampaignHandler::prologVoiceName(index) : AudioPath();
			ret.prologText.appendTextID(readLocalizedString(header, reader, header.filename, header.modName, header.encoding, identifier));
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
	assert(ret.difficulty < 5);
	ret.regionText.appendTextID(readLocalizedString(header, reader, header.filename, header.modName, header.encoding, ret.mapName + ".region"));
	ret.prolog = prologEpilogReader(ret.mapName + ".prolog");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".prolog2");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".prolog3");
	ret.epilog = prologEpilogReader(ret.mapName + ".epilog");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".epilog2");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".epilog3");

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
	
	if (version == CampaignVersion::HotA)
	{
		readContainer(ret.monstersKeptByHero, reader, 24);
		readContainer(ret.artifactsKeptByHero, reader, 21);
	}
	else
	{
		readContainer(ret.monstersKeptByHero, reader, 19);
		readContainer(ret.artifactsKeptByHero, reader, version < CampaignVersion::SoD ? 17 : 18);
	}

	ret.startOptions = static_cast<CampaignStartOptions>(reader.readUInt8());

	switch(ret.startOptions)
	{
	case CampaignStartOptions::NONE:
		//no bonuses. Seems to be OK
		break;
	case CampaignStartOptions::START_BONUS: //reading of bonuses player can choose
		{
			ret.playerColor.setNum(reader.readUInt8());
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

std::vector< std::vector<ui8> > CampaignHandler::getFile(std::unique_ptr<CInputStream> file, const std::string & filename, bool headerOnly)
{
	std::array<ui8, 2> magic;
	file->read(magic.data(), magic.size());
	file->seek(0);

	std::vector< std::vector<ui8> > ret;

	static const std::array<ui8, 2> zipHeaderMagic{0x50, 0x4B};
	if (magic == zipHeaderMagic) // ZIP archive - assume VCMP format
	{
		CInputStream * buffer(file.get());
		auto ioApi = std::make_shared<CProxyROIOApi>(buffer);
		CZipLoader loader("", "_", ioApi);

		// load header
		JsonPath jsonPath = JsonPath::builtin(VCMP_HEADER_FILE_NAME);
		if(!loader.existsResource(jsonPath))
			throw std::runtime_error(jsonPath.getName() + " not found in " + filename);
		auto data = loader.load(jsonPath)->readAll();
		ret.emplace_back(data.first.get(), data.first.get() + data.second);

		if(headerOnly)
			return ret;

		// load scenarios
		JsonNode header(reinterpret_cast<const std::byte*>(data.first.get()), data.second, VCMP_HEADER_FILE_NAME);
		for(auto scenario : header["scenarios"].Vector())
		{
			ResourcePath mapPath(scenario["map"].String(), EResType::MAP);
			if(!loader.existsResource(mapPath))
				throw std::runtime_error(mapPath.getName() + " not found in " + filename);
			auto data = loader.load(mapPath)->readAll();
			ret.emplace_back(data.first.get(), data.first.get() + data.second);
		}

		return ret;
	}
	else // H3C
	{
		CCompressedStream stream(std::move(file), true);

		try
		{
			do
			{
				std::vector<ui8> block(stream.getSize());
				stream.read(block.data(), block.size());
				ret.push_back(block);
				ret.back().shrink_to_fit();
			}
			while (!headerOnly && stream.getNextBlock());
		}
		catch (const DecompressionException & e)
		{
			// Some campaigns in French version from gog.com have trailing garbage bytes
			// For example, slayer.h3c consist from 5 parts: header + 4 maps
			// However file also contains ~100 "extra" bytes after those 5 parts are decompressed that do not represent gzip stream
			// leading to exception "Incorrect header check"
			// Since H3 handles these files correctly, simply log this as warning and proceed
			logGlobal->warn("Failed to read file %s. Encountered error during decompression: %s", filename, e.what());
		}

		return ret;
	}
}

VideoPath CampaignHandler::prologVideoName(ui8 index)
{
	JsonNode config(JsonPath::builtin("CONFIG/campaignMedia"));
	auto vids = config["videos"].Vector();
	if(index < vids.size())
		return VideoPath::fromJson(vids[index]);
	return VideoPath();
}

AudioPath CampaignHandler::prologMusicName(ui8 index)
{
	std::vector<std::string> music;
	return AudioPath::builtinTODO(LIBRARY->generaltexth->translate("core.cmpmusic." + std::to_string(static_cast<int>(index))));
}

AudioPath CampaignHandler::prologVoiceName(ui8 index)
{
	JsonNode config(JsonPath::builtin("CONFIG/campaignMedia"));
	auto audio = config["voice"].Vector();
	if(index < audio.size())
		return AudioPath::fromJson(audio[index]);
	return AudioPath();
}

VCMI_LIB_NAMESPACE_END
