/*
 * MapFormatH3M.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapFormatH3M.h"

#include "CMap.h"
#include "MapReaderH3M.h"
#include "MapFormat.h"

#include "../ArtifactUtils.h"
#include "../CCreatureHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../CSkillHandler.h"
#include "../CStopWatch.h"
#include "../CModHandler.h"
#include "../GameSettings.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../TerrainHandler.h"
#include "../TextOperations.h"
#include "../VCMI_Lib.h"
#include "../filesystem/CBinaryReader.h"
#include "../filesystem/Filesystem.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGCreature.h"
#include "../mapObjects/MapObjects.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../spells/CSpellHandler.h"

#include <boost/crc.hpp>

VCMI_LIB_NAMESPACE_BEGIN

static std::string convertMapName(std::string input)
{
	boost::algorithm::to_lower(input);
	boost::algorithm::trim(input);

	size_t slashPos = input.find_last_of('/');

	if(slashPos != std::string::npos)
		return input.substr(slashPos + 1);

	return input;
}

CMapLoaderH3M::CMapLoaderH3M(const std::string & mapName, const std::string & modName, const std::string & encodingName, CInputStream * stream)
	: map(nullptr)
	, reader(new MapReaderH3M(stream))
	, inputStream(stream)
	, mapName(convertMapName(mapName))
	, modName(modName)
	, fileEncoding(encodingName)
{
}

//must be instantiated in .cpp file for access to complete types of all member fields
CMapLoaderH3M::~CMapLoaderH3M() = default;

std::unique_ptr<CMap> CMapLoaderH3M::loadMap()
{
	// Init map object by parsing the input buffer
	map = new CMap();
	mapHeader = std::unique_ptr<CMapHeader>(dynamic_cast<CMapHeader *>(map));
	init();

	return std::unique_ptr<CMap>(dynamic_cast<CMap *>(mapHeader.release()));
}

std::unique_ptr<CMapHeader> CMapLoaderH3M::loadMapHeader()
{
	// Read header
	mapHeader = std::make_unique<CMapHeader>();
	readHeader();

	return std::move(mapHeader);
}

void CMapLoaderH3M::init()
{
	//TODO: get rid of double input process
	si64 temp_size = inputStream->getSize();
	inputStream->seek(0);

	auto * temp_buffer = new ui8[temp_size];
	inputStream->read(temp_buffer, temp_size);

	// Compute checksum
	boost::crc_32_type result;
	result.process_bytes(temp_buffer, temp_size);
	map->checksum = result.checksum();

	delete[] temp_buffer;
	inputStream->seek(0);

	readHeader();
	map->allHeroes.resize(map->allowedHeroes.size());

	readDisposedHeroes();
	readMapOptions();
	readAllowedArtifacts();
	readAllowedSpellsAbilities();
	readRumors();
	readPredefinedHeroes();
	readTerrain();
	readObjectTemplates();
	readObjects();
	readEvents();

	map->calculateGuardingGreaturePositions();
	afterRead();
}

void CMapLoaderH3M::readHeader()
{
	// Map version
	mapHeader->version = static_cast<EMapFormat>(reader->readUInt32());

	if(mapHeader->version == EMapFormat::HOTA)
	{
		uint32_t hotaVersion = reader->readUInt32();
		features = MapFormatFeaturesH3M::find(mapHeader->version, hotaVersion);
		reader->setFormatLevel(features);

		if(hotaVersion > 0)
		{
			bool isMirrorMap = reader->readBool();
			bool isArenaMap = reader->readBool();

			//TODO: HotA
			if (isMirrorMap)
				logGlobal->warn("Map '%s': Mirror maps are not yet supported!", mapName);

			if (isArenaMap)
				logGlobal->warn("Map '%s': Arena maps are not supported!", mapName);
		}

		if(hotaVersion > 1)
		{
			[[maybe_unused]] uint8_t unknown = reader->readUInt32();
			assert(unknown == 12);
		}
	}
	else
	{
		features = MapFormatFeaturesH3M::find(mapHeader->version, 0);
		reader->setFormatLevel(features);
	}
	MapIdentifiersH3M identifierMapper;

	if (features.levelROE)
		identifierMapper.loadMapping(VLC->settings()->getValue(EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA));
	if (features.levelAB)
		identifierMapper.loadMapping(VLC->settings()->getValue(EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE));
	if (features.levelSOD)
		identifierMapper.loadMapping(VLC->settings()->getValue(EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH));
	if (features.levelWOG)
		identifierMapper.loadMapping(VLC->settings()->getValue(EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS));
	if (features.levelHOTA0)
		identifierMapper.loadMapping(VLC->settings()->getValue(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS));
	
	reader->setIdentifierRemapper(identifierMapper);

	// include basic mod
	if(mapHeader->version == EMapFormat::WOG)
		mapHeader->mods["wake-of-gods"];

	// Read map name, description, dimensions,...
	mapHeader->areAnyPlayers = reader->readBool();
	mapHeader->height = mapHeader->width = reader->readInt32();
	mapHeader->twoLevel = reader->readBool();
	mapHeader->name = readLocalizedString("header.name");
	mapHeader->description = readLocalizedString("header.description");
	mapHeader->difficulty = reader->readInt8();

	if(features.levelAB)
		mapHeader->levelLimit = reader->readUInt8();
	else
		mapHeader->levelLimit = 0;

	readPlayerInfo();
	readVictoryLossConditions();
	readTeamInfo();
	readAllowedHeroes();
}

void CMapLoaderH3M::readPlayerInfo()
{
	for(int i = 0; i < mapHeader->players.size(); ++i)
	{
		auto & playerInfo = mapHeader->players[i];

		playerInfo.canHumanPlay = reader->readBool();
		playerInfo.canComputerPlay = reader->readBool();

		// If nobody can play with this player - skip loading of these properties
		if((!(playerInfo.canHumanPlay || playerInfo.canComputerPlay)))
		{
			if(features.levelROE)
				reader->skipUnused(6);
			if(features.levelAB)
				reader->skipUnused(6);
			if(features.levelSOD)
				reader->skipUnused(1);
			continue;
		}

		playerInfo.aiTactic = static_cast<EAiTactic::EAiTactic>(reader->readUInt8());

		if(features.levelSOD)
			reader->skipUnused(1); //TODO: check meaning?

		std::set<FactionID> allowedFactions;

		reader->readBitmaskFactions(allowedFactions, false);

		const bool isFactionRandom = playerInfo.isFactionRandom = reader->readBool();
		const bool allFactionsAllowed = isFactionRandom && allowedFactions.size() == features.factionsCount;

		if(!allFactionsAllowed)
			playerInfo.allowedFactions = allowedFactions;

		playerInfo.hasMainTown = reader->readBool();
		if(playerInfo.hasMainTown)
		{
			if(features.levelAB)
			{
				playerInfo.generateHeroAtMainTown = reader->readBool();
				reader->skipUnused(1); //TODO: check meaning?
			}
			else
			{
				playerInfo.generateHeroAtMainTown = true;
			}

			playerInfo.posOfMainTown = reader->readInt3();
		}

		playerInfo.hasRandomHero = reader->readBool();
		playerInfo.mainCustomHeroId = reader->readHero().getNum();

		if(playerInfo.mainCustomHeroId != -1)
		{
			playerInfo.mainCustomHeroPortrait = reader->readHeroPortrait();
			playerInfo.mainCustomHeroName = readLocalizedString(TextIdentifier("header", "player", i, "mainHeroName"));
		}

		if(features.levelAB)
		{
			reader->skipUnused(1); //TODO: check meaning?
			uint32_t heroCount = reader->readUInt32();
			for(int pp = 0; pp < heroCount; ++pp)
			{
				SHeroName vv;
				vv.heroId = reader->readUInt8();
				vv.heroName = readLocalizedString(TextIdentifier("header", "heroNames", vv.heroId));

				playerInfo.heroesNames.push_back(vv);
			}
		}
	}
}

enum class EVictoryConditionType : uint8_t
{
	ARTIFACT = 0,
	GATHERTROOP = 1,
	GATHERRESOURCE = 2,
	BUILDCITY = 3,
	BUILDGRAIL = 4,
	BEATHERO = 5,
	CAPTURECITY = 6,
	BEATMONSTER = 7,
	TAKEDWELLINGS = 8,
	TAKEMINES = 9,
	TRANSPORTITEM = 10,
	HOTA_ELIMINATE_ALL_MONSTERS = 11,
	HOTA_SURVIVE_FOR_DAYS = 12,
	WINSTANDARD = 255
};

enum class ELossConditionType : uint8_t
{
	LOSSCASTLE = 0,
	LOSSHERO = 1,
	TIMEEXPIRES = 2,
	LOSSSTANDARD = 255
};

void CMapLoaderH3M::readVictoryLossConditions()
{
	mapHeader->triggeredEvents.clear();
	mapHeader->victoryMessage.clear();
	mapHeader->defeatMessage.clear();

	auto vicCondition = static_cast<EVictoryConditionType>(reader->readUInt8());

	EventCondition victoryCondition(EventCondition::STANDARD_WIN);
	EventCondition defeatCondition(EventCondition::DAYS_WITHOUT_TOWN);
	defeatCondition.value = 7;

	TriggeredEvent standardVictory;
	standardVictory.effect.type = EventEffect::VICTORY;
	standardVictory.effect.toOtherMessage.appendTextID("core.genrltxt.5");
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill.appendTextID("core.genrltxt.659");
	standardVictory.trigger = EventExpression(victoryCondition);

	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage.appendTextID("core.genrltxt.8");
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill.appendTextID("core.genrltxt.7");
	standardDefeat.trigger = EventExpression(defeatCondition);

	// Specific victory conditions
	if(vicCondition == EVictoryConditionType::WINSTANDARD)
	{
		// create normal condition
		mapHeader->triggeredEvents.push_back(standardVictory);
		mapHeader->victoryIconIndex = 11;
		mapHeader->victoryMessage.appendTextID("core.vcdesc.0");
	}
	else
	{
		TriggeredEvent specialVictory;
		specialVictory.effect.type = EventEffect::VICTORY;
		specialVictory.identifier = "specialVictory";
		specialVictory.description.clear(); // TODO: display in quest window

		mapHeader->victoryIconIndex = static_cast<ui16>(vicCondition);

		bool allowNormalVictory = reader->readBool();
		bool appliesToAI = reader->readBool();

		if(allowNormalVictory)
		{
			size_t playersOnMap = boost::range::count_if(
				mapHeader->players,
				[](const PlayerInfo & info)
				{
					return info.canAnyonePlay();
				}
			);

			if(playersOnMap == 1)
			{
				logGlobal->warn("Map %s: Only one player exists, but normal victory allowed!", mapName);
				allowNormalVictory = false; // makes sense? Not much. Works as H3? Yes!
			}
		}

		switch(vicCondition)
		{
			case EVictoryConditionType::ARTIFACT:
			{
				EventCondition cond(EventCondition::HAVE_ARTIFACT);
				cond.objectType = reader->readArtifact();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.281");
				specialVictory.onFulfill.appendTextID("core.genrltxt.280");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.1");
				break;
			}
			case EVictoryConditionType::GATHERTROOP:
			{
				EventCondition cond(EventCondition::HAVE_CREATURES);
				cond.objectType = reader->readCreature();
				cond.value = reader->readInt32();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.277");
				specialVictory.onFulfill.appendTextID("core.genrltxt.6");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.2");
				break;
			}
			case EVictoryConditionType::GATHERRESOURCE:
			{
				EventCondition cond(EventCondition::HAVE_RESOURCES);
				cond.objectType = reader->readUInt8();
				cond.value = reader->readInt32();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.279");
				specialVictory.onFulfill.appendTextID("core.genrltxt.278");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.3");
				break;
			}
			case EVictoryConditionType::BUILDCITY:
			{
				EventExpression::OperatorAll oper;
				EventCondition cond(EventCondition::HAVE_BUILDING);
				cond.position = reader->readInt3();
				cond.objectType = BuildingID::TOWN_HALL + reader->readUInt8();
				oper.expressions.emplace_back(cond);
				cond.objectType = BuildingID::FORT + reader->readUInt8();
				oper.expressions.emplace_back(cond);

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.283");
				specialVictory.onFulfill.appendTextID("core.genrltxt.282");
				specialVictory.trigger = EventExpression(oper);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.4");
				break;
			}
			case EVictoryConditionType::BUILDGRAIL:
			{
				EventCondition cond(EventCondition::HAVE_BUILDING);
				cond.objectType = BuildingID::GRAIL;
				cond.position = reader->readInt3();
				if(cond.position.z > 2)
					cond.position = int3(-1, -1, -1);

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.285");
				specialVictory.onFulfill.appendTextID("core.genrltxt.284");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.5");
				break;
			}
			case EVictoryConditionType::BEATHERO:
			{
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = Obj::HERO;
				cond.position = reader->readInt3();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.253");
				specialVictory.onFulfill.appendTextID("core.genrltxt.252");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.6");
				break;
			}
			case EVictoryConditionType::CAPTURECITY:
			{
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::TOWN;
				cond.position = reader->readInt3();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.250");
				specialVictory.onFulfill.appendTextID("core.genrltxt.249");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.7");
				break;
			}
			case EVictoryConditionType::BEATMONSTER:
			{
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = Obj::MONSTER;
				cond.position = reader->readInt3();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.287");
				specialVictory.onFulfill.appendTextID("core.genrltxt.286");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.8");
				break;
			}
			case EVictoryConditionType::TAKEDWELLINGS:
			{
				EventExpression::OperatorAll oper;
				oper.expressions.emplace_back(EventCondition(EventCondition::CONTROL, 0, Obj::CREATURE_GENERATOR1));
				oper.expressions.emplace_back(EventCondition(EventCondition::CONTROL, 0, Obj::CREATURE_GENERATOR4));

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.289");
				specialVictory.onFulfill.appendTextID("core.genrltxt.288");
				specialVictory.trigger = EventExpression(oper);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.9");
				break;
			}
			case EVictoryConditionType::TAKEMINES:
			{
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::MINE;

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.291");
				specialVictory.onFulfill.appendTextID("core.genrltxt.290");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.10");
				break;
			}
			case EVictoryConditionType::TRANSPORTITEM:
			{
				EventCondition cond(EventCondition::TRANSPORT);
				cond.objectType = reader->readUInt8();
				cond.position = reader->readInt3();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.293");
				specialVictory.onFulfill.appendTextID("core.genrltxt.292");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.11");
				break;
			}
			case EVictoryConditionType::HOTA_ELIMINATE_ALL_MONSTERS:
			{
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = Obj::MONSTER;

				specialVictory.effect.toOtherMessage.appendTextID("vcmi.map.victoryCondition.eliminateMonsters.toOthers");
				specialVictory.onFulfill.appendTextID("vcmi.map.victoryCondition.eliminateMonsters.toSelf");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.12");
				mapHeader->victoryIconIndex = 12;
				break;
			}
			case EVictoryConditionType::HOTA_SURVIVE_FOR_DAYS:
			{
				EventCondition cond(EventCondition::DAYS_PASSED);
				cond.value = reader->readUInt32();

				specialVictory.effect.toOtherMessage.appendTextID("vcmi.map.victoryCondition.daysPassed.toOthers");
				specialVictory.onFulfill.appendTextID("vcmi.map.victoryCondition.daysPassed.toSelf");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.13");
				mapHeader->victoryIconIndex = 13;
				break;
			}
			default:
				assert(0);
		}

		// if condition is human-only turn it into following construction: AllOf(human, condition)
		if(!appliesToAI)
		{
			EventExpression::OperatorAll oper;
			EventCondition notAI(EventCondition::IS_HUMAN);
			notAI.value = 1;
			oper.expressions.emplace_back(notAI);
			oper.expressions.push_back(specialVictory.trigger.get());
			specialVictory.trigger = EventExpression(oper);
		}

		// if normal victory allowed - add one more quest
		if(allowNormalVictory)
		{
			mapHeader->victoryMessage.appendRawString(" / ");
			mapHeader->victoryMessage.appendTextID("core.vcdesc.0");
			mapHeader->triggeredEvents.push_back(standardVictory);
		}
		mapHeader->triggeredEvents.push_back(specialVictory);
	}

	// Read loss conditions
	auto lossCond = static_cast<ELossConditionType>(reader->readUInt8());
	if(lossCond == ELossConditionType::LOSSSTANDARD)
	{
		mapHeader->defeatIconIndex = 3;
		mapHeader->defeatMessage.appendTextID("core.lcdesc.0");
	}
	else
	{
		TriggeredEvent specialDefeat;
		specialDefeat.effect.type = EventEffect::DEFEAT;
		specialDefeat.effect.toOtherMessage.appendTextID("core.genrltxt.5");
		specialDefeat.identifier = "specialDefeat";
		specialDefeat.description.clear(); // TODO: display in quest window

		mapHeader->defeatIconIndex = static_cast<ui16>(lossCond);

		switch(lossCond)
		{
			case ELossConditionType::LOSSCASTLE:
			{
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::TOWN;
				cond.position = reader->readInt3();

				noneOf.expressions.emplace_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.251");
				specialDefeat.trigger = EventExpression(noneOf);

				mapHeader->defeatMessage.appendTextID("core.lcdesc.1");
				break;
			}
			case ELossConditionType::LOSSHERO:
			{
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::HERO;
				cond.position = reader->readInt3();

				noneOf.expressions.emplace_back(cond);
				specialDefeat.onFulfill.appendTextID("core.genrltxt.253");
				specialDefeat.trigger = EventExpression(noneOf);

				mapHeader->defeatMessage.appendTextID("core.lcdesc.2");
				break;
			}
			case ELossConditionType::TIMEEXPIRES:
			{
				EventCondition cond(EventCondition::DAYS_PASSED);
				cond.value = reader->readUInt16();

				specialDefeat.onFulfill.appendTextID("core.genrltxt.254");
				specialDefeat.trigger = EventExpression(cond);

				mapHeader->defeatMessage.appendTextID("core.lcdesc.3");
				break;
			}
		}
		// turn simple loss condition into complete one that can be evaluated later:
		// - any of :
		//   - days without town: 7
		//   - all of:
		//     - is human
		//     - (expression)

		EventExpression::OperatorAll allOf;
		EventCondition isHuman(EventCondition::IS_HUMAN);
		isHuman.value = 1;

		allOf.expressions.emplace_back(isHuman);
		allOf.expressions.push_back(specialDefeat.trigger.get());
		specialDefeat.trigger = EventExpression(allOf);

		mapHeader->triggeredEvents.push_back(specialDefeat);
	}
	mapHeader->triggeredEvents.push_back(standardDefeat);
}

void CMapLoaderH3M::readTeamInfo()
{
	mapHeader->howManyTeams = reader->readUInt8();
	if(mapHeader->howManyTeams > 0)
	{
		// Teams
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
			mapHeader->players[i].team = TeamID(reader->readUInt8());
	}
	else
	{
		// No alliances
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
			if(mapHeader->players[i].canComputerPlay || mapHeader->players[i].canHumanPlay)
				mapHeader->players[i].team = TeamID(mapHeader->howManyTeams++);
	}
}

void CMapLoaderH3M::readAllowedHeroes()
{
	mapHeader->allowedHeroes = VLC->heroh->getDefaultAllowed();

	if(features.levelHOTA0)
		reader->readBitmaskHeroesSized(mapHeader->allowedHeroes, false);
	else
		reader->readBitmaskHeroes(mapHeader->allowedHeroes, false);

	if(features.levelAB)
	{
		uint32_t placeholdersQty = reader->readUInt32();

		for (uint32_t i = 0; i < placeholdersQty; ++i)
		{
			auto heroID = reader->readHero();
			mapHeader->reservedCampaignHeroes.push_back(heroID);
		}
	}
}

void CMapLoaderH3M::readDisposedHeroes()
{
	// Reading disposed heroes (20 bytes)
	if(features.levelSOD)
	{
		ui8 disp = reader->readUInt8();
		map->disposedHeroes.resize(disp);
		for(int g = 0; g < disp; ++g)
		{
			map->disposedHeroes[g].heroId = reader->readHero().getNum();
			map->disposedHeroes[g].portrait = reader->readHeroPortrait();
			map->disposedHeroes[g].name = readLocalizedString(TextIdentifier("header", "heroes", map->disposedHeroes[g].heroId));
			map->disposedHeroes[g].players = reader->readUInt8();
		}
	}
}

void CMapLoaderH3M::readMapOptions()
{
	//omitting NULLS
	reader->skipZero(31);

	if(features.levelHOTA0)
	{
		//TODO: HotA
		bool allowSpecialMonths = reader->readBool();
		if(!allowSpecialMonths)
			logGlobal->warn("Map '%s': Option 'allow special months' is not implemented!", mapName);
		reader->skipZero(3);
	}

	if(features.levelHOTA1)
	{
		// Unknown, may be another "sized bitmap", e.g
		// 4 bytes - size of bitmap (16)
		// 2 bytes - bitmap data (16 bits / 2 bytes)
		[[maybe_unused]] uint8_t unknownConstant = reader->readUInt8();
		assert(unknownConstant == 16);
		reader->skipZero(5);
	}

	if(features.levelHOTA3)
	{
		//TODO: HotA
		int32_t roundLimit = reader->readInt32();
		if(roundLimit != -1)
			logGlobal->warn("Map '%s': roundLimit of %d is not implemented!", mapName, roundLimit);
	}
}

void CMapLoaderH3M::readAllowedArtifacts()
{
	map->allowedArtifact = VLC->arth->getDefaultAllowed();

	if(features.levelAB)
	{
		if(features.levelHOTA0)
			reader->readBitmaskArtifactsSized(map->allowedArtifact, true);
		else
			reader->readBitmaskArtifacts(map->allowedArtifact, true);
	}

	// ban combo artifacts
	if(!features.levelSOD)
	{
		for(CArtifact * artifact : VLC->arth->objects)
			if(artifact->constituents)
				map->allowedArtifact[artifact->getId()] = false;
	}

	if(!features.levelAB)
	{
		map->allowedArtifact[ArtifactID::VIAL_OF_DRAGON_BLOOD] = false;
		map->allowedArtifact[ArtifactID::ARMAGEDDONS_BLADE] = false;
	}

	// Messy, but needed
	for(TriggeredEvent & event : map->triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			if(cond.condition == EventCondition::HAVE_ARTIFACT || cond.condition == EventCondition::TRANSPORT)
			{
				map->allowedArtifact[cond.objectType] = false;
			}
			return cond;
		};

		event.trigger = event.trigger.morph(patcher);
	}
}

void CMapLoaderH3M::readAllowedSpellsAbilities()
{
	map->allowedSpell = VLC->spellh->getDefaultAllowed();
	map->allowedAbilities = VLC->skillh->getDefaultAllowed();

	if(features.levelSOD)
	{
		reader->readBitmaskSpells(map->allowedSpell, true);
		reader->readBitmaskSkills(map->allowedAbilities, true);
	}
}

void CMapLoaderH3M::readRumors()
{
	uint32_t rumorsCount = reader->readUInt32();
	assert(rumorsCount < 1000); // sanity check

	for(int it = 0; it < rumorsCount; it++)
	{
		Rumor ourRumor;
		ourRumor.name = readBasicString();
		ourRumor.text = readLocalizedString(TextIdentifier("header", "rumor", it, "text"));
		map->rumors.push_back(ourRumor);
	}
}

void CMapLoaderH3M::readPredefinedHeroes()
{
	if(!features.levelSOD)
		return;

	uint32_t heroesCount = features.heroesCount;

	if(features.levelHOTA0)
		heroesCount = reader->readUInt32();

	assert(heroesCount <= features.heroesCount);

	for(int heroID = 0; heroID < heroesCount; heroID++)
	{
		bool custom = reader->readBool();
		if(!custom)
			continue;

		auto * hero = new CGHeroInstance();
		hero->ID = Obj::HERO;
		hero->subID = heroID;

		bool hasExp = reader->readBool();
		if(hasExp)
		{
			hero->exp = reader->readUInt32();
		}
		else
		{
			hero->exp = 0;
		}

		bool hasSecSkills = reader->readBool();
		if(hasSecSkills)
		{
			uint32_t howMany = reader->readUInt32();
			hero->secSkills.resize(howMany);
			for(int yy = 0; yy < howMany; ++yy)
			{
				hero->secSkills[yy].first = reader->readSkill();
				hero->secSkills[yy].second = reader->readUInt8();
			}
		}

		loadArtifactsOfHero(hero);

		bool hasCustomBio = reader->readBool();
		if(hasCustomBio)
			hero->biographyCustom = readLocalizedString(TextIdentifier("heroes", heroID, "biography"));

		// 0xFF is default, 00 male, 01 female
		hero->gender = static_cast<EHeroGender>(reader->readUInt8());
		assert(hero->gender == EHeroGender::MALE || hero->gender == EHeroGender::FEMALE || hero->gender == EHeroGender::DEFAULT);

		bool hasCustomSpells = reader->readBool();
		if(hasCustomSpells)
			reader->readBitmaskSpells(hero->spells, false);

		bool hasCustomPrimSkills = reader->readBool();
		if(hasCustomPrimSkills)
		{
			for(int skillID = 0; skillID < GameConstants::PRIMARY_SKILLS; skillID++)
			{
				hero->pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(skillID), reader->readUInt8());
			}
		}
		map->predefinedHeroes.emplace_back(hero);

		logGlobal->debug("Map '%s': Hero predefined in map: %s", mapName, VLC->heroh->getByIndex(hero->subID)->getJsonKey());
	}
}

void CMapLoaderH3M::loadArtifactsOfHero(CGHeroInstance * hero)
{
	bool hasArtSet = reader->readBool();

	// True if artifact set is not default (hero has some artifacts)
	if(!hasArtSet)
		return;

	if(!hero->artifactsWorn.empty() || !hero->artifactsInBackpack.empty())
	{
		logGlobal->warn("Hero %s at %s has set artifacts twice (in map properties and on adventure map instance). Using the latter set...", hero->getNameTranslated(), hero->pos.toString());

		hero->artifactsInBackpack.clear();
		while(!hero->artifactsWorn.empty())
			hero->eraseArtSlot(hero->artifactsWorn.begin()->first);
	}

	for(int i = 0; i < features.artifactSlotsCount; i++)
		loadArtifactToSlot(hero, i);

	// bag artifacts
	// number of artifacts in hero's bag
	int amount = reader->readUInt16();
	for(int i = 0; i < amount; ++i)
	{
		loadArtifactToSlot(hero, GameConstants::BACKPACK_START + static_cast<int>(hero->artifactsInBackpack.size()));
	}
}

bool CMapLoaderH3M::loadArtifactToSlot(CGHeroInstance * hero, int slot)
{
	ArtifactID artifactID = reader->readArtifact();

	if(artifactID == ArtifactID::NONE)
		return false;

	const Artifact * art = artifactID.toArtifact(VLC->artifacts());

	if(!art)
	{
		logGlobal->warn("Map '%s': Invalid artifact in hero's backpack, ignoring...", mapName);
		return false;
	}

	if(art->isBig() && slot >= GameConstants::BACKPACK_START)
	{
		logGlobal->warn("Map '%s': A big artifact (war machine) in hero's backpack, ignoring...", mapName);
		return false;
	}

	// H3 bug workaround - Enemy hero on 3rd scenario of Good1.h3c campaign ("Long Live The Queen")
	// He has Shackles of War (normally - MISC slot artifact) in LEFT_HAND slot set in editor
	// Artifact seems to be missing in game, so skip artifacts that don't fit target slot
	auto * artifact = ArtifactUtils::createArtifact(map, artifactID);
	auto artifactPos = ArtifactPosition(slot);
	if(artifact->canBePutAt(ArtifactLocation(hero, artifactPos)))
	{
		hero->putArtifact(artifactPos, artifact);
	}
	else
	{
		logGlobal->warn("Map '%s': Artifact '%s' can't be put at the slot %d", mapName, artifact->artType->getNameTranslated(), slot);
		return false;
	}

	return true;
}

void CMapLoaderH3M::readTerrain()
{
	map->initTerrain();

	// Read terrain
	int3 pos;
	for(pos.z = 0; pos.z < map->levels(); ++pos.z)
	{
		//OH3 format is [z][y][x]
		for(pos.y = 0; pos.y < map->height; pos.y++)
		{
			for(pos.x = 0; pos.x < map->width; pos.x++)
			{
				auto & tile = map->getTile(pos);
				tile.terType = VLC->terrainTypeHandler->getById(reader->readTerrain());
				tile.terView = reader->readUInt8();
				tile.riverType = VLC->riverTypeHandler->getById(reader->readRiver());
				tile.riverDir = reader->readUInt8();
				tile.roadType = VLC->roadTypeHandler->getById(reader->readRoad());
				tile.roadDir = reader->readUInt8();
				tile.extTileFlags = reader->readUInt8();
				tile.blocked = !tile.terType->isPassable();
				tile.visitable = false;

				assert(tile.terType->getId() != ETerrainId::NONE);
			}
		}
	}
}

void CMapLoaderH3M::readObjectTemplates()
{
	uint32_t defAmount = reader->readUInt32();

	templates.reserve(defAmount);

	// Read custom defs
	for(int defID = 0; defID < defAmount; ++defID)
	{
		auto tmpl = reader->readObjectTemplate();
		templates.push_back(tmpl);

		if (!CResourceHandler::get()->existsResource(ResourceID( "SPRITES/" + tmpl->animationFile, EResType::ANIMATION)))
			logMod->warn("Template animation %s of type (%d %d) is missing!", tmpl->animationFile, tmpl->id, tmpl->subid );
	}
}

CGObjectInstance * CMapLoaderH3M::readEvent(const int3 & mapPosition)
{
	auto * object = new CGEvent();

	readBoxContent(object, mapPosition);

	object->availableFor = reader->readUInt8();
	object->computerActivate = reader->readBool();
	object->removeAfterVisit = reader->readBool();

	reader->skipZero(4);

	if(features.levelHOTA3)
		object->humanActivate = reader->readBool();
	else
		object->humanActivate = true;

	return object;
}

CGObjectInstance * CMapLoaderH3M::readPandora(const int3 & mapPosition)
{
	auto * object = new CGPandoraBox();
	readBoxContent(object, mapPosition);
	return object;
}

void CMapLoaderH3M::readBoxContent(CGPandoraBox * object, const int3 & mapPosition)
{
	readMessageAndGuards(object->message, object, mapPosition);

	object->gainedExp = reader->readUInt32();
	object->manaDiff = reader->readInt32();
	object->moraleDiff = reader->readInt8();
	object->luckDiff = reader->readInt8();

	reader->readResourses(object->resources);

	object->primskills.resize(GameConstants::PRIMARY_SKILLS);
	for(int x = 0; x < GameConstants::PRIMARY_SKILLS; ++x)
		object->primskills[x] = static_cast<PrimarySkill::PrimarySkill>(reader->readUInt8());

	int gabn = reader->readUInt8(); //number of gained abilities
	for(int oo = 0; oo < gabn; ++oo)
	{
		object->abilities.emplace_back(reader->readSkill());
		object->abilityLevels.push_back(reader->readUInt8());
	}
	int gart = reader->readUInt8(); //number of gained artifacts
	for(int oo = 0; oo < gart; ++oo)
		object->artifacts.emplace_back(reader->readArtifact());

	int gspel = reader->readUInt8(); //number of gained spells
	for(int oo = 0; oo < gspel; ++oo)
		object->spells.emplace_back(reader->readSpell());

	int gcre = reader->readUInt8(); //number of gained creatures
	readCreatureSet(&object->creatures, gcre);
	reader->skipZero(8);
}

CGObjectInstance * CMapLoaderH3M::readMonster(const int3 & mapPosition, const ObjectInstanceID & objectInstanceID)
{
	auto * object = new CGCreature();

	if(features.levelAB)
	{
		object->identifier = reader->readUInt32();
		map->questIdentifierToId[object->identifier] = objectInstanceID;
	}

	auto * hlp = new CStackInstance();
	hlp->count = reader->readUInt16();

	//type will be set during initialization
	object->putStack(SlotID(0), hlp);

	object->character = reader->readInt8();

	bool hasMessage = reader->readBool();
	if(hasMessage)
	{
		object->message = readLocalizedString(TextIdentifier("monster", mapPosition.x, mapPosition.y, mapPosition.z, "message"));
		reader->readResourses(object->resources);
		object->gainedArtifact = reader->readArtifact();
	}
	object->neverFlees = reader->readBool();
	object->notGrowingTeam = reader->readBool();
	reader->skipZero(2);

	if(features.levelHOTA3)
	{
		//TODO: HotA
		int32_t agressionExact = reader->readInt32(); // -1 = default, 1-10 = possible values range
		bool joinOnlyForMoney = reader->readBool(); // if true, monsters will only join for money
		int32_t joinPercent = reader->readInt32(); // 100 = default, percent of monsters that will join on succesfull agression check
		int32_t upgradedStack = reader->readInt32(); // Presence of upgraded stack, -1 = random, 0 = never, 1 = always
		int32_t stacksCount = reader->readInt32(); // TODO: check possible values. How many creature stacks will be present on battlefield, -1 = default

		if(agressionExact != -1 || joinOnlyForMoney || joinPercent != 100 || upgradedStack != -1 || stacksCount != -1)
			logGlobal->warn(
				"Map '%s': Wandering monsters %s settings %d %d %d %d %d are not implemeted!",
				mapName,
				mapPosition.toString(),
				agressionExact,
				int(joinOnlyForMoney),
				joinPercent,
				upgradedStack,
				stacksCount
			);
	}

	return object;
}

CGObjectInstance * CMapLoaderH3M::readSign(const int3 & mapPosition)
{
	auto * object = new CGSignBottle();
	object->message = readLocalizedString(TextIdentifier("sign", mapPosition.x, mapPosition.y, mapPosition.z, "message"));
	reader->skipZero(4);
	return object;
}

CGObjectInstance * CMapLoaderH3M::readWitchHut()
{
	auto * object = new CGWitchHut();

	// AB and later maps have allowed abilities defined in H3M
	if(features.levelAB)
	{
		reader->readBitmaskSkills(object->allowedAbilities, false);

		if(object->allowedAbilities.size() != 1)
		{
			auto defaultAllowed = VLC->skillh->getDefaultAllowed();

			for(int skillID = 0; skillID < VLC->skillh->size(); ++skillID)
				if(defaultAllowed[skillID])
					object->allowedAbilities.insert(SecondarySkill(skillID));
		}
	}
	return object;
}

CGObjectInstance * CMapLoaderH3M::readScholar()
{
	auto * object = new CGScholar();
	object->bonusType = static_cast<CGScholar::EBonusType>(reader->readUInt8());
	object->bonusID = reader->readUInt8();
	reader->skipZero(6);
	return object;
}

CGObjectInstance * CMapLoaderH3M::readGarrison(const int3 & mapPosition)
{
	auto * object = new CGGarrison();

	setOwnerAndValidate(mapPosition, object, reader->readPlayer32());
	readCreatureSet(object, 7);
	if(features.levelAB)
		object->removableUnits = reader->readBool();
	else
		object->removableUnits = true;

	reader->skipZero(8);
	return object;
}

CGObjectInstance * CMapLoaderH3M::readArtifact(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto artID = ArtifactID::NONE; //random, set later
	int spellID = -1;
	auto * object = new CGArtifact();

	readMessageAndGuards(object->message, object, mapPosition);

	if(objectTemplate->id == Obj::SPELL_SCROLL)
	{
		spellID = reader->readSpell32();
		artID = ArtifactID::SPELL_SCROLL;
	}
	else if(objectTemplate->id == Obj::ARTIFACT)
	{
		//specific artifact
		artID = ArtifactID(objectTemplate->subid);
	}

	object->storedArtifact = ArtifactUtils::createArtifact(map, artID, spellID);
	return object;
}

CGObjectInstance * CMapLoaderH3M::readResource(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto * object = new CGResource();

	readMessageAndGuards(object->message, object, mapPosition);

	object->amount = reader->readUInt32();
	if(objectTemplate->subid == GameResID(EGameResID::GOLD))
	{
		// Gold is multiplied by 100.
		object->amount *= 100;
	}
	reader->skipZero(4);
	return object;
}

CGObjectInstance * CMapLoaderH3M::readMine(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto * object = new CGMine();
	if(objectTemplate->subid < 7)
	{
		setOwnerAndValidate(mapPosition, object, reader->readPlayer32());
	}
	else
	{
		object->setOwner(PlayerColor::NEUTRAL);
		reader->readBitmaskResources(object->abandonedMineResources, false);
	}
	return object;
}

CGObjectInstance * CMapLoaderH3M::readDwelling(const int3 & position)
{
	auto * object = new CGDwelling();
	setOwnerAndValidate(position, object, reader->readPlayer32());
	return object;
}

CGObjectInstance * CMapLoaderH3M::readDwellingRandom(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto * object = new CGDwelling();

	CSpecObjInfo * spec = nullptr;
	switch(objectTemplate->id)
	{
		case Obj::RANDOM_DWELLING:
			spec = new CCreGenLeveledCastleInfo();
			break;
		case Obj::RANDOM_DWELLING_LVL:
			spec = new CCreGenAsCastleInfo();
			break;
		case Obj::RANDOM_DWELLING_FACTION:
			spec = new CCreGenLeveledInfo();
			break;
		default:
			throw std::runtime_error("Invalid random dwelling format");
	}
	spec->owner = object;

	setOwnerAndValidate(mapPosition, object, reader->readPlayer32());

	//216 and 217
	if(auto * castleSpec = dynamic_cast<CCreGenAsCastleInfo *>(spec))
	{
		castleSpec->instanceId = "";
		castleSpec->identifier = reader->readUInt32();
		if(!castleSpec->identifier)
		{
			castleSpec->asCastle = false;
			const int MASK_SIZE = 8;
			ui8 mask[2];
			mask[0] = reader->readUInt8();
			mask[1] = reader->readUInt8();

			castleSpec->allowedFactions.clear();
			castleSpec->allowedFactions.resize(VLC->townh->size(), false);

			for(int i = 0; i < MASK_SIZE; i++)
				castleSpec->allowedFactions[i] = ((mask[0] & (1 << i)) > 0);

			for(int i = 0; i < (GameConstants::F_NUMBER - MASK_SIZE); i++)
				castleSpec->allowedFactions[i + MASK_SIZE] = ((mask[1] & (1 << i)) > 0);
		}
		else
		{
			castleSpec->asCastle = true;
		}
	}

	//216 and 218
	if(auto * lvlSpec = dynamic_cast<CCreGenLeveledInfo *>(spec))
	{
		lvlSpec->minLevel = std::max(reader->readUInt8(), static_cast<ui8>(1));
		lvlSpec->maxLevel = std::min(reader->readUInt8(), static_cast<ui8>(7));
	}
	object->info = spec;
	return object;
}

CGObjectInstance * CMapLoaderH3M::readShrine()
{
	auto * object = new CGShrine();
	object->spell = reader->readSpell32();
	return object;
}

CGObjectInstance * CMapLoaderH3M::readHeroPlaceholder(const int3 & mapPosition)
{
	auto * object = new CGHeroPlaceholder();

	setOwnerAndValidate(mapPosition, object, reader->readPlayer());

	HeroTypeID htid = reader->readHero(); //hero type id
	object->subID = htid.getNum();

	if(htid.getNum() == -1)
	{
		object->power = reader->readUInt8();
		logGlobal->debug("Map '%s': Hero placeholder: by power at %s, owned by %s", mapName, mapPosition.toString(), object->getOwner().getStr());
	}
	else
	{
		object->power = 0;
		logGlobal->debug("Map '%s': Hero placeholder: %s at %s, owned by %s", mapName, VLC->heroh->getById(htid)->getJsonKey(), mapPosition.toString(), object->getOwner().getStr());
	}

	return object;
}

CGObjectInstance * CMapLoaderH3M::readGrail(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	if (objectTemplate->subid < 1000)
	{
		map->grailPos = mapPosition;
		map->grailRadius = reader->readInt32();
	}
	else
	{
		// Battle location for arena mode in HotA
		logGlobal->warn("Map '%s': Arena mode is not supported!", mapName);
	}
	return nullptr;
}

CGObjectInstance * CMapLoaderH3M::readGeneric(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	if(VLC->objtypeh->knownSubObjects(objectTemplate->id).count(objectTemplate->subid))
		return VLC->objtypeh->getHandlerFor(objectTemplate->id, objectTemplate->subid)->create(objectTemplate);

	logGlobal->warn("Map '%s': Unrecognized object %d:%d ('%s') at %s found!", mapName, objectTemplate->id.toEnum(), objectTemplate->subid, objectTemplate->animationFile, mapPosition.toString());
	return new CGObjectInstance();
}

CGObjectInstance * CMapLoaderH3M::readPyramid(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	if(objectTemplate->subid == 0)
		return new CBank();

	return new CGObjectInstance();
}

CGObjectInstance * CMapLoaderH3M::readQuestGuard(const int3 & mapPosition)
{
	auto * guard = new CGQuestGuard();
	readQuest(guard, mapPosition);
	return guard;
}

CGObjectInstance * CMapLoaderH3M::readShipyard(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto * object = readGeneric(mapPosition, objectTemplate);
	setOwnerAndValidate(mapPosition, object, reader->readPlayer32());
	return object;
}

CGObjectInstance * CMapLoaderH3M::readLighthouse(const int3 & mapPosition)
{
	auto * object = new CGLighthouse();
	setOwnerAndValidate(mapPosition, object, reader->readPlayer32());
	return object;
}

CGObjectInstance * CMapLoaderH3M::readBank(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	if(features.levelHOTA3)
	{
		//TODO: HotA
		// index of guards preset. -1 = random, 0-4 = index of possible guards settings
		int32_t guardsPresetIndex = reader->readInt32();

		// presence of upgraded stack: -1 = random, 0 = never, 1 = always
		int8_t upgradedStackPresence = reader->readInt8();

		assert(vstd::iswithin(guardsPresetIndex, -1, 4));
		assert(vstd::iswithin(upgradedStackPresence, -1, 1));

		// list of possible artifacts in reward
		// - if list is empty, artifacts are either not present in reward or random
		// - if non-empty, then list always have same number of elements as number of artifacts in bank
		// - ArtifactID::NONE indictates random artifact, other values indicate artifact that should be used as reward
		std::vector<ArtifactID> artifacts;
		int artNumber = reader->readUInt32();
		for(int yy = 0; yy < artNumber; ++yy)
		{
			artifacts.push_back(reader->readArtifact32());
		}

		if(guardsPresetIndex != -1 || upgradedStackPresence != -1 || !artifacts.empty())
			logGlobal->warn(
				"Map '%s: creature bank at %s settings %d %d %d are not implemented!",
				mapName,
				mapPosition.toString(),
				guardsPresetIndex,
				int(upgradedStackPresence),
				artifacts.size()
			);
	}

	return readGeneric(mapPosition, objectTemplate);
}

CGObjectInstance * CMapLoaderH3M::readObject(std::shared_ptr<const ObjectTemplate> objectTemplate, const int3 & mapPosition, const ObjectInstanceID & objectInstanceID)
{
	switch(objectTemplate->id)
	{
		case Obj::EVENT:
			return readEvent(mapPosition);

		case Obj::HERO:
		case Obj::RANDOM_HERO:
		case Obj::PRISON:
			return readHero(mapPosition, objectInstanceID);

		case Obj::MONSTER:
		case Obj::RANDOM_MONSTER:
		case Obj::RANDOM_MONSTER_L1:
		case Obj::RANDOM_MONSTER_L2:
		case Obj::RANDOM_MONSTER_L3:
		case Obj::RANDOM_MONSTER_L4:
		case Obj::RANDOM_MONSTER_L5:
		case Obj::RANDOM_MONSTER_L6:
		case Obj::RANDOM_MONSTER_L7:
			return readMonster(mapPosition, objectInstanceID);

		case Obj::OCEAN_BOTTLE:
		case Obj::SIGN:
			return readSign(mapPosition);

		case Obj::SEER_HUT:
			return readSeerHut(mapPosition);

		case Obj::WITCH_HUT:
			return readWitchHut();
		case Obj::SCHOLAR:
			return readScholar();

		case Obj::GARRISON:
		case Obj::GARRISON2:
			return readGarrison(mapPosition);

		case Obj::ARTIFACT:
		case Obj::RANDOM_ART:
		case Obj::RANDOM_TREASURE_ART:
		case Obj::RANDOM_MINOR_ART:
		case Obj::RANDOM_MAJOR_ART:
		case Obj::RANDOM_RELIC_ART:
		case Obj::SPELL_SCROLL:
			return readArtifact(mapPosition, objectTemplate);

		case Obj::RANDOM_RESOURCE:
		case Obj::RESOURCE:
			return readResource(mapPosition, objectTemplate);
		case Obj::RANDOM_TOWN:
		case Obj::TOWN:
			return readTown(mapPosition, objectTemplate);

		case Obj::MINE:
		case Obj::ABANDONED_MINE:
			return readMine(mapPosition, objectTemplate);

		case Obj::CREATURE_GENERATOR1:
		case Obj::CREATURE_GENERATOR2:
		case Obj::CREATURE_GENERATOR3:
		case Obj::CREATURE_GENERATOR4:
			return readDwelling(mapPosition);

		case Obj::SHRINE_OF_MAGIC_INCANTATION:
		case Obj::SHRINE_OF_MAGIC_GESTURE:
		case Obj::SHRINE_OF_MAGIC_THOUGHT:
			return readShrine();

		case Obj::PANDORAS_BOX:
			return readPandora(mapPosition);

		case Obj::GRAIL:
			return readGrail(mapPosition, objectTemplate);

		case Obj::RANDOM_DWELLING:
		case Obj::RANDOM_DWELLING_LVL:
		case Obj::RANDOM_DWELLING_FACTION:
			return readDwellingRandom(mapPosition, objectTemplate);

		case Obj::QUEST_GUARD:
			return readQuestGuard(mapPosition);

		case Obj::SHIPYARD:
			return readShipyard(mapPosition, objectTemplate);

		case Obj::HERO_PLACEHOLDER:
			return readHeroPlaceholder(mapPosition);

		case Obj::PYRAMID:
			return readPyramid(mapPosition, objectTemplate);

		case Obj::LIGHTHOUSE:
			return readLighthouse(mapPosition);

		case Obj::CREATURE_BANK:
		case Obj::DERELICT_SHIP:
		case Obj::DRAGON_UTOPIA:
		case Obj::CRYPT:
		case Obj::SHIPWRECK:
			return readBank(mapPosition, objectTemplate);

		default: //any other object
			return readGeneric(mapPosition, objectTemplate);
	}
}

void CMapLoaderH3M::readObjects()
{
	uint32_t objectsCount = reader->readUInt32();

	for(uint32_t i = 0; i < objectsCount; ++i)
	{
		int3 mapPosition = reader->readInt3();

		uint32_t defIndex = reader->readUInt32();
		ObjectInstanceID objectInstanceID = ObjectInstanceID(static_cast<si32>(map->objects.size()));

		std::shared_ptr<const ObjectTemplate> objectTemplate = templates.at(defIndex);
		reader->skipZero(5);

		CGObjectInstance * newObject = readObject(objectTemplate, mapPosition, objectInstanceID);

		if(!newObject)
			continue;

		newObject->pos = mapPosition;
		newObject->ID = objectTemplate->id;
		newObject->id = objectInstanceID;
		if(newObject->ID != Obj::HERO && newObject->ID != Obj::HERO_PLACEHOLDER && newObject->ID != Obj::PRISON)
		{
			newObject->subID = objectTemplate->subid;
		}
		newObject->appearance = objectTemplate;
		assert(objectInstanceID == ObjectInstanceID((si32)map->objects.size()));

		{
			//TODO: define valid typeName and subtypeName for H3M maps
			//boost::format fmt("%s_%d");
			//fmt % nobj->typeName % nobj->id.getNum();
			boost::format fmt("obj_%d");
			fmt % newObject->id.getNum();
			newObject->instanceName = fmt.str();
		}
		map->addNewObject(newObject);
	}

	std::sort(
		map->heroesOnMap.begin(),
		map->heroesOnMap.end(),
		[](const ConstTransitivePtr<CGHeroInstance> & a, const ConstTransitivePtr<CGHeroInstance> & b)
		{
			return a->subID < b->subID;
		}
	);
}

void CMapLoaderH3M::readCreatureSet(CCreatureSet * out, int number)
{
	for(int index = 0; index < number; ++index)
	{
		CreatureID creatureID = reader->readCreature();
		int count = reader->readUInt16();

		// Empty slot
		if(creatureID == CreatureID::NONE)
			continue;

		auto * result = new CStackInstance();
		result->count = count;

		if(creatureID < CreatureID::NONE)
		{
			int value = -creatureID.getNum() - 2;
			assert(value >= 0 && value < 14);
			uint8_t level = value / 2;
			uint8_t upgrade = value % 2;

			//this will happen when random object has random army
			result->randomStack = CStackInstance::RandomStackInfo{level, upgrade};
		}
		else
		{
			result->setType(creatureID);
		}

		out->putStack(SlotID(index), result);
	}

	out->validTypes(true);
}

void CMapLoaderH3M::setOwnerAndValidate(const int3 & mapPosition, CGObjectInstance * object, const PlayerColor & owner)
{
	assert(owner.isValidPlayer() || owner == PlayerColor::NEUTRAL);

	if(owner == PlayerColor::NEUTRAL)
	{
		object->setOwner(PlayerColor::NEUTRAL);
		return;
	}

	if(!owner.isValidPlayer())
	{
		object->setOwner(PlayerColor::NEUTRAL);
		logGlobal->warn("Map '%s': Object at %s - owned by invalid player %d! Will be set to neutral!", mapName, mapPosition.toString(), int(owner.getNum()));
		return;
	}

	if(!mapHeader->players[owner.getNum()].canAnyonePlay())
	{
		object->setOwner(PlayerColor::NEUTRAL);
		logGlobal->warn("Map '%s': Object at %s - owned by non-existing player %d! Will be set to neutral!", mapName, mapPosition.toString(), int(owner.getNum())
		);
		return;
	}

	object->setOwner(owner);
}

CGObjectInstance * CMapLoaderH3M::readHero(const int3 & mapPosition, const ObjectInstanceID & objectInstanceID)
{
	auto * object = new CGHeroInstance();

	if(features.levelAB)
	{
		unsigned int identifier = reader->readUInt32();
		map->questIdentifierToId[identifier] = objectInstanceID;
	}

	PlayerColor owner = reader->readPlayer();
	object->subID = reader->readHero().getNum();

	//If hero of this type has been predefined, use that as a base.
	//Instance data will overwrite the predefined values where appropriate.
	for(auto & elem : map->predefinedHeroes)
	{
		if(elem->subID == object->subID)
		{
			logGlobal->debug("Hero %d will be taken from the predefined heroes list.", object->subID);
			delete object;
			object = elem;
			break;
		}
	}

	setOwnerAndValidate(mapPosition, object, owner);
	object->portrait = CGHeroInstance::UNINITIALIZED_PORTRAIT;

	for(auto & elem : map->disposedHeroes)
	{
		if(elem.heroId == object->subID)
		{
			object->nameCustom = elem.name;
			object->portrait = elem.portrait;
			break;
		}
	}

	bool hasName = reader->readBool();
	if(hasName)
		object->nameCustom = readLocalizedString(TextIdentifier("heroes", object->subID, "name"));

	if(features.levelSOD)
	{
		bool hasCustomExperience = reader->readBool();
		if(hasCustomExperience)
			object->exp = reader->readUInt32();
		else
			object->exp = CGHeroInstance::UNINITIALIZED_EXPERIENCE;
	}
	else
	{
		object->exp = reader->readUInt32();

		//0 means "not set" in <=AB maps
		if(!object->exp)
			object->exp = CGHeroInstance::UNINITIALIZED_EXPERIENCE;
	}

	bool hasPortrait = reader->readBool();
	if(hasPortrait)
		object->portrait = reader->readHeroPortrait();

	bool hasSecSkills = reader->readBool();
	if(hasSecSkills)
	{
		if(!object->secSkills.empty())
		{
			if(object->secSkills[0].first != SecondarySkill::DEFAULT)
				logGlobal->warn("Hero %s subID=%d has set secondary skills twice (in map properties and on adventure map instance). Using the latter set...", object->getNameTextID(), object->subID);
			object->secSkills.clear();
		}

		uint32_t skillsCount = reader->readUInt32();
		object->secSkills.resize(skillsCount);
		for(int i = 0; i < skillsCount; ++i)
		{
			object->secSkills[i].first = reader->readSkill();
			object->secSkills[i].second = reader->readUInt8();
		}
	}

	bool hasGarison = reader->readBool();
	if(hasGarison)
		readCreatureSet(object, 7);

	object->formation = static_cast<EArmyFormation>(reader->readUInt8());
	assert(object->formation == EArmyFormation::LOOSE || object->formation == EArmyFormation::TIGHT);

	loadArtifactsOfHero(object);
	object->patrol.patrolRadius = reader->readUInt8();
	object->patrol.patrolling = (object->patrol.patrolRadius != 0xff);

	if(features.levelAB)
	{
		bool hasCustomBiography = reader->readBool();
		if(hasCustomBiography)
			object->biographyCustom = readLocalizedString(TextIdentifier("heroes", object->subID, "biography"));

		object->gender = static_cast<EHeroGender>(reader->readUInt8());
		assert(object->gender == EHeroGender::MALE || object->gender == EHeroGender::FEMALE || object->gender == EHeroGender::DEFAULT);
	}
	else
	{
		object->gender = EHeroGender::DEFAULT;
	}

	// Spells
	if(features.levelSOD)
	{
		bool hasCustomSpells = reader->readBool();
		if(hasCustomSpells)
		{
			if(!object->spells.empty())
			{
				object->clear();
				logGlobal->warn("Hero %s subID=%d has spells set twice (in map properties and on adventure map instance). Using the latter set...", object->getNameTextID(), object->subID);
			}

			object->spells.insert(SpellID::PRESET); //placeholder "preset spells"

			reader->readBitmaskSpells(object->spells, false);
		}
	}
	else if(features.levelAB)
	{
		//we can read one spell
		SpellID spell = reader->readSpell();

		if(spell != SpellID::NONE)
			object->spells.insert(spell);
	}

	if(features.levelSOD)
	{
		bool hasCustomPrimSkills = reader->readBool();
		if(hasCustomPrimSkills)
		{
			auto ps = object->getAllBonuses(Selector::type()(BonusType::PRIMARY_SKILL).And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL)), nullptr);
			if(ps->size())
			{
				logGlobal->warn("Hero %s subID=%d has set primary skills twice (in map properties and on adventure map instance). Using the latter set...", object->getNameTranslated(), object->subID );
				for(const auto & b : *ps)
					object->removeBonus(b);
			}

			for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; ++xx)
			{
				object->pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(xx), reader->readUInt8());
			}
		}
	}

	if (object->subID != -1)
		logGlobal->debug("Map '%s': Hero on map: %s at %s, owned by %s", mapName, VLC->heroh->getByIndex(object->subID)->getJsonKey(), mapPosition.toString(), object->getOwner().getStr());
	else
		logGlobal->debug("Map '%s': Hero on map: (random) at %s, owned by %s", mapName, mapPosition.toString(), object->getOwner().getStr());

	reader->skipZero(16);
	return object;
}

CGObjectInstance * CMapLoaderH3M::readSeerHut(const int3 & position)
{
	auto * hut = new CGSeerHut();

	uint32_t questsCount = 1;

	if(features.levelHOTA3)
		questsCount = reader->readUInt32();

	//TODO: HotA
	if(questsCount > 1)
		logGlobal->warn("Map '%s': Seer Hut at %s - %d quests are not implemented!", mapName, position.toString(), questsCount);

	for(size_t i = 0; i < questsCount; ++i)
		readSeerHutQuest(hut, position);

	if(features.levelHOTA3)
	{
		uint32_t repeateableQuestsCount = reader->readUInt32();

		if(repeateableQuestsCount != 0)
			logGlobal->warn("Map '%s': Seer Hut at %s - %d repeatable quests are not implemented!", mapName, position.toString(), repeateableQuestsCount);

		for(size_t i = 0; i < repeateableQuestsCount; ++i)
			readSeerHutQuest(hut, position);
	}

	reader->skipZero(2);

	return hut;
}

void CMapLoaderH3M::readSeerHutQuest(CGSeerHut * hut, const int3 & position)
{
	if(features.levelAB)
	{
		readQuest(hut, position);
	}
	else
	{
		//RoE
		auto artID = reader->readArtifact();
		if(artID != ArtifactID::NONE)
		{
			//not none quest
			hut->quest->addArtifactID(artID);
			hut->quest->missionType = CQuest::MISSION_ART;
		}
		else
		{
			hut->quest->missionType = CQuest::MISSION_NONE;
		}
		hut->quest->lastDay = -1; //no timeout
		hut->quest->isCustomFirst = false;
		hut->quest->isCustomNext = false;
		hut->quest->isCustomComplete = false;
	}

	if(hut->quest->missionType)
	{
		auto rewardType = static_cast<CGSeerHut::ERewardType>(reader->readUInt8());
		hut->rewardType = rewardType;
		switch(rewardType)
		{
			case CGSeerHut::EXPERIENCE:
			{
				hut->rVal = reader->readUInt32();
				break;
			}
			case CGSeerHut::MANA_POINTS:
			{
				hut->rVal = reader->readUInt32();
				break;
			}
			case CGSeerHut::MORALE_BONUS:
			{
				hut->rVal = reader->readUInt8();
				break;
			}
			case CGSeerHut::LUCK_BONUS:
			{
				hut->rVal = reader->readUInt8();
				break;
			}
			case CGSeerHut::RESOURCES:
			{
				hut->rID = reader->readUInt8();
				hut->rVal = reader->readUInt32();

				assert(hut->rID < features.resourcesCount);
				assert((hut->rVal & 0x00ffffff) == hut->rVal);
				break;
			}
			case CGSeerHut::PRIMARY_SKILL:
			{
				hut->rID = reader->readUInt8();
				hut->rVal = reader->readUInt8();
				break;
			}
			case CGSeerHut::SECONDARY_SKILL:
			{
				hut->rID = reader->readSkill();
				hut->rVal = reader->readUInt8();
				break;
			}
			case CGSeerHut::ARTIFACT:
			{
				hut->rID = reader->readArtifact();
				break;
			}
			case CGSeerHut::SPELL:
			{
				hut->rID = reader->readSpell();
				break;
			}
			case CGSeerHut::CREATURE:
			{
				hut->rID = reader->readCreature();
				hut->rVal = reader->readUInt16();
				break;
			}
			case CGSeerHut::NOTHING:
			{
				// no-op
				break;
			}
			default:
			{
				assert(0);
			}
		}
	}
	else
	{
		// missionType==255
		reader->skipZero(1);
	}
}

void CMapLoaderH3M::readQuest(IQuestObject * guard, const int3 & position)
{
	guard->quest->missionType = static_cast<CQuest::Emission>(reader->readUInt8());

	switch(guard->quest->missionType)
	{
		case CQuest::MISSION_NONE:
			return;
		case CQuest::MISSION_PRIMARY_STAT:
		{
			guard->quest->m2stats.resize(4);
			for(int x = 0; x < 4; ++x)
			{
				guard->quest->m2stats[x] = reader->readUInt8();
			}
		}
		break;
		case CQuest::MISSION_LEVEL:
		case CQuest::MISSION_KILL_HERO:
		case CQuest::MISSION_KILL_CREATURE:
		{
			guard->quest->m13489val = reader->readUInt32();
			break;
		}
		case CQuest::MISSION_ART:
		{
			int artNumber = reader->readUInt8();
			for(int yy = 0; yy < artNumber; ++yy)
			{
				auto artid = reader->readArtifact();
				guard->quest->addArtifactID(artid);
				map->allowedArtifact[artid] = false; //these are unavailable for random generation
			}
			break;
		}
		case CQuest::MISSION_ARMY:
		{
			int typeNumber = reader->readUInt8();
			guard->quest->m6creatures.resize(typeNumber);
			for(int hh = 0; hh < typeNumber; ++hh)
			{
				guard->quest->m6creatures[hh].type = VLC->creh->objects[reader->readCreature()];
				guard->quest->m6creatures[hh].count = reader->readUInt16();
			}
			break;
		}
		case CQuest::MISSION_RESOURCES:
		{
			for(int x = 0; x < 7; ++x)
				guard->quest->m7resources[x] = reader->readUInt32();

			break;
		}
		case CQuest::MISSION_HERO:
		{
			guard->quest->m13489val = reader->readHero().getNum();
			break;
		}
		case CQuest::MISSION_PLAYER:
		{
			guard->quest->m13489val = reader->readPlayer().getNum();
			break;
		}
		case CQuest::MISSION_HOTA_MULTI:
		{
			uint32_t missionSubID = reader->readUInt32();

			if(missionSubID == 0)
			{
				guard->quest->missionType = CQuest::MISSION_NONE; //TODO: CQuest::MISSION_HOTA_HERO_CLASS;
				std::set<HeroClassID> heroClasses;
				reader->readBitmaskHeroClassesSized(heroClasses, false);

				logGlobal->warn("Map '%s': Quest at %s 'Belong to one of %d classes' is not implemented!", mapName, position.toString(), heroClasses.size());
				break;
			}
			if(missionSubID == 1)
			{
				guard->quest->missionType = CQuest::MISSION_NONE; //TODO: CQuest::MISSION_HOTA_REACH_DATE;
				uint32_t daysPassed = reader->readUInt32();

				logGlobal->warn("Map '%s': Quest at %s 'Wait till %d days passed' is not implemented!", mapName, position.toString(), daysPassed);
				break;
			}
			assert(0);
			break;
		}
		default:
		{
			assert(0);
		}
	}

	guard->quest->lastDay = reader->readInt32();
	guard->quest->firstVisitText = readLocalizedString(TextIdentifier("quest", position.x, position.y, position.z, "firstVisit"));
	guard->quest->nextVisitText = readLocalizedString(TextIdentifier("quest", position.x, position.y, position.z, "nextVisit"));
	guard->quest->completedText = readLocalizedString(TextIdentifier("quest", position.x, position.y, position.z, "completed"));
	guard->quest->isCustomFirst = !guard->quest->firstVisitText.empty();
	guard->quest->isCustomNext = !guard->quest->nextVisitText.empty();
	guard->quest->isCustomComplete = !guard->quest->completedText.empty();
}

CGObjectInstance * CMapLoaderH3M::readTown(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto * object = new CGTownInstance();
	if(features.levelAB)
		object->identifier = reader->readUInt32();

	setOwnerAndValidate(position, object, reader->readPlayer());

	std::optional<FactionID> faction;
	if (objectTemplate->id == Obj::TOWN)
		faction = FactionID(objectTemplate->subid);

	bool hasName = reader->readBool();
	if(hasName)
		object->setNameTranslated(readLocalizedString(TextIdentifier("town", position.x, position.y, position.z, "name")));

	bool hasGarrison = reader->readBool();
	if(hasGarrison)
		readCreatureSet(object, 7);

	object->formation = static_cast<EArmyFormation>(reader->readUInt8());
	assert(object->formation == EArmyFormation::LOOSE || object->formation == EArmyFormation::TIGHT);

	bool hasCustomBuildings = reader->readBool();
	if(hasCustomBuildings)
	{
		reader->readBitmaskBuildings(object->builtBuildings, faction);
		reader->readBitmaskBuildings(object->forbiddenBuildings, faction);
	}
	// Standard buildings
	else
	{
		bool hasFort = reader->readBool();
		if(hasFort)
			object->builtBuildings.insert(BuildingID::FORT);

		//means that set of standard building should be included
		object->builtBuildings.insert(BuildingID::DEFAULT);
	}

	if(features.levelAB)
	{
		std::set<SpellID> spellsMask;

		reader->readBitmaskSpells(spellsMask, false);
		std::copy(spellsMask.begin(), spellsMask.end(), std::back_inserter(object->obligatorySpells));
	}

	{
		std::set<SpellID> spellsMask;

		reader->readBitmaskSpells(spellsMask, true);
		std::copy(spellsMask.begin(), spellsMask.end(), std::back_inserter(object->possibleSpells));

		auto defaultAllowed = VLC->spellh->getDefaultAllowed();

		//add all spells from mods
		for(int i = features.spellsCount; i < defaultAllowed.size(); ++i)
			if(defaultAllowed[i])
				object->possibleSpells.emplace_back(i);
	}

	if(features.levelHOTA1)
	{
		// TODO: HOTA support
		[[maybe_unused]] bool spellResearchAvailable = reader->readBool();
	}

	// Read castle events
	uint32_t eventsCount = reader->readUInt32();

	for(int eventID = 0; eventID < eventsCount; ++eventID)
	{
		CCastleEvent event;
		event.town = object;
		event.name = readBasicString();
		event.message = readLocalizedString(TextIdentifier("town", position.x, position.y, position.z, "event", eventID, "description"));

		reader->readResourses(event.resources);

		event.players = reader->readUInt8();
		if(features.levelSOD)
			event.humanAffected = reader->readBool();
		else
			event.humanAffected = true;

		event.computerAffected = reader->readUInt8();
		event.firstOccurence = reader->readUInt16();
		event.nextOccurence = reader->readUInt8();

		reader->skipZero(17);

		// New buildings
		reader->readBitmaskBuildings(event.buildings, faction);

		event.creatures.resize(7);
		for(int i = 0; i < 7; ++i)
			event.creatures[i] = reader->readUInt16();

		reader->skipZero(4);
		object->events.push_back(event);
	}

	if(features.levelSOD)
	{
		object->alignmentToPlayer = PlayerColor::NEUTRAL; // "same as owner or random"

		 uint8_t alignment = reader->readUInt8();

		if(alignment != PlayerColor::NEUTRAL.getNum())
		{
			if(alignment < PlayerColor::PLAYER_LIMIT.getNum())
			{
				if (mapHeader->players[alignment].canAnyonePlay())
					object->alignmentToPlayer = PlayerColor(alignment);
				else
					logGlobal->warn("%s - Aligment of town at %s is invalid! Player %d is not present on map!", mapName, position.toString(), int(alignment));
			}
			else
			{
				// TODO: HOTA support
				uint8_t invertedAlignment = alignment - PlayerColor::PLAYER_LIMIT.getNum();

				if(invertedAlignment < PlayerColor::PLAYER_LIMIT.getNum())
				{
					logGlobal->warn("%s - Aligment of town at %s 'not as player %d' is not implemented!", mapName, position.toString(), alignment - PlayerColor::PLAYER_LIMIT.getNum());
				}
				else
				{
					logGlobal->warn("%s - Aligment of town at %s is corrupted!!", mapName, position.toString());
				}
			}
		}
	}
	reader->skipZero(3);

	return object;
}

void CMapLoaderH3M::readEvents()
{
	uint32_t eventsCount = reader->readUInt32();
	for(int eventID = 0; eventID < eventsCount; ++eventID)
	{
		CMapEvent event;
		event.name = readBasicString();
		event.message = readLocalizedString(TextIdentifier("event", eventID, "description"));

		reader->readResourses(event.resources);
		event.players = reader->readUInt8();
		if(features.levelSOD)
		{
			event.humanAffected = reader->readBool();
		}
		else
		{
			event.humanAffected = true;
		}
		event.computerAffected = reader->readBool();
		event.firstOccurence = reader->readUInt16();
		event.nextOccurence = reader->readUInt8();

		reader->skipZero(17);

		map->events.push_back(event);
	}
}

void CMapLoaderH3M::readMessageAndGuards(std::string & message, CCreatureSet * guards, const int3 & position)
{
	bool hasMessage = reader->readBool();
	if(hasMessage)
	{
		message = readLocalizedString(TextIdentifier("guards", position.x, position.y, position.z, "message"));
		bool hasGuards = reader->readBool();
		if(hasGuards)
			readCreatureSet(guards, 7);

		reader->skipZero(4);
	}
}

std::string CMapLoaderH3M::readBasicString()
{
	return TextOperations::toUnicode(reader->readBaseString(), fileEncoding);
}

std::string CMapLoaderH3M::readLocalizedString(const TextIdentifier & stringIdentifier)
{
	std::string mapString = TextOperations::toUnicode(reader->readBaseString(), fileEncoding);
	TextIdentifier fullIdentifier("map", mapName, stringIdentifier.get());

	if(mapString.empty())
		return "";

	VLC->generaltexth->registerString(modName, fullIdentifier, mapString);
	return VLC->generaltexth->translate(fullIdentifier.get());
}

void CMapLoaderH3M::afterRead()
{
	//convert main town positions for all players to actual object position, in H3M it is position of active tile

	for(auto & p : map->players)
	{
		int3 posOfMainTown = p.posOfMainTown;
		if(posOfMainTown.valid() && map->isInTheMap(posOfMainTown))
		{
			const TerrainTile & t = map->getTile(posOfMainTown);

			const CGObjectInstance * mainTown = nullptr;

			for(auto * obj : t.visitableObjects)
			{
				if(obj->ID == Obj::TOWN || obj->ID == Obj::RANDOM_TOWN)
				{
					mainTown = obj;
					break;
				}
			}

			if(mainTown == nullptr)
				continue;

			p.posOfMainTown = posOfMainTown + mainTown->getVisitableOffset();
		}
	}
}

VCMI_LIB_NAMESPACE_END
