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

#include "CCastleEvent.h"
#include "CMap.h"
#include "MapReaderH3M.h"
#include "MapFormatSettings.h"

#include "../CCreatureHandler.h"
#include "../texts/CGeneralTextHandler.h"
#include "../CSkillHandler.h"
#include "../CStopWatch.h"
#include "../IGameSettings.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../TerrainHandler.h"
#include "../GameLibrary.h"
#include "../constants/StringConstants.h"
#include "../entities/artifact/CArtHandler.h"
#include "../entities/hero/CHeroHandler.h"
#include "../entities/ResourceTypeHandler.h"
#include "../filesystem/CBinaryReader.h"
#include "../filesystem/Filesystem.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjectConstructors/CommonConstructors.h"
#include "../mapObjects/CGCreature.h"
#include "../mapObjects/CGResource.h"
#include "../mapObjects/CQuest.h"
#include "../mapObjects/MapObjects.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../modding/ModScope.h"
#include "../networkPacks/Component.h"
#include "../networkPacks/ArtifactLocation.h"
#include "../spells/CSpellHandler.h"
#include "../texts/TextOperations.h"
#include "entities/hero/CHeroClass.h"

VCMI_LIB_NAMESPACE_BEGIN

CMapLoaderH3M::CMapLoaderH3M(const std::string & mapName, const std::string & modName, const std::string & encodingName, CInputStream * stream)
	: map(nullptr)
	, reader(new MapReaderH3M(stream))
	, inputStream(stream)
	, mapName(TextOperations::convertMapName(mapName))
	, modName(modName)
	, fileEncoding(encodingName)
{
}

//must be instantiated in .cpp file for access to complete types of all member fields
CMapLoaderH3M::~CMapLoaderH3M() = default;

std::unique_ptr<CMap> CMapLoaderH3M::loadMap(IGameInfoCallback * cb)
{
	// Init map object by parsing the input buffer
	map = new CMap(cb);
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
	inputStream->seek(0);

	readHeader();
	readMapOptions();
	readHotaScripts();
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
	//map->banWaterContent(); //Not sure if force this for custom scenarios
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

		if(features.levelHOTA8)
		{
			int hotaVersionMajor = reader->readUInt32();
			int hotaVersionMinor = reader->readUInt32();
			int hotaVersionPatch = reader->readUInt32();
			logGlobal->trace("Loading HotA map, version %d.%d.%d", hotaVersionMajor, hotaVersionMinor, hotaVersionPatch);
		}

		if(features.levelHOTA1)
		{
			bool isMirrorMap = reader->readBool();
			bool isArenaMap = reader->readBool();

			//TODO: HotA
			if (isMirrorMap)
				logGlobal->warn("Map '%s': Mirror maps are not yet supported!", mapName);

			if (isArenaMap)
				logGlobal->warn("Map '%s': Arena maps are not supported!", mapName);
		}

		if(features.levelHOTA2)
		{
			int32_t terrainTypesCount = reader->readUInt32();
			assert(features.terrainsCount == terrainTypesCount);

			if (features.terrainsCount != terrainTypesCount)
				logGlobal->warn("Map '%s': Expected %d terrains, but %d found!", mapName, features.terrainsCount, terrainTypesCount);
		}

		if(features.levelHOTA5)
		{
			int32_t townTypesCount = reader->readUInt32();
			int8_t allowedDifficultiesMask = reader->readInt8Checked(0, 31);

			// wrong assumption and this is not related to factions?
			// assert(features.factionsCount == townTypesCount);

			if (features.factionsCount != townTypesCount)
				logGlobal->warn("Map '%s': Expected %d factions, but %d found!", mapName, features.factionsCount, townTypesCount);

			if (allowedDifficultiesMask != 0 && allowedDifficultiesMask != 31) //TODO: recheck if 0 is possible
				logGlobal->warn("Map '%s': List of allowed difficulties (%d) is not implemented!", mapName, static_cast<int>(allowedDifficultiesMask));
		}

		if(features.levelHOTA7)
		{
			bool canHireDefeatedHeroes = reader->readBool();

			if (!canHireDefeatedHeroes)
				logGlobal->warn("Map '%s': Option to block hiring of defeated heroes is not implemented!", mapName);
		}

		if(features.levelHOTA8)
		{
			bool forceMatchingVersion = reader->readBool();
			if (forceMatchingVersion)
				logGlobal->warn("Map '%s': This map is forced to use specific hota version!", mapName);
		}

		if(features.levelHOTA9)
		{
			int unknown = reader->readInt32();
			if(unknown != 0)
				logGlobal->warn("Map '%s': Unknown value in header was set to %d!", mapName, unknown);
		}
	}
	else
	{
		features = MapFormatFeaturesH3M::find(mapHeader->version, 0);
		reader->setFormatLevel(features);
	}

	if (!LIBRARY->mapFormat->isSupported(mapHeader->version))
		throw std::runtime_error("Unsupported map format! Format ID " + std::to_string(static_cast<int>(mapHeader->version)));

	const MapIdentifiersH3M & identifierMapper = LIBRARY->mapFormat->getMapping(mapHeader->version);

	reader->setIdentifierRemapper(identifierMapper);

	// Read map name, description, dimensions,...
	mapHeader->areAnyPlayers = reader->readBool();
	mapHeader->height = mapHeader->width = reader->readInt32();
	mapHeader->mapLayers = reader->readBool() ? std::vector<MapLayerId>({MapLayerId::SURFACE, MapLayerId::UNDERGROUND}) : std::vector<MapLayerId>({MapLayerId::SURFACE});
	mapHeader->name.appendTextID(readLocalizedString("header.name"));
	mapHeader->description.appendTextID(readLocalizedString("header.description"));
	mapHeader->author.appendRawString("");
	mapHeader->authorContact.appendRawString("");
	mapHeader->mapVersion.appendRawString("");
	mapHeader->creationDateTime = 0;
	mapHeader->difficulty = static_cast<EMapDifficulty>(reader->readInt8Checked(0, 4));

	if(features.levelAB)
		mapHeader->levelLimit = reader->readInt8Checked(0, std::min(100u, LIBRARY->heroh->maxSupportedLevel()));
	else
		mapHeader->levelLimit = 0;

	readPlayerInfo();
	readVictoryLossConditions();
	readTeamInfo();
	readAllowedHeroes();
	readDisposedHeroes();
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

		playerInfo.aiTactic = static_cast<EAiTactic>(reader->readInt8Checked(-1, 3));

		if(features.levelSOD)
			reader->skipUnused(1); //faction is selectable

		std::set<FactionID> allowedFactions;

		reader->readBitmaskFactions(allowedFactions, false);

		playerInfo.isFactionRandom = reader->readBool();
		const bool allFactionsAllowed = playerInfo.isFactionRandom && allowedFactions.size() == features.factionsCount;

		if(!allFactionsAllowed)
		{
			if (!allowedFactions.empty())
				playerInfo.allowedFactions = allowedFactions;
			else
				logGlobal->warn("Map '%s': Player %d has no allowed factions to play! Ignoring.", mapName, i);
		}

		playerInfo.hasMainTown = reader->readBool();
		if(playerInfo.hasMainTown)
		{
			if(features.levelAB)
			{
				playerInfo.generateHeroAtMainTown = reader->readBool();
				reader->skipUnused(1); // starting town type, unused
			}
			else
			{
				playerInfo.generateHeroAtMainTown = true;
			}

			playerInfo.posOfMainTown = reader->readInt3();
		}

		playerInfo.hasRandomHero = reader->readBool();
		playerInfo.mainCustomHeroId = reader->readHero();

		if(playerInfo.mainCustomHeroId != HeroTypeID::NONE)
		{
			playerInfo.mainCustomHeroPortrait = reader->readHeroPortrait();
			playerInfo.mainCustomHeroNameTextId = readLocalizedString(TextIdentifier("header", "player", i, "mainHeroName"));
		}

		if(features.levelAB)
		{
			reader->skipUnused(1); //TODO: check meaning?
			size_t heroCount = reader->readUInt32();
			for(size_t pp = 0; pp < heroCount; ++pp)
			{
				SHeroName vv;
				vv.heroId = reader->readHero();
				vv.heroName = readLocalizedString(TextIdentifier("header", "heroNames", vv.heroId.getNum()));

				playerInfo.heroesNames.push_back(vv);
			}
		}
	}
}

void CMapLoaderH3M::readVictoryLossConditions()
{
	mapHeader->triggeredEvents.clear();
	mapHeader->victoryMessage.clear();
	mapHeader->defeatMessage.clear();

	auto vicCondition = static_cast<EVictoryConditionType>(reader->readInt8Checked(-1, 12));

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

		switch(vicCondition)
		{
			case EVictoryConditionType::ARTIFACT:
			{
				assert(allowNormalVictory == true); // not selectable in editor
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
				specialVictory.onFulfill.appendTextID("core.genrltxt.276");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.2");
				break;
			}
			case EVictoryConditionType::GATHERRESOURCE:
			{
				EventCondition cond(EventCondition::HAVE_RESOURCES);
				cond.objectType = reader->readGameResID();
				cond.value = reader->readInt32();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.279");
				specialVictory.onFulfill.appendTextID("core.genrltxt.278");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.3");
				break;
			}
			case EVictoryConditionType::BUILDCITY:
			{
				assert(appliesToAI == true); // not selectable in editor
				EventExpression::OperatorAll oper;
				EventCondition cond(EventCondition::HAVE_BUILDING);
				cond.position = reader->readInt3();
				cond.objectType = BuildingID::HALL_LEVEL(reader->readInt8Checked(0,3) + 1);
				oper.expressions.emplace_back(cond);
				cond.objectType = BuildingID::FORT_LEVEL(reader->readInt8Checked(0, 2));
				oper.expressions.emplace_back(cond);

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.283");
				specialVictory.onFulfill.appendTextID("core.genrltxt.282");
				specialVictory.trigger = EventExpression(oper);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.4");
				break;
			}
			case EVictoryConditionType::BUILDGRAIL:
			{
				assert(allowNormalVictory == true); // not selectable in editor
				assert(appliesToAI == true); // not selectable in editor
				EventCondition cond(EventCondition::HAVE_BUILDING);
				cond.objectType = BuildingID(BuildingID::GRAIL);
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
				if (!allowNormalVictory)
					logGlobal->debug("Map %s: Has 'beat hero' as victory condition, but 'allow normal victory' not set. Ignoring", mapName);
				allowNormalVictory = true; // H3 behavior
				assert(appliesToAI == false); // not selectable in editor
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = MapObjectID(MapObjectID::HERO);
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
				cond.objectType = MapObjectID(MapObjectID::TOWN);
				cond.position = reader->readInt3();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.250");
				specialVictory.onFulfill.appendTextID("core.genrltxt.249");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.7");
				break;
			}
			case EVictoryConditionType::BEATMONSTER:
			{
				assert(appliesToAI == true); // not selectable in editor
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = MapObjectID(MapObjectID::MONSTER);
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
				oper.expressions.emplace_back(EventCondition(EventCondition::CONTROL, 0, Obj(Obj::CREATURE_GENERATOR1)));
				oper.expressions.emplace_back(EventCondition(EventCondition::CONTROL, 0, Obj(Obj::CREATURE_GENERATOR4)));

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.289");
				specialVictory.onFulfill.appendTextID("core.genrltxt.288");
				specialVictory.trigger = EventExpression(oper);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.9");
				break;
			}
			case EVictoryConditionType::TAKEMINES:
			{
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = MapObjectID(MapObjectID::MINE);

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.291");
				specialVictory.onFulfill.appendTextID("core.genrltxt.290");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.10");
				break;
			}
			case EVictoryConditionType::TRANSPORTITEM:
			{
				assert(allowNormalVictory == true); // not selectable in editor
				EventCondition cond(EventCondition::TRANSPORT);
				cond.objectType = reader->readArtifact8();
				cond.position = reader->readInt3();

				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.293");
				specialVictory.onFulfill.appendTextID("core.genrltxt.292");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.11");
				break;
			}
			case EVictoryConditionType::HOTA_ELIMINATE_ALL_MONSTERS:
			{
				assert(appliesToAI == false); // not selectable in editor
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = MapObjectID(MapObjectID::MONSTER);

				specialVictory.effect.toOtherMessage.appendTextID("vcmi.map.victoryCondition.eliminateMonsters.toOthers");
				specialVictory.onFulfill.appendTextID("vcmi.map.victoryCondition.eliminateMonsters.toSelf");
				specialVictory.trigger = EventExpression(cond);

				mapHeader->victoryMessage.appendTextID("core.vcdesc.12");
				mapHeader->victoryIconIndex = 12;
				break;
			}
			case EVictoryConditionType::HOTA_SURVIVE_FOR_DAYS:
			{
				assert(appliesToAI == false); // not selectable in editor
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
	auto lossCond = static_cast<ELossConditionType>(reader->readInt8Checked(-1, 2));
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
				cond.objectType = Obj(Obj::TOWN);
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
				cond.objectType = Obj(Obj::HERO);
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
	mapHeader->allowedHeroes = LIBRARY->heroh->getDefaultAllowed();

	if(features.levelHOTA0)
		reader->readBitmaskHeroesSized(mapHeader->allowedHeroes, false);
	else
		reader->readBitmaskHeroes(mapHeader->allowedHeroes, false);

	if(features.levelAB)
	{
		size_t placeholdersQty = reader->readUInt32();

		for (size_t i = 0; i < placeholdersQty; ++i)
		{
			auto heroID = reader->readHero();
			mapHeader->reservedCampaignHeroes.insert(heroID);
		}
	}
}

void CMapLoaderH3M::readDisposedHeroes()
{
	// Reading disposed heroes (20 bytes)
	if(features.levelSOD)
	{
		size_t disp = reader->readUInt8();
		mapHeader->disposedHeroes.resize(disp);
		for(size_t g = 0; g < disp; ++g)
		{
			mapHeader->disposedHeroes[g].heroId = reader->readHero();
			mapHeader->disposedHeroes[g].portrait = reader->readHeroPortrait();
			mapHeader->disposedHeroes[g].name = readLocalizedString(TextIdentifier("header", "heroes", mapHeader->disposedHeroes[g].heroId.getNum()));
			reader->readBitmaskPlayers(mapHeader->disposedHeroes[g].players, false);
		}
	}
}

void CMapLoaderH3M::readMapOptions()
{
	//omitting NULLS
	reader->skipZero(31);

	if(features.levelHOTA0)
	{
		bool allowSpecialMonths = reader->readBool();
		map->overrideGameSetting(EGameSettings::CREATURES_ALLOW_RANDOM_SPECIAL_WEEKS, JsonNode(allowSpecialMonths));
		reader->skipZero(3);
	}

	if(features.levelHOTA1)
	{
		int32_t combinedArtifactsCount = reader->readInt32();
		int32_t combinedArtifactsBytes = (combinedArtifactsCount + 7) / 8;

		for (int i = 0; i < combinedArtifactsBytes; ++i)
		{
			uint8_t mask = reader->readUInt8();
			if (mask != 0)
				logGlobal->warn("Map '%s': Option to ban specific combined artifacts is not implemented!", mapName);
		}
	}

	if(features.levelHOTA3)
	{
		//TODO: HotA
		int32_t roundLimit = reader->readInt32();
		if(roundLimit != -1)
			logGlobal->warn("Map '%s': roundLimit of %d is not implemented!", mapName, roundLimit);
	}

	if(features.levelHOTA5)
	{
		for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		{
			// unconfirmed, but only remainig option according to changelog
			bool heroRecruitmentBlocked = reader->readBool();
			if (heroRecruitmentBlocked)
				logGlobal->warn("Map '%s': option to ban hero recruitment for %s is not implemented!!", mapName, PlayerColor(i).toString());
		}
	}

	const MapIdentifiersH3M & identifierMapper = LIBRARY->mapFormat->getMapping(mapHeader->version);
	map->overrideGameSettings(identifierMapper.getFormatSettings());
}

void CMapLoaderH3M::readHotaScripts()
{
	if(!features.levelHOTA9)
		return;

	bool eventsSystemActive = reader->readBool();
	if(!eventsSystemActive)
		return;

	const auto & loadEventList = [this](const std::string & eventType) -> void
	{
		int eventsCount = reader->readInt32();
		for(int i = 0; i < eventsCount; ++i)
		{
			int eventID = reader->readInt32();
			readHotaScriptActions();
			std::string eventName = reader->readBaseString();
			logGlobal->warn("Map %s: Event %s (%d), type %d is not implemented!", mapName, eventName, eventID, eventType);
		}
	};

	const auto & loadEventMap = [this]() -> void
	{
		int mappingSize = reader->readInt32();
		for(int i = 0; i < mappingSize; ++i)
			reader->readInt32(); // UID of event
	};

	loadEventList("hero event");
	loadEventList("player event");
	loadEventList("town event");
	loadEventList("quest event");

	int nextVariableID = reader->readInt32();
	int nextHeroEventID = reader->readInt32();
	int nextPlayerEventID = reader->readInt32();
	int nextTownEventID = reader->readInt32();
	int nextQuestEventID = reader->readInt32();
	logGlobal->trace("Map %s: Next event ID's: %d, %d, %d, %d, %d", mapName, nextVariableID, nextHeroEventID, nextPlayerEventID, nextTownEventID, nextQuestEventID);

	int variablesCount = reader->readInt32();
	for(int i = 0; i < variablesCount; ++i)
	{
		int32_t uniqueID = reader->readInt32(); // 1... - unique index?
		std::string variableID = reader->readBaseString();
		bool unkPropA = reader->readBool(); // save in campaign?
		bool unkPropB = reader->readBool(); // import from prev map?
		int32_t initialValue = reader->readInt32();
		logGlobal->warn("Map %s: Variable %s (%d), initial value %d, flags %d/%d is not implemented", mapName, variableID, uniqueID, initialValue, unkPropA, unkPropB);
	}

	loadEventMap(); // hero event
	loadEventMap(); // player event
	loadEventMap(); // town event
	loadEventMap(); // quest event
	loadEventMap(); // variable
}

void CMapLoaderH3M::readHotaScriptActions()
{
	enum class HotaScriptActions : int32_t
	{
		// NOOP = 0? Unused?
		CONDITIONAL_CHAIN = 1,
		SET_VARIABLE_CONDITIONAL = 2,
		MODIFY_VARIABLE = 3,
		RESOURCES = 4,
		REMOVE_CURRENT_OBJECT_OR_FINISH_QUEST = 5, // shared ID ???
		SHOW_REWARDS_MESSAGE = 6,
		QUEST_ACTION = 7,
		CREATURES = 8,
		ARTIFACT = 9,
		CONSTRUCT_BUILDING = 10,
		SET_QUEST_HINT = 11,
		SHOW_QUESTION = 12,
		CONDITIONAL = 13,
		CREATURES_TO_HIRE = 14,
		SPELL = 15,
		EXPERIENCE = 16,
		SPELL_POINTS = 17,
		MOVEMENT_POINTS = 18,
		PRIMARY_SKILL = 19,
		SECONDARY_SKILL = 20,
		LUCK = 21,
		MORALE = 22,
		START_COMBAT = 23,
		EXECUTE_EVENT = 24,
		WAR_MACHINE = 25,
		SPELLBOOK = 26,
		DISABLE_EVENT = 27,
		LOOP_FOR = 28,
		SHOW_MESSAGE = 29
	};

	int unk2 = reader->readInt32(); // event type?
	int unk3 = reader->readInt8();
	assert(unk2 == 1);
	assert(unk3 == 0);
	logGlobal->warn("Map %s: HotA Script action - unkown values %d/%d", mapName, unk2, unk3);

	int actionsCount = reader->readInt32();
	for(int j = 0; j < actionsCount; ++j)
	{
		HotaScriptActions actionType = static_cast<HotaScriptActions>(reader->readInt32());

		switch(actionType)
		{
			case HotaScriptActions::SHOW_MESSAGE:
			{
				std::string textID = readBasicString();
				int32_t numberOfImages = reader->readInt32();
				for (int i = 0; i < numberOfImages;++i)
				{
					int32_t imageType = reader->readInt32(); // e.g. Creatures
					int32_t imageSubtype = reader->readInt32(); // e.g. Archers
					readHotaScriptExpression(); // e.g. 10 Archers
					logGlobal->warn("Map %s: HotA Script action - SHOW_MESSAGE: type %d/%d", mapName, imageType, imageSubtype);
				}
				logGlobal->warn("Map %s: HotA Script action - SHOW_MESSAGE: message %s, %d images", mapName, textID, numberOfImages);
				break;
			}
			case HotaScriptActions::SHOW_REWARDS_MESSAGE:
			{
				std::string textID = readBasicString();
				readHotaScriptActions();
				logGlobal->warn("Map %s: HotA Script action - SHOW_REWARDS_MESSAGE: message %s", mapName, textID);
				break;
			}
			case HotaScriptActions::REMOVE_CURRENT_OBJECT_OR_FINISH_QUEST:
			{
				logGlobal->warn("Map %s: HotA Script action - REMOVE_CURRENT_OBJECT_OR_FINISH_QUEST", mapName);
				break; // no-op
			}
			case HotaScriptActions::DISABLE_EVENT:
			{
				logGlobal->warn("Map %s: HotA Script action - DISABLE_EVENT", mapName);
				break; // no-op
			}
			case HotaScriptActions::QUEST_ACTION:
			{
				readHotaScriptCondition();
				std::string proposalTextID = readBasicString();
				std::string progressionTextID = readBasicString();
				std::string completionTextID = readBasicString();
				std::string hintTextID = readBasicString();
				readHotaScriptActions();
				bool unk5 = reader->readBool(); // ???
				assert(unk5 == 1);
				logGlobal->warn("Map %s: HotA Script action - QUEST_ACTION: '%s' / '%s' / '%s' / '%s', unknown: %d", mapName, proposalTextID, progressionTextID, completionTextID, hintTextID, unk5);
				break;
			}
			case HotaScriptActions::CONDITIONAL:
			{
				readHotaScriptCondition();
				readHotaScriptActions();
				readHotaScriptActions();
				logGlobal->warn("Map %s: HotA Script action - CONDITIONAL", mapName);
				break;
			}

			case HotaScriptActions::LOOP_FOR:
			{
				readHotaScriptActions(); // loop body
				readHotaScriptExpression(); // initial value
				readHotaScriptExpression(); // final value
				int variableID = reader->readInt32();
				logGlobal->warn("Map %s: HotA Script action - LOOP_FOR: variable ID %d", mapName, variableID);
				break;
			}

			case HotaScriptActions::SET_QUEST_HINT:
			{
				std::string messageTextID = readBasicString();
				int32_t numberOfImages = reader->readInt32();
				for (int i = 0; i < numberOfImages;++i)
				{
					int32_t imageType = reader->readInt32(); // e.g. Creatures
					int32_t imageSubtype = reader->readInt32(); // e.g. Archers
					readHotaScriptExpression(); // e.g. 10 Archers
					logGlobal->warn("Map %s: HotA Script action - SET_QUEST_HINT: type %d/%d", mapName, imageType, imageSubtype);
				}
				bool showInLog = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - SET_QUEST_HINT: '%s', %d images, show in log: %d", mapName, messageTextID, numberOfImages, showInLog);
				break;
			}

			case HotaScriptActions::SHOW_QUESTION:
			{
				// 0 = no images
				// 1 = no exit
				// 2 = can exit?
				// 3 = specify images
				int imageShowType = reader->readInt8();
				std::string messageTextID = readBasicString();
				readHotaScriptActions();
				readHotaScriptActions();

				if (imageShowType == 2)
					readHotaScriptActions();

				int numberOfImages = 2;
				if (imageShowType == 0 || imageShowType == 3)
					numberOfImages = reader->readInt32();

				for (int i = 0; i < numberOfImages; ++i)
				{
					int32_t imageType = reader->readInt32(); // e.g. Creatures
					int32_t imageSubtype = reader->readInt32(); // e.g. Archers
					readHotaScriptExpression(); // e.g. 10 Archers
					logGlobal->warn("Map %s: HotA Script action - SHOW_QUESTION: type %d/%d", mapName, imageType, imageSubtype);
				}

				if (imageShowType == 1 || imageShowType == 2)
				{
					bool showOrBetweenImages = reader->readBool();
					int32_t unknown = reader->readInt32();
					logGlobal->warn("Map %s: HotA Script action - SHOW_QUESTION: show OR: %d, unknown: %d", mapName, showOrBetweenImages, unknown);
				}
				logGlobal->warn("Map %s: HotA Script action - SHOW_QUESTION: '%s', mode: %d, images: %d", mapName, messageTextID, imageShowType, numberOfImages);
				break;
			}
			case HotaScriptActions::ARTIFACT:
			{
				bool takeArtifact = reader->readBool();
				ArtifactID artifact = reader->readArtifact32();
				SpellID scrollSpellID = reader->readSpell32();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - ARTIFACT: art %d, spell %d, take: %d, show message: %d", mapName, artifact.getNum(), scrollSpellID.getNum(), takeArtifact, showMessage);
				break;
			}
			case HotaScriptActions::WAR_MACHINE:
			{
				bool takeMachine = reader->readBool();
				ArtifactID machine = reader->readArtifact32();
				reader->skipUnused(4); // garbage?
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - WAR_MACHINE: art %d, take: %d, show message: %d", mapName, machine.getNum(), takeMachine, showMessage);
				break;
			}
			case HotaScriptActions::SPELL:
			{
				SpellID spellID = reader->readSpell32();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - SPELL: spell %d, show message: %d", mapName, spellID.getNum(), showMessage);
				break;
			}
			case HotaScriptActions::SPELLBOOK:
			{
				bool takeSpellbook = reader->readBool();
				reader->skipUnused(8); // garbage?
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - SPELLBOOK: take: %d, show message: %d", mapName, takeSpellbook, showMessage);
				break;
			}
			case HotaScriptActions::CREATURES:
			{
				bool takeCreatures = reader->readBool();
				CreatureID creature = reader->readCreature32();
				readHotaScriptExpression();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - CREATURES: creature %d, take creatures: %d, show message: %d", mapName, creature.getNum(), takeCreatures, showMessage);
				break;
			}
			case HotaScriptActions::START_COMBAT:
			{
				for(int i = 0; i < 7; ++i)
				{
					readHotaScriptExpression();
					CreatureID creature = reader->readCreature32();
					logGlobal->warn("Map %s: HotA Script action - START_COMBAT, unit %d", mapName, creature.getNum());
				}
				logGlobal->warn("Map %s: HotA Script action - START_COMBAT done", mapName);
				break;
			}
			case HotaScriptActions::SECONDARY_SKILL:
			{
				int masteryLevel = reader->readInt32(); // 1..3
				SecondarySkill skill = reader->readSkill32();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - SECONDARY_SKILL %d mastery %d, show message %d", mapName, skill.getNum(), masteryLevel, showMessage);
				break;
			}
			case HotaScriptActions::MORALE:
			{
				int amount = reader->readInt32(); // -3..3
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - MORALE amount: %d, show message: %d", mapName, amount, showMessage);
				break;
			}
			case HotaScriptActions::LUCK:
			{
				int amount = reader->readInt32(); // -3..3
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - LUCK amount: %d, show message: %d", mapName, amount, showMessage);
				break;
			}
			case HotaScriptActions::EXPERIENCE:
			{
				readHotaScriptExpression();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - EXPERIENCE show message: %d", mapName, showMessage);
				break;
			}
			case HotaScriptActions::SPELL_POINTS:
			{
				readHotaScriptExpression();
				int mode = reader->readInt32();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - SPELL_POINTS mode: %d, show message: %d", mapName, mode, showMessage);
				break;
			}
			case HotaScriptActions::CREATURES_TO_HIRE:
			{
				int dwelling = reader->readInt32(); // 0-based
				readHotaScriptExpression(); // amount
				int unknown = reader->readInt32(); // factory 8th dwelling?
				bool showMessage = reader->readBool();
				assert(unknown == -1);
				logGlobal->warn("Map %s: HotA Script action - CREATURES_TO_HIRE dwelling: %d, unknown: %d, show message: %d", mapName, dwelling, unknown, showMessage);
				break;
			}
			case HotaScriptActions::CONSTRUCT_BUILDING:
			{
				BuildingID buildingID = reader->readBuilding32(std::nullopt);
				int unknownA = reader->readInt16(); // faction ID?
				int unknownB = reader->readInt16(); // faction building ID?
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - CONSTRUCT_BUILDING, building %d, unknown %d / %d, show message %d", mapName, buildingID.getNum(), unknownA, unknownB, showMessage);
				break;
			}
			case HotaScriptActions::EXECUTE_EVENT:
			{
				int eventType = reader->readInt32();
				int eventID = reader->readInt32();
				logGlobal->warn("Map %s: HotA Script action - EXECUTE_EVENT event type %d, event ID %d", mapName, eventType, eventID);
				break;
			}
			case HotaScriptActions::MOVEMENT_POINTS:
			{
				readHotaScriptExpression();
				int mode = reader->readInt32();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - MOVEMENT_POINTS mode %d, show message %d", mapName, mode, showMessage);
				break;
			}
			case HotaScriptActions::RESOURCES:
			{
				int mode = reader->readInt8();
				for(int i = 0; i < 7; ++i)
					readHotaScriptExpression();
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - RESOURCES mode %d, show message %d", mapName, mode, showMessage);
				break;
			}
			case HotaScriptActions::PRIMARY_SKILL:
			{
				readHotaScriptExpression();
				PrimarySkill skillToGive(reader->readInt32());
				bool showMessage = reader->readBool();
				logGlobal->warn("Map %s: HotA Script action - PRIMARY_SKILL skill %d, show message %d", mapName, skillToGive.getNum(), showMessage);
				break;
			}
			case HotaScriptActions::MODIFY_VARIABLE:
			{
				int variableID = reader->readInt32();
				int mode = reader->readInt8(); // 0 = add, 1 = substract, 2 = set
				readHotaScriptExpressionInternal(); // new value
				logGlobal->warn("Map %s: HotA Script action - MODIFY_VARIABLE variable ID %d, mode %d", mapName, variableID, mode);
				break;
			}
			case HotaScriptActions::SET_VARIABLE_CONDITIONAL:
			{
				int variableID = reader->readInt32();
				readHotaScriptCondition();
				readHotaScriptExpression();
				readHotaScriptExpression();
				logGlobal->warn("Map %s: HotA Script action - SET_VARIABLE_CONDITIONAL variable ID %d", mapName, variableID);
				break;
			}
			case HotaScriptActions::CONDITIONAL_CHAIN:
			{
				for(;;)
				{
					readHotaScriptCondition();
					readHotaScriptActions();

					int unknown = reader->readBool();
					int unknown2 = reader->readInt32();
					assert(unknown == 1);
					assert(unknown2 == 1 || unknown2 == 0);
					logGlobal->warn("Map %s: HotA Script action - CONDITIONAL_CHAIN block, unknown %d/%d", mapName, unknown, unknown2);
					if(unknown2 == 0)
						break;
				}
				int unknown3 = reader->readInt32();
				logGlobal->warn("Map %s: HotA Script action - CONDITIONAL_CHAIN end, unknown %d", mapName, unknown3);
				break;
			}
			default:
				throw std::runtime_error("Unknown event action code:" + std::to_string(static_cast<int>(actionType)));
		}
	}
}

void CMapLoaderH3M::readHotaScriptCondition()
{
	bool unknown = reader->readBool();
	assert(unknown == true);
	logGlobal->warn("Map %s: HotA Script condition - unknown value %d", mapName, unknown);
	readHotaScriptConditionInternal();
}

void CMapLoaderH3M::readHotaScriptConditionInternal()
{
	enum class HotaScriptCondition : int32_t
	{
		CONSTANT = 0,
		ALL_OF = 1, // and
		ANY_OF = 2, // or
		LESSER = 3,
		GREATER = 4,
		EQUAL = 5,
		NOT = 6,
		HAS_ARTIFACT = 7,
		GREATER_OR_EQUAL = 8,
		LESSER_OR_EQUAL = 9,
		NOT_EQUAL = 10,
		CURRENT_PLAYER = 11,
		HERO_OWNER = 12,
		// ??? = 13 unused or missing?
		PLAYER_DEFEATED_MONSTER = 14,
		PLAYER_DEFEATED_HERO = 15,
		HERO_SECONDARY_SKILL = 16,
		PLAYER_DEFEATED = 17,
		PLAYER_OWNS_TOWN = 18,
		PLAYER_IS_HUMAN = 19,
		PLAYER_STARTING_FACTION = 20,
		TOWN_IS_NEUTRAL = 21
	};

	HotaScriptCondition conditionCode = static_cast<HotaScriptCondition>(reader->readInt32());
	switch(conditionCode)
	{
		case HotaScriptCondition::CONSTANT:
		{
			bool value = reader->readBool();
			logGlobal->warn("Map %s: HotA Script condition - CONSTANT %d", mapName, value);
			break;
		}
		case HotaScriptCondition::ANY_OF:
		case HotaScriptCondition::ALL_OF:
		{
			int argumentsCount = reader->readInt32();
			for(int i = 0; i < argumentsCount; ++i)
				readHotaScriptConditionInternal();
			logGlobal->warn("Map %s: HotA Script condition - ANY_OF/ALL_OF, %d arguments", mapName, argumentsCount);
			break;
		}
		case HotaScriptCondition::LESSER_OR_EQUAL:
		case HotaScriptCondition::NOT_EQUAL:
		case HotaScriptCondition::GREATER_OR_EQUAL:
		case HotaScriptCondition::LESSER:
		case HotaScriptCondition::EQUAL:
		case HotaScriptCondition::GREATER:
		{
			readHotaScriptExpression();
			readHotaScriptExpression();
			logGlobal->warn("Map %s: HotA Script condition - (comparison check)", mapName);
			break;
		}
		case HotaScriptCondition::NOT:
		{
			readHotaScriptCondition();
			logGlobal->warn("Map %s: HotA Script condition - NOT", mapName);
			break;
		}
		case HotaScriptCondition::HAS_ARTIFACT:
		{
			ArtifactID artifact = reader->readArtifact32();
			SpellID scrollSpellID = reader->readSpell32();
			logGlobal->warn("Map %s: HotA Script condition - HAS_ARTIFACT, %d artifact, %d scroll spell", mapName, artifact.getNum(), scrollSpellID.getNum());
			break;
		}
		case HotaScriptCondition::CURRENT_PLAYER:
		{
			PlayerColor expectedPlayer = reader->readPlayer32();
			logGlobal->warn("Map %s: HotA Script condition - CURRENT_PLAYER, %d player", mapName, expectedPlayer.getNum());
			break;
		}
		case HotaScriptCondition::HERO_OWNER:
		{
			HeroTypeID expectedHero = reader->readHero32();
			PlayerColor expectedPlayer = reader->readPlayer32(); // -2 = current hero, -1 = current player
			logGlobal->warn("Map %s: HotA Script condition - HERO_OWNER $d hero, %d player", mapName, expectedHero.getNum(), expectedPlayer.getNum());
			break;
		}
		case HotaScriptCondition::HERO_SECONDARY_SKILL:
		{
			SecondarySkill expectedSkill = reader->readSkill32();
			int32_t expectedMastery = reader->readInt32();
			logGlobal->warn("Map %s: HotA Script condition - HERO_SECONDARY_SKILL, expectec %d skill with $d mastery", mapName, expectedSkill.getNum(), expectedMastery);
			break;
		}
		case HotaScriptCondition::TOWN_IS_NEUTRAL:
		{
			logGlobal->warn("Map %s: HotA Script condition - TOWN_IS_NEUTRAL", mapName);
			break;
		}
		case HotaScriptCondition::PLAYER_DEFEATED:
		{
			PlayerColor expectedPlayer = reader->readPlayer32();
			logGlobal->warn("Map %s: HotA Script condition - PLAYER_DEFEATED, %d player", mapName, expectedPlayer.getNum());
			break;
		}
		case HotaScriptCondition::PLAYER_IS_HUMAN:
		{
			PlayerColor expectedPlayer = reader->readPlayer32(); // -1 = current player
			logGlobal->warn("Map %s: HotA Script condition - PLAYER_IS_HUMAN, %d player", mapName, expectedPlayer.getNum());
			break;
		}
		case HotaScriptCondition::PLAYER_STARTING_FACTION:
		{
			PlayerColor expectedPlayer = reader->readPlayer32(); // -1 = current player
			FactionID expectedFaction = reader->readFaction32();
			logGlobal->warn("Map %s: HotA Script condition - PLAYER_STARTING_FACTION, %d player %d faction", mapName, expectedPlayer.getNum(), expectedFaction.getNum());
			break;
		}
		case HotaScriptCondition::PLAYER_DEFEATED_MONSTER:
		{
			PlayerColor expectedPlayer = reader->readPlayer32(); // -1 = current player
			int32_t targetObjectID = reader->readInt32(); // Quest identifier?
			logGlobal->warn("Map %s: HotA Script condition - PLAYER_DEFEATED_MONSTER, %d player %d object ID", mapName, expectedPlayer.getNum(), targetObjectID);
			break;
		}
		case HotaScriptCondition::PLAYER_DEFEATED_HERO:
		{
			PlayerColor expectedPlayer = reader->readPlayer32(); // -1 = current player
			int32_t targetObjectID = reader->readInt32(); // Quest identifier? Hero tyoe ID? Garbage???
			logGlobal->warn("Map %s: HotA Script condition - PLAYER_DEFEATED_HERO, %d player %d object ID", mapName, expectedPlayer.getNum(), targetObjectID);
			break;
		}
		case HotaScriptCondition::PLAYER_OWNS_TOWN:
		{
			PlayerColor expectedPlayer = reader->readPlayer32(); // -1 = current player
			int32_t targetObjectID = reader->readInt32(); // Quest identifier?
			logGlobal->warn("Map %s: HotA Script condition - PLAYER_OWNS_TOWN, %d player %d object ID", mapName, expectedPlayer.getNum(), targetObjectID);
			break;
		}
		default:
			throw std::runtime_error("Unknown event condition code:" + std::to_string(static_cast<int>(conditionCode)));
	}
}

void CMapLoaderH3M::readHotaScriptExpression()
{
	bool isExpression = reader->readBool();

	if(!isExpression)
	{
		int rawValue = reader->readInt32();
		logGlobal->warn("Map %s: HotA Script expression - RAW VALUE, %d value", mapName, rawValue);
		return;
	}

	readHotaScriptExpressionInternal();
}

void CMapLoaderH3M::readHotaScriptExpressionInternal()
{
	enum class HotaScriptExpression : int32_t
	{
		INTEGER_VALUE = 0,
		VARIABLE_VALUE = 1,
		NEGATE = 2,
		ADD = 3,
		SUBSTRACT = 4,
		RESOURCE = 5,
		MULTIPLY = 6,
		DIVIDE = 7,
		REMAINDER = 8,
		CREATURE_COUNT_IN_ARMY = 9,
		CURRENT_DIFFICULTY = 10,
		COMPARE_DIFFICULTY = 11,
		CURRENT_DATE = 12,
		HERO_EXPERIENCE = 13,
		HERO_LEVEL = 14,
		HERO_PRIMARY_SKILL = 15,
		RANDOM_NUMBER = 16,
		HERO_OWNED_ARTIFACTS = 17,
	};

	int unknown = reader->readBool();
	assert(unknown==true);
	logGlobal->warn("Map %s: HotA Script expression - unknown value %d", mapName, unknown);


	HotaScriptExpression expressionCode = static_cast<HotaScriptExpression>(reader->readInt32());
	switch(expressionCode)
	{
		case HotaScriptExpression::INTEGER_VALUE:
		{
			int value = reader->readInt32();
			logGlobal->warn("Map %s: HotA Script expression - INTEGER_VALUE %d", mapName, value);
			break;
		}
		case HotaScriptExpression::VARIABLE_VALUE:
		{
			int variableIndex = reader->readInt32();
			logGlobal->warn("Map %s: HotA Script expression - VARIABLE_VALUE %d", mapName, variableIndex);
			break;
		}
		case HotaScriptExpression::RANDOM_NUMBER:
		{
			readHotaScriptExpression();
			readHotaScriptExpression();
			logGlobal->warn("Map %s: HotA Script expression - RANDOM_NUMBER", mapName);
			break;
		}
		case HotaScriptExpression::ADD:
		case HotaScriptExpression::SUBSTRACT:
		case HotaScriptExpression::MULTIPLY:
		case HotaScriptExpression::DIVIDE:
		case HotaScriptExpression::REMAINDER:
		{
			readHotaScriptExpressionInternal();
			readHotaScriptExpressionInternal();
			logGlobal->warn("Map %s: HotA Script expression - (arithmetic)", mapName);
			break;
		}
		case HotaScriptExpression::NEGATE:
		{
			int unknown = reader->readInt32();
			readHotaScriptExpression();
			assert(unknown == 1);
			logGlobal->warn("Map %s: HotA Script expression - NEGATE, unknown %d", mapName, unknown);
			break;
		}
		case HotaScriptExpression::CREATURE_COUNT_IN_ARMY:
		{
			CreatureID creature = reader->readCreature32();
			logGlobal->warn("Map %s: HotA Script expression - CREATURE_COUNT_IN_ARMY, creature %d", mapName, creature.getNum());
			break;
		}
		case HotaScriptExpression::CURRENT_DIFFICULTY:
		{
			logGlobal->warn("Map %s: HotA Script expression - CURRENT_DIFFICULTY", mapName);
			break;
		}
		case HotaScriptExpression::COMPARE_DIFFICULTY:
		{
			// TODO: figure out what exactly this does
			int difficultyToCompare = reader->readInt32();
			logGlobal->warn("Map %s: HotA Script expression - COMPARE_DIFFICULTY, difficulty %d", mapName, difficultyToCompare);
			break;
		}
		case HotaScriptExpression::HERO_PRIMARY_SKILL:
		{
			PrimarySkill skill(reader->readInt32());
			logGlobal->warn("Map %s: HotA Script expression - HERO_PRIMARY_SKILL, skill %d", mapName, skill);
			break;
		}
		case HotaScriptExpression::CURRENT_DATE:
		{
			logGlobal->warn("Map %s: HotA Script expression - CURRENT_DATE", mapName);
			break;
		}
		case HotaScriptExpression::HERO_EXPERIENCE:
		{
			logGlobal->warn("Map %s: HotA Script expression - HERO_EXPERIENCE", mapName);
			break;
		}
		case HotaScriptExpression::HERO_LEVEL:
		{
			logGlobal->warn("Map %s: HotA Script expression - HERO_LEVEL", mapName);
			break;
		}
		case HotaScriptExpression::HERO_OWNED_ARTIFACTS:
		{
			ArtifactID artifact = reader->readArtifact32();
			SpellID scrollSpell = reader->readSpell32();
			logGlobal->warn("Map %s: HotA Script expression - HERO_OWNED_ARTIFACTS, %d artifact, %d scroll spell", mapName, artifact.getNum(), scrollSpell.getNum());
			break;
		}
		case HotaScriptExpression::RESOURCE:
		{
			PlayerColor player = reader->readPlayer(); // has special value for current player
			GameResID resource = reader->readGameResID32();
			logGlobal->warn("Map %s: HotA Script expression - RESOURCE, %d player, %d resource", mapName, player.getNum(), resource.getNum());
			break;
		}
		default:
			throw std::runtime_error("Unknown event expression code:" + std::to_string(static_cast<int>(expressionCode)));
	}
}

void CMapLoaderH3M::readAllowedArtifacts()
{
	map->allowedArtifact = LIBRARY->arth->getDefaultAllowed();

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
		for(auto const & artifact : LIBRARY->arth->objects)
			if(artifact->isCombined())
				map->allowedArtifact.erase(artifact->getId());
	}

	if(!features.levelAB)
	{
		map->allowedArtifact.erase(ArtifactID::VIAL_OF_DRAGON_BLOOD);
		map->allowedArtifact.erase(ArtifactID::ARMAGEDDONS_BLADE);
	}

	// Messy, but needed
	for(TriggeredEvent & event : map->triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			if(cond.condition == EventCondition::HAVE_ARTIFACT || cond.condition == EventCondition::TRANSPORT)
			{
				map->allowedArtifact.erase(cond.objectType.as<ArtifactID>());
			}
			return cond;
		};

		event.trigger = event.trigger.morph(patcher);
	}
}

void CMapLoaderH3M::readAllowedSpellsAbilities()
{
	map->allowedSpells = LIBRARY->spellh->getDefaultAllowed();
	map->allowedAbilities = LIBRARY->skillh->getDefaultAllowed();

	if(features.levelSOD)
	{
		reader->readBitmaskSpells(map->allowedSpells, true);
		reader->readBitmaskSkills(map->allowedAbilities, true);
	}
}

void CMapLoaderH3M::readRumors()
{
	size_t rumorsCount = reader->readUInt32();
	assert(rumorsCount < 1000); // sanity check

	for(size_t it = 0; it < rumorsCount; it++)
	{
		Rumor ourRumor;
		ourRumor.name = readBasicString();
		ourRumor.text.appendTextID(readLocalizedString(TextIdentifier("header", "rumor", it, "text")));
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

		auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, HeroTypeID(heroID).toHeroType()->heroClass->getIndex());
		auto object = handler->create(map->cb, handler->getTemplates().front());
		auto hero = std::dynamic_pointer_cast<CGHeroInstance>(object);
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
				hero->secSkills[yy].second = reader->readInt8Checked(1,3);
			}
		}

		loadArtifactsOfHero(hero.get());

		bool hasCustomBio = reader->readBool();
		if(hasCustomBio)
			hero->biographyCustomTextId = readLocalizedString(TextIdentifier("heroes", heroID, "biography"));

		// 0xFF is default, 00 male, 01 female
		hero->gender = static_cast<EHeroGender>(reader->readInt8Checked(-1, 1));
		assert(hero->gender == EHeroGender::MALE || hero->gender == EHeroGender::FEMALE || hero->gender == EHeroGender::DEFAULT);

		bool hasCustomSpells = reader->readBool();
		if(hasCustomSpells)
			reader->readBitmaskSpells(hero->spells, false);

		bool hasCustomPrimSkills = reader->readBool();
		if(hasCustomPrimSkills)
		{
			for(int skillID = 0; skillID < GameConstants::PRIMARY_SKILLS; skillID++)
			{
				hero->pushPrimSkill(static_cast<PrimarySkill>(skillID), reader->readUInt8());
			}
		}
		map->addToHeroPool(hero);

		logGlobal->debug("Map '%s': Hero predefined in map: %s", mapName, hero->getHeroType()->getJsonKey());
	}

	if(features.levelHOTA5)
	{
		for(int heroID = 0; heroID < heroesCount; heroID++)
		{
			bool alwaysAddSkills = reader->readBool(); // prevent heroes from receiving additional random secondary skills at the start of the map if they are not of the first level
			bool cannotGainXP = reader->readBool();
			int32_t level = reader->readInt32(); // Needs investigation how this interacts with usual setting of level via experience
			assert(level > 0);

			if (!alwaysAddSkills)
				logGlobal->warn("Map '%s': Option to prevent hero %d from gaining skills on map start is not implemented!", mapName, heroID);

			if (cannotGainXP)
				logGlobal->warn("Map '%s': Option to prevent hero %d from receiveing experience is not implemented!", mapName, heroID);

			if (level > 1)
				logGlobal->warn("Map '%s': Option to set level of hero %d to %d is not implemented!", mapName, heroID, level);
		}
	}
}

void CMapLoaderH3M::loadArtifactsOfHero(CGHeroInstance * hero)
{
	bool hasArtSet = reader->readBool();

	// True if artifact set is not default (hero has some artifacts)
	if(!hasArtSet)
		return;

	// Workaround - if hero has customized artifacts game should not attempt to add spellbook based on hero type
	hero->spells.insert(SpellID::SPELLBOOK_PRESET);

	if(!hero->artifactsWorn.empty() || !hero->artifactsInBackpack.empty())
	{
		logGlobal->debug("Hero %d at %s has set artifacts twice (in map properties and on adventure map instance). Using the latter set...", hero->getHeroTypeID().getNum(), hero->anchorPos().toString());

		hero->artifactsInBackpack.clear();
		hero->artifactsWorn.clear();
	}

	for(int i = 0; i < features.artifactSlotsCount; i++)
		loadArtifactToSlot(hero, i);

	// bag artifacts
	// number of artifacts in hero's bag
	size_t amount = reader->readUInt16();
	for(size_t i = 0; i < amount; ++i)
	{
		loadArtifactToSlot(hero, ArtifactPosition::BACKPACK_START + static_cast<int>(hero->artifactsInBackpack.size()));
	}
}

bool CMapLoaderH3M::loadArtifactToSlot(CGHeroInstance * hero, int slot)
{
	ArtifactID artifactID = reader->readArtifact();
	SpellID scrollSpell = SpellID::NONE;
	if (features.levelHOTA5)
		scrollSpell = reader->readSpell16();

	if(artifactID == ArtifactID::NONE)
		return false;

	const Artifact * art = artifactID.toEntity(LIBRARY);

	if(!art)
	{
		logGlobal->warn("Map '%s': Invalid artifact in hero's backpack, ignoring...", mapName);
		return false;
	}

	if(art->isBig() && slot >= ArtifactPosition::BACKPACK_START)
	{
		logGlobal->warn("Map '%s': A big artifact (war machine) in hero's backpack, ignoring...", mapName);
		return false;
	}

	// H3 bug workaround - Enemy hero on 3rd scenario of Good1.h3c campaign ("Long Live The Queen")
	// He has Shackles of War (normally - MISC slot artifact) in LEFT_HAND slot set in editor
	// Artifact seems to be missing in game, so skip artifacts that don't fit target slot
	if(ArtifactID(artifactID).toArtifact()->canBePutAt(hero, ArtifactPosition(slot)))
	{
		auto * artifact = map->createArtifact(artifactID, scrollSpell);
		map->putArtifactInstance(*hero, artifact->getId(), slot);
	}
	else
	{
		logGlobal->warn("Map '%s': Artifact '%s' can't be put at the slot %d", mapName, ArtifactID(artifactID).toArtifact()->getNameTranslated(), slot);
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
				tile.terrainType = reader->readTerrain();
				tile.terView = reader->readUInt8();
				tile.riverType = reader->readRiver();
				tile.riverDir = reader->readUInt8();
				tile.roadType = reader->readRoad();
				tile.roadDir = reader->readUInt8();
				tile.extTileFlags = reader->readUInt8();
			}
		}
	}
	map->calculateWaterContent();
}

void CMapLoaderH3M::readObjectTemplates()
{
	uint32_t defAmount = reader->readUInt32();

	originalTemplates.reserve(defAmount);
	remappedTemplates.reserve(defAmount);

	// Read custom defs
	for(int defID = 0; defID < defAmount; ++defID)
	{
		auto tmpl = reader->readObjectTemplate();
		originalTemplates.push_back(tmpl);

		auto remapped = std::make_shared<ObjectTemplate>(*tmpl);
		reader->remapTemplate(*remapped);
		remappedTemplates.push_back(remapped);

		if (!CResourceHandler::get()->existsResource(remapped->animationFile.addPrefix("SPRITES/")))
			logMod->warn("Template animation %s of type (%d %d) is missing!", remapped->animationFile.getOriginalName(), remapped->id, remapped->subid );
	}
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readEvent(const int3 & mapPosition, const ObjectInstanceID & idToBeGiven)
{
	auto object = std::make_shared<CGEvent>(map->cb);

	readBoxContent(object.get(), mapPosition, idToBeGiven);

	reader->readBitmaskPlayers(object->availableFor, false);
	object->computerActivate = reader->readBool();
	object->removeAfterVisit = reader->readBool();

	reader->skipZero(4);

	if(features.levelHOTA3)
		object->humanActivate = reader->readBool();
	else
		object->humanActivate = true;

	readBoxHotaContent(object.get(), mapPosition, idToBeGiven);

	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readPandora(const int3 & mapPosition, const ObjectInstanceID & idToBeGiven)
{
	auto object = std::make_shared<CGPandoraBox>(map->cb);
	readBoxContent(object.get(), mapPosition, idToBeGiven);

	if(features.levelHOTA5)
		reader->skipZero(1); // Unknown value, always 0 so far

	readBoxHotaContent(object.get(), mapPosition, idToBeGiven);

	return object;
}

void CMapLoaderH3M::readBoxContent(CGPandoraBox * object, const int3 & mapPosition, const ObjectInstanceID & idToBeGiven)
{
	readMessageAndGuards(object->message, object, mapPosition, idToBeGiven);
	Rewardable::VisitInfo vinfo;
	auto & reward = vinfo.reward;

	reward.heroExperience = reader->readUInt32();
	reward.manaDiff = reader->readInt32();
	if(auto val = reader->readInt8Checked(-3, 3))
		reward.heroBonuses.push_back(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::MORALE, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(idToBeGiven)));
	if(auto val = reader->readInt8Checked(-3, 3))
		reward.heroBonuses.push_back(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OBJECT_INSTANCE, val, BonusSourceID(idToBeGiven)));

	reader->readResources(reward.resources);
	for(int x = 0; x < GameConstants::PRIMARY_SKILLS; ++x)
		reward.primary.at(x) = reader->readUInt8();

	size_t gabn = reader->readUInt8(); //number of gained abilities
	for(size_t oo = 0; oo < gabn; ++oo)
	{
		auto rId = reader->readSkill();
		auto rVal = reader->readInt8Checked(1,3);

		reward.secondary[rId] = rVal;
	}
	size_t gart = reader->readUInt8(); //number of gained artifacts
	for(size_t oo = 0; oo < gart; ++oo)
	{
		ArtifactID grantedArtifact = reader->readArtifact();

		if (features.levelHOTA5)
		{
			SpellID scrollSpell = reader->readSpell16();
			if (grantedArtifact == ArtifactID::SPELL_SCROLL)
				reward.grantedScrolls.push_back(scrollSpell);
			else
				reward.grantedArtifacts.push_back(grantedArtifact);
		}
		else
			reward.grantedArtifacts.push_back(grantedArtifact);
	}

	size_t gspel = reader->readUInt8(); //number of gained spells
	for(size_t oo = 0; oo < gspel; ++oo)
		reward.spells.push_back(reader->readSpell());

	size_t gcre = reader->readUInt8(); //number of gained creatures
	for(size_t oo = 0; oo < gcre; ++oo)
	{
		auto rId = reader->readCreature();
		auto rVal = reader->readUInt16();

		reward.creatures.emplace_back(rId, rVal);
	}

	vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	object->configuration.info.push_back(vinfo);

	reader->skipZero(8);
}

void CMapLoaderH3M::readBoxHotaContent(CGPandoraBox * object, const int3 & mapPosition, const ObjectInstanceID & idToBeGiven)
{
	if(features.levelHOTA5)
	{
		int32_t movementMode = reader->readInt32(); // Give, Take, Nullify, Set, Replenish
		int32_t movementAmount = reader->readInt32();
		assert(movementMode >= 0 && movementMode <= 4);

		auto & boxReward = object->configuration.info.back();

		switch (movementMode)
		{
			case 0: // Give
				boxReward.reward.movePoints = movementAmount;
				boxReward.reward.moveOverflowFactor = 100;
				break;
			case 1: // Take
				boxReward.reward.movePoints = -movementAmount;
				break;
			case 2: // Nullify
				boxReward.reward.movePercentage = 0;
				break;
			case 3: // Set
				boxReward.reward.movePercentage = 0;
				boxReward.reward.movePoints = movementAmount;
				boxReward.reward.moveOverflowFactor = 100;
				break;
			case 4: // Replenish
				boxReward.reward.movePoints = movementAmount;
				boxReward.reward.moveOverflowFactor = 0;
				break;
		}
	}

	if(features.levelHOTA6)
	{
		int32_t allowedDifficultiesMask = reader->readInt32();
		assert(allowedDifficultiesMask > 0 && allowedDifficultiesMask < 32);
		object->presentOnDifficulties = MapDifficultySet(allowedDifficultiesMask);
	}

	if (features.levelHOTA9)
	{
		bool usesEventSystem = reader->readBool();
		if(usesEventSystem)
		{
			int32_t eventID = reader->readInt32();
			bool syncronizeObjects = reader->readBool();
			logGlobal->warn("Map %s: Extended script system (event ID %d, synchronize %d) for event/pandora at %s is not implemented!", mapName, eventID, syncronizeObjects, mapPosition.toString());
		}
	}
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readMonster(const int3 & mapPosition, const ObjectInstanceID & objectInstanceID)
{
	auto object = std::make_shared<CGCreature>(map->cb);
	object->id = objectInstanceID;

	if(features.levelAB)
	{
		int32_t identifier = reader->readUInt32();
		questIdentifierToId[identifier] = objectInstanceID;
	}

	auto hlp = std::make_unique<CStackInstance>(map->cb);
	hlp->setCount(reader->readUInt16());

	//type will be set during initialization
	object->putStack(SlotID(0), std::move(hlp));

	//TODO: 0-4 is h3 range. 5 is hota extension for exact aggression?
	object->initialCharacter = static_cast<CGCreature::Character>(reader->readInt8Checked(0, 5));

	bool hasMessage = reader->readBool();
	if(hasMessage)
	{
		object->message.appendTextID(readLocalizedString(TextIdentifier("monster", mapPosition.x, mapPosition.y, mapPosition.z, "message")));
		reader->readResources(object->resources);
		object->gainedArtifact = reader->readArtifact();
	}
	object->neverFlees = reader->readBool();
	object->notGrowingTeam = reader->readBool();
	reader->skipZero(2);

	if(features.levelHOTA3)
	{
		//TODO: HotA

		// -1 = default, 1-10 = possible values range
		object->agression = reader->readInt32();
		// if true, monsters will only join for money
		object->joinOnlyForMoney = reader->readBool();
		// 100 = default, percent of monsters that will join on successful aggression check
		object->joiningPercentage = reader->readInt32();
		// Presence of upgraded stack, -1 = random, 0 = never, 1 = always
		object->upgradedStackPresence = static_cast<CGCreature::UpgradedStackPresence>(reader->readInt32());
		// How many creature stacks will be present on battlefield, -1 = default
		object->stacksCount = reader->readInt32();
	}

	if (features.levelHOTA5)
	{
		bool sizeByValue = reader->readBool();//FIXME: double-check this flag effect
		int32_t targetValue = reader->readInt32();

		if (sizeByValue || targetValue)
			logGlobal->warn( "Map '%s': Wandering monsters %s option to set unit size to %d (%d) AI value is not implemented!", mapName, mapPosition.toString(), targetValue, sizeByValue);
	}

	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readSign(const int3 & mapPosition)
{
	auto object = std::make_shared<CGSignBottle>(map->cb);
	object->message.appendTextID(readLocalizedString(TextIdentifier("sign", mapPosition.x, mapPosition.y, mapPosition.z, "message")));
	reader->skipZero(4);
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readWitchHut(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(position, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	// AB and later maps have allowed abilities defined in H3M
	if(features.levelAB)
	{
		std::set<SecondarySkill> allowedAbilities;
		reader->readBitmaskSkills(allowedAbilities, false);

		if (rewardable)
		{
			if(allowedAbilities.size() != 1)
			{
				auto defaultAllowed = LIBRARY->skillh->getDefaultAllowed();

				for(int skillID = features.skillsCount; skillID < defaultAllowed.size(); ++skillID)
					if(defaultAllowed.count(skillID))
						allowedAbilities.insert(SecondarySkill(skillID));
			}

			JsonNode variable;
			if (allowedAbilities.size() == 1)
			{
				variable.String() = LIBRARY->skills()->getById(*allowedAbilities.begin())->getJsonKey();
			}
			else
			{
				JsonVector anyOfList;
				for (auto const & skill : allowedAbilities)
				{
					JsonNode entry;
					entry.String() = LIBRARY->skills()->getById(skill)->getJsonKey();
					anyOfList.push_back(entry);
				}
				variable["anyOf"].Vector() = anyOfList;
			}

			variable.setModScope(ModScope::scopeGame()); // list may include skills from all mods
			rewardable->configuration.presetVariable("secondarySkill", "gainedSkill", variable);
		}
		else
		{
			logGlobal->warn("Failed to set allowed secondary skills to a Witch Hut! Object is not rewardable!");
		}
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readScholar(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	enum class ScholarBonusType : int8_t {
		RANDOM = -1,
		PRIM_SKILL = 0,
		SECONDARY_SKILL = 1,
		SPELL = 2,
	};

	auto object = readGeneric(position, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	uint8_t bonusTypeRaw = reader->readInt8Checked(-1, 2);
	auto bonusType = static_cast<ScholarBonusType>(bonusTypeRaw);
	auto bonusID = reader->readUInt8();

	if (rewardable)
	{
		switch (bonusType)
		{
			case ScholarBonusType::PRIM_SKILL:
			{
				JsonNode variable;
				variable.String() = NPrimarySkill::names[bonusID];
				variable.setModScope(ModScope::scopeGame());
				rewardable->configuration.presetVariable("primarySkill", "gainedStat", variable);
				rewardable->configuration.presetVariable("dice", "map", JsonNode(2));
				break;
			}
			case ScholarBonusType::SECONDARY_SKILL:
			{
				JsonNode variable;
				variable.String() = LIBRARY->skills()->getByIndex(bonusID)->getJsonKey();
				variable.setModScope(ModScope::scopeGame());
				rewardable->configuration.presetVariable("secondarySkill", "gainedSkill", variable);
				rewardable->configuration.presetVariable("dice", "map", JsonNode(1));
				break;
			}
			case ScholarBonusType::SPELL:
			{
				JsonNode variable;
				variable.String() = LIBRARY->spells()->getByIndex(bonusID)->getJsonKey();
				variable.setModScope(ModScope::scopeGame());
				rewardable->configuration.presetVariable("spell", "gainedSpell", variable);
				rewardable->configuration.presetVariable("dice", "map", JsonNode(0));
				break;
			}
			case ScholarBonusType::RANDOM:
				break;// No-op
			default:
				logGlobal->warn("Map '%s': Invalid Scholar settings! Ignoring...", mapName);
		}
	}
	else
	{
		logGlobal->warn("Failed to set reward parameters for a Scholar! Object is not rewardable!");
	}

	reader->skipZero(6);
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readGarrison(const int3 & mapPosition, const ObjectInstanceID & idToBeGiven)
{
	auto object = std::make_shared<CGGarrison>(map->cb);

	setOwnerAndValidate(mapPosition, object.get(), reader->readPlayer32());
	readCreatureSet(object.get(), idToBeGiven);
	if(features.levelAB)
		object->removableUnits = reader->readBool();
	else
		object->removableUnits = true;

	reader->skipZero(8);
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readArtifact(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate, const ObjectInstanceID & idToBeGiven)
{
	ArtifactID artID = ArtifactID::NONE; //random, set later
	auto object = std::make_shared<CGArtifact>(map->cb);

	readMessageAndGuards(object->message, object.get(), mapPosition, idToBeGiven);

	//specific artifact
	if(objectTemplate->id == Obj::ARTIFACT)
		artID = ArtifactID(objectTemplate->subid);

	if(features.levelHOTA5)
	{
		uint32_t pickupMode = reader->readUInt32();
		uint8_t pickupFlags = reader->readUInt8();

		assert(pickupMode == 0 || pickupMode == 1 || pickupMode == 2); // DISABLED, RANDOM, CUSTOM

		if (pickupMode != 0)
			logGlobal->warn("Map '%s': Artifact %s: not implemented pickup mode %d (flags: %d)", mapName, mapPosition.toString(), pickupMode, static_cast<int>(pickupFlags));
	}

	if (artID.hasValue())
		object->setArtifactInstance(map->createArtifact(artID, SpellID::NONE));
	// else - random, will be initialized later
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readScroll(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate, const ObjectInstanceID & idToBeGiven)
{
	auto object = std::make_shared<CGArtifact>(map->cb);
	readMessageAndGuards(object->message, object.get(), mapPosition, idToBeGiven);
	SpellID spellID = reader->readSpell32();

	object->setArtifactInstance(map->createArtifact(ArtifactID::SPELL_SCROLL, spellID.getNum()));
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readResource(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate, const ObjectInstanceID & idToBeGiven)
{
	auto object = std::make_shared<CGResource>(map->cb);

	readMessageAndGuards(object->message, object.get(), mapPosition, idToBeGiven);

	object->amount = reader->readUInt32();

	if (objectTemplate->id != Obj::RANDOM_RESOURCE)
	{
		const auto & baseHandler = LIBRARY->objtypeh->getHandlerFor(objectTemplate->id, objectTemplate->subid);
		const auto & ourHandler = std::dynamic_pointer_cast<ResourceInstanceConstructor>(baseHandler);

		object->amount *= ourHandler->getAmountMultiplier();
	}

	reader->skipZero(4);
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readMine(const int3 & mapPosition)
{
	auto object = std::make_shared<CGMine>(map->cb);
	setOwnerAndValidate(mapPosition, object.get(), reader->readPlayer32());
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readAbandonedMine(const int3 & mapPosition)
{
	auto object = std::make_shared<CGMine>(map->cb);
	object->setOwner(PlayerColor::NEUTRAL);
	reader->readBitmaskResources(object->abandonedMineResources, false);

	if(features.levelHOTA5)
	{
		bool hasCustomGuards = reader->readBool();
		if (hasCustomGuards)
		{
			object->abandonedMineGuards.creature = reader->readCreature32();
			object->abandonedMineGuards.minAmount = reader->readInt32();
			object->abandonedMineGuards.maxAmount = reader->readInt32();
		}
		else
			reader->skipUnused(12);
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readDwelling(const int3 & position)
{
	auto object = std::make_shared<CGDwelling>(map->cb);
	setOwnerAndValidate(position, object.get(), reader->readPlayer32());
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readDwellingRandom(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = std::make_shared<CGDwelling>(map->cb);

	setOwnerAndValidate(mapPosition, object.get(), reader->readPlayer32());

	object->randomizationInfo = CGDwellingRandomizationInfo();

	bool hasFactionInfo = objectTemplate->id == Obj::RANDOM_DWELLING || objectTemplate->id == Obj::RANDOM_DWELLING_LVL;
	bool hasLevelInfo = objectTemplate->id == Obj::RANDOM_DWELLING || objectTemplate->id == Obj::RANDOM_DWELLING_FACTION;

	if (hasFactionInfo)
	{
		object->randomizationInfo->identifier = reader->readUInt32();

		if(object->randomizationInfo->identifier == 0)
			reader->readBitmaskFactions(object->randomizationInfo->allowedFactions, false);
	}
	else
		object->randomizationInfo->allowedFactions.insert(FactionID(objectTemplate->subid));

	if(hasLevelInfo)
	{
		object->randomizationInfo->minLevel = std::max(reader->readUInt8(), static_cast<ui8>(0)) + 1;
		object->randomizationInfo->maxLevel = std::min(reader->readUInt8(), static_cast<ui8>(6)) + 1;
	}
	else
	{
		object->randomizationInfo->minLevel = objectTemplate->subid;
		object->randomizationInfo->maxLevel = objectTemplate->subid;
	}

	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readShrine(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(position, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	SpellID spell = reader->readSpell32();

	if (rewardable)
	{
		if(spell != SpellID::NONE)
		{
			JsonNode variable;
			variable.String() = LIBRARY->spells()->getById(spell)->getJsonKey();
			variable.setModScope(ModScope::scopeGame()); // list may include spells from all mods
			rewardable->configuration.presetVariable("spell", "gainedSpell", variable);
		}
	}
	else
	{
		logGlobal->warn("Failed to set selected spell to a Shrine!. Object is not rewardable!");
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readHeroPlaceholder(const int3 & mapPosition)
{
	auto object = std::make_shared<CGHeroPlaceholder>(map->cb);

	setOwnerAndValidate(mapPosition, object.get(), reader->readPlayer());

	HeroTypeID htid = reader->readHero(); //hero type id

	if(htid.getNum() == -1)
	{
		object->powerRank = reader->readUInt8();
		logGlobal->debug("Map '%s': Hero placeholder: by power at %s, owned by %s", mapName, mapPosition.toString(), object->getOwner().toString());
	}
	else
	{
		object->heroType = htid;
		logGlobal->debug("Map '%s': Hero placeholder: %s at %s, owned by %s", mapName, LIBRARY->heroh->getById(htid)->getJsonKey(), mapPosition.toString(), object->getOwner().toString());
	}

	if(features.levelHOTA5)
	{
		bool customizedStatingUnits = reader->readBool();

		if (customizedStatingUnits)
			logGlobal->warn("Map '%s': Hero placeholder: not implemented option to customize starting units", mapName);

		for (int i = 0; i < 7; ++i)
		{
			int32_t unitAmount = reader->readInt32();
			CreatureID unitToGive = reader->readCreature32();

			if (unitToGive.hasValue())
				logGlobal->warn("Map '%s': Hero placeholder: not implemented option to give %d units of type %d on map start to slot %d is not implemented!", mapName, unitAmount, unitToGive.toEntity(LIBRARY)->getJsonKey(), i);
		}

		int32_t artifactsToGive	= reader->readInt32();
		assert(artifactsToGive >= 0);
		assert(artifactsToGive < 100); // technically legal, but not possible in h3

		for (int i = 0; i < artifactsToGive; ++i)
		{
			// NOTE: this might actually be 2 bytes for artifact ID + 2 bytes for spell scroll
			ArtifactID startingArtifact = reader->readArtifact32();
			logGlobal->warn("Map '%s': Hero placeholder: not implemented option to give hero artifact %d", mapName, startingArtifact.toEntity(LIBRARY)->getJsonKey());
		}
	}

	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readGrail(const int3 & mapPosition)
{
	map->grailPos = mapPosition;
	map->grailRadius = reader->readInt32();
	return nullptr;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readHotaBattleLocation(const int3 & mapPosition)
{
	// Battle location for arena mode in HotA
	logGlobal->warn("Map '%s': Arena mode is not supported!", mapName);
	return nullptr;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readGeneric(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	if(LIBRARY->objtypeh->knownSubObjects(objectTemplate->id).count(objectTemplate->subid))
		return LIBRARY->objtypeh->getHandlerFor(objectTemplate->id, objectTemplate->subid)->create(map->cb, objectTemplate);

	logGlobal->warn("Map '%s': Unrecognized object %d:%d ('%s') at %s found!", mapName, objectTemplate->id.toEnum(), objectTemplate->subid, objectTemplate->animationFile.getOriginalName(), mapPosition.toString());
	return std::make_shared<CGObjectInstance>(map->cb);
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readQuestGuard(const int3 & mapPosition)
{
	auto guard = std::make_shared<CGQuestGuard>(map->cb);
	readQuest(guard.get(), mapPosition);
	return guard;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readShipyard(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	setOwnerAndValidate(mapPosition, object.get(), reader->readPlayer32());
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readLighthouse(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	setOwnerAndValidate(mapPosition, object.get(), reader->readPlayer32());
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readBank(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA3)
	{
		// index of guards preset. -1 = random, 0-4 = index of possible guards settings
		int32_t guardsPresetIndex = reader->readInt32();

		//TODO: HotA
		// presence of upgraded stack: -1 = random, 0 = never, 1 = always
		int8_t upgradedStackPresence = reader->readInt8Checked(-1, 1);

		assert(vstd::iswithin(guardsPresetIndex, -1, 4));
		assert(vstd::iswithin(upgradedStackPresence, -1, 1));

		// list of possible artifacts in reward
		// - if list is empty, artifacts are either not present in reward or random
		// - if non-empty, then list always have same number of elements as number of artifacts in bank
		// - ArtifactID::NONE indictates random artifact, other values indicate artifact that should be used as reward
		std::vector<ArtifactID> artifacts;
		int artNumber = reader->readUInt32();
		for(int yy = 0; yy < artNumber; ++yy)
			artifacts.push_back(reader->readArtifact32());

		if(rewardable && guardsPresetIndex != -1)
			rewardable->configuration.presetVariable("dice", "map", JsonNode(guardsPresetIndex));

		if(upgradedStackPresence != -1 || !artifacts.empty())
			logGlobal->warn(
				"Map '%s': creature bank at %s settings %d %d are not implemented!",
				mapName,
				mapPosition.toString(),
				int(upgradedStackPresence),
				artifacts.size()
			);
	}

	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readRewardWithGarbage(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();

		if(content != -1 && rewardable)
			rewardable->configuration.presetVariable("dice", "map", JsonNode(content));

		reader->skipUnused(4); // garbage data, usually -1, but sometimes uninitialized
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readPyramid(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();
		if(content == 0)
		{
			SpellID spell = reader->readSpell32();
			if(rewardable && spell.hasValue())
			{
				JsonNode variable;
				variable.String() = spell.toSpell()->getJsonKey();
				variable.setModScope(ModScope::scopeGame());
				rewardable->configuration.presetVariable("spell", "gainedSpell", variable);
			}
		}
		else
			reader->skipUnused(4); // garbage data, usually -1, but sometimes uninitialized
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readRewardWithArtifact(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate, int artifactRewardIndex)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();

		if(content != -1)
		{
			if(rewardable)
				rewardable->configuration.presetVariable("dice", "map", JsonNode(content));

			if(content == artifactRewardIndex)
			{
				ArtifactID artifact = reader->readArtifact32(); // NOTE: might be 2 byte artifact + 2 bytes scroll spell
				if(rewardable && artifact.hasValue())
				{
					JsonNode variable;
					variable.String() = artifact.toArtifact()->getJsonKey();
					variable.setModScope(ModScope::scopeGame());
					rewardable->configuration.presetVariable("artifact", "gainedArtifact", variable);
				}
			}
			else
				reader->skipUnused(4); // garbage data, usually -1, but sometimes uninitialized
		}
		else
			reader->skipUnused(4); // garbage data, usually -1, but sometimes uninitialized
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readBlackMarket(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	if(features.levelHOTA5)
	{
		for (int i = 0; i < 7; ++i)
		{
			ArtifactID artifact = reader->readArtifact();
			SpellID spellID = reader->readSpell16();

			if (artifact.hasValue())
			{
				if (artifact != ArtifactID::SPELL_SCROLL)
					logGlobal->warn("Map '%s': Black Market at %s: option to sell artifact %s is not implemented", mapName, mapPosition.toString(), artifact.toEntity(LIBRARY)->getJsonKey());
				else
					logGlobal->warn("Map '%s': Black Market at %s: option to sell scroll %s is not implemented", mapName, mapPosition.toString(), spellID.toEntity(LIBRARY)->getJsonKey());
			}
		}
	}
	return readGeneric(mapPosition, objectTemplate);
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readUniversity(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	if(features.levelHOTA5)
	{
		int32_t customized = reader->readInt32();

		std::set<SecondarySkill> allowedSkills;
		reader->readBitmaskSkills(allowedSkills, false);

		// NOTE: check how this interacts with hota Seafaring Academy that is guaranteed to give Navigation
		assert(customized == -1 || customized == 0);
		if (customized != -1)
			logGlobal->warn("Map '%s': University at %s: option to give specific skills out of %d is not implemented", mapName, mapPosition.toString(), allowedSkills.size());
	}
	return readGeneric(mapPosition, objectTemplate);
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readHotaGrave(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();

		if (content != -1)
		{
			ArtifactID artifact = reader->readArtifact32();
			int32_t amountA = reader->readInt32();
			GameResID resourceA = reader->readGameResID();
			reader->skipUnused(5); // no 2nd resource

			if(rewardable)
			{
				JsonNode variable;
				variable.setModScope(ModScope::scopeGame());
				variable.String() = artifact.toEntity(LIBRARY)->getJsonKey();
				rewardable->configuration.presetVariable("artifact", "gainedArtifact", variable);

				variable.String() = resourceA.toEntity(LIBRARY)->getJsonKey();
				rewardable->configuration.presetVariable("resource", "gainedResource", variable);

				rewardable->configuration.presetVariable("number", "gainedAmount", JsonNode(amountA));
			}
		}
		else
			reader->skipUnused(14); // garbage data
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readWagon(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();

		switch(content)
		{
			case -1: // random
				reader->skipUnused(14); // garbage data
				break;
			case 1: // empty
				reader->skipUnused(14); // garbage data
				if(rewardable)
					rewardable->configuration.presetVariable("dice", "map", JsonNode(2));
				break;
			case 0: // custom
			{
				ArtifactID artifact = reader->readArtifact32();
				int32_t amountA = reader->readInt32();
				GameResID resourceA = reader->readGameResID();
				reader->skipUnused(5); // no 2nd resource

				if(rewardable)
				{
					if (artifact.hasValue())
					{
						JsonNode variable;
						variable.setModScope(ModScope::scopeGame());
						variable.String() = artifact.toEntity(LIBRARY)->getJsonKey();
						rewardable->configuration.presetVariable("artifact", "gainedArtifact", variable);
						rewardable->configuration.presetVariable("dice", "map", JsonNode(0));
					}
					else
					{
						JsonNode variable;
						variable.setModScope(ModScope::scopeGame());
						variable.String() = resourceA.toEntity(LIBRARY)->getJsonKey();
						rewardable->configuration.presetVariable("resource", "gainedResource", variable);
						rewardable->configuration.presetVariable("dice", "map", JsonNode(1));
						rewardable->configuration.presetVariable("number", "gainedAmount", JsonNode(amountA));

					}
				}
			}
		}
	}
	return object;
}


std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readRewardWithAmount(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	// TODO
	// Ancient Lamp / 0 -> aID = -1, aA = amount to recruit, rA = 0, aB = 1, rB = 0

	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();

		switch(content)
		{
			case -1: // random
				reader->skipUnused(14); // garbage data
				break;
			case 0: // custom
			{
				reader->skipUnused(4); // no artifact
				int32_t amountA = reader->readInt32();
				reader->skipUnused(6); // no 1st resource ID, no 2nd resource

				if(rewardable)
					rewardable->configuration.presetVariable("number", "gainedAmount", JsonNode(amountA));
			}
		}
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readHotaTrapperLodge(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA9)
	{
		// -1 = random
		// 0 = gold
		// 1 = creatures
		int32_t content = reader->readInt32();
		int32_t goldAmount = reader->readInt32();
		int32_t creatureAmount = reader->readInt32();
		CreatureID creatureType = reader->readCreature32();

		if(content != -1)
		{
			if(rewardable)
			{
				JsonNode variable;
				variable.setModScope(ModScope::scopeGame());

				variable.String() = creatureType.toEntity(LIBRARY)->getJsonKey();
				rewardable->configuration.presetVariable("creature", "gainedCreature", variable);

				rewardable->configuration.presetVariable("dice", "map", JsonNode(content));
				rewardable->configuration.presetVariable("number", "gainedCreatureAmount", JsonNode(creatureAmount));
				rewardable->configuration.presetVariable("number", "gainedGoldAmount", JsonNode(goldAmount));
			}
		}
	}
	return object;
}


std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readLeanTo(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();

		if(content != -1)
		{
			reader->skipUnused(4); // no artifact
			int32_t amountA = reader->readInt32();
			GameResID resourceA = reader->readGameResID();
			reader->skipUnused(5); // no 2nd resource

			if(rewardable)
			{
				JsonNode variable;
				variable.setModScope(ModScope::scopeGame());

				variable.String() = resourceA.toEntity(LIBRARY)->getJsonKey();
				rewardable->configuration.presetVariable("dice", "map", JsonNode(content));
				rewardable->configuration.presetVariable("resource", "gainedResource", variable);
				rewardable->configuration.presetVariable("number", "gainedAmount", JsonNode(amountA));
			}
		}
		else
			reader->skipUnused(14); // garbage data
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readCampfire(const int3 & mapPosition, std::shared_ptr<const ObjectTemplate> objectTemplate)
{
	auto object = readGeneric(mapPosition, objectTemplate);
	auto rewardable = std::dynamic_pointer_cast<CRewardableObject>(object);

	if(features.levelHOTA5)
	{
		int32_t content = reader->readInt32();

		if(content != -1)
		{
			reader->skipUnused(4); // no artifact
			int32_t amountA = reader->readInt32();
			GameResID resourceA = reader->readGameResID();
			int32_t amountB = reader->readInt32();
			GameResID resourceB = reader->readGameResID();

			if(rewardable)
			{
				JsonNode variable;
				variable.setModScope(ModScope::scopeGame());

				variable.String() = resourceA.toEntity(LIBRARY)->getJsonKey();
				rewardable->configuration.presetVariable("resource", "gainedResourceA", variable);

				variable.String() = resourceB.toEntity(LIBRARY)->getJsonKey();
				rewardable->configuration.presetVariable("resource", "gainedResourceB", variable);

				rewardable->configuration.presetVariable("dice", "map", JsonNode(content));
				rewardable->configuration.presetVariable("number", "gainedAmountA", JsonNode(amountA));
				rewardable->configuration.presetVariable("number", "gainedAmountB", JsonNode(amountB));
			}
		}
		else
			reader->skipUnused(14); // garbage data
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readObject(MapObjectID id, MapObjectSubID subid, std::shared_ptr<const ObjectTemplate> objectTemplate, const int3 & mapPosition, const ObjectInstanceID & objectInstanceID)
{
	switch(id.toEnum())
	{
		case Obj::EVENT:
			return readEvent(mapPosition, objectInstanceID);

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
			return readSeerHut(mapPosition, objectInstanceID);

		case Obj::WITCH_HUT:
			return readWitchHut(mapPosition, objectTemplate);
		case Obj::SCHOLAR:
			return readScholar(mapPosition, objectTemplate);

		case Obj::GARRISON:
		case Obj::GARRISON2:
			return readGarrison(mapPosition, objectInstanceID);

		case Obj::ARTIFACT:
		case Obj::RANDOM_ART:
		case Obj::RANDOM_TREASURE_ART:
		case Obj::RANDOM_MINOR_ART:
		case Obj::RANDOM_MAJOR_ART:
		case Obj::RANDOM_RELIC_ART:
			return readArtifact(mapPosition, objectTemplate, objectInstanceID);
		case Obj::SPELL_SCROLL:
			return readScroll(mapPosition, objectTemplate, objectInstanceID);

		case Obj::RANDOM_RESOURCE:
		case Obj::RESOURCE:
			return readResource(mapPosition, objectTemplate, objectInstanceID);
		case Obj::RANDOM_TOWN:
		case Obj::TOWN:
			return readTown(mapPosition, objectTemplate, objectInstanceID);

		case Obj::MINE:
		case Obj::ABANDONED_MINE:
			if (subid < 7)
				return readMine(mapPosition);
			else
				return readAbandonedMine(mapPosition);

		case Obj::CREATURE_GENERATOR1:
		case Obj::CREATURE_GENERATOR2:
		case Obj::CREATURE_GENERATOR3:
		case Obj::CREATURE_GENERATOR4:
			return readDwelling(mapPosition);

		case Obj::SHRINE_OF_MAGIC_INCANTATION:
		case Obj::SHRINE_OF_MAGIC_GESTURE:
		case Obj::SHRINE_OF_MAGIC_THOUGHT:
			return readShrine(mapPosition, objectTemplate);

		case Obj::PANDORAS_BOX:
			return readPandora(mapPosition, objectInstanceID);

		case Obj::GRAIL:
			if (subid < 1000)
				return readGrail(mapPosition);
			else
				return readHotaBattleLocation(mapPosition);

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

		case Obj::LIGHTHOUSE:
			return readLighthouse(mapPosition, objectTemplate);

		case Obj::CREATURE_BANK:
		case Obj::DERELICT_SHIP:
		case Obj::DRAGON_UTOPIA:
		case Obj::CRYPT:
		case Obj::SHIPWRECK:
			return readBank(mapPosition, objectTemplate);

		case Obj::PYRAMID:
			return readPyramid(mapPosition, objectTemplate);

		case Obj::TREASURE_CHEST:
			return readRewardWithArtifact(mapPosition, objectTemplate, 3);

		case Obj::CORPSE:
			return readRewardWithArtifact(mapPosition, objectTemplate, 1);

		case Obj::WARRIORS_TOMB:
		case Obj::SHIPWRECK_SURVIVOR:
			return readRewardWithArtifact(mapPosition, objectTemplate, 0);

		case Obj::SEA_CHEST:
			return readRewardWithArtifact(mapPosition, objectTemplate, 2);

		case Obj::FLOTSAM:
		case Obj::TREE_OF_KNOWLEDGE:
			return readRewardWithGarbage(mapPosition, objectTemplate);

		case Obj::CAMPFIRE:
			return readCampfire(mapPosition, objectTemplate);

		case Obj::LEAN_TO:
			return readLeanTo(mapPosition, objectTemplate);

		case Obj::WAGON:
			return readWagon(mapPosition, objectTemplate);

		case Obj::BORDER_GATE:
			if (subid == 1000) // HotA hacks - Quest Gate
				return readQuestGuard(mapPosition);
			if (subid == 1001) // HotA hacks - Grave
				return readHotaGrave(mapPosition, objectTemplate);
			return readGeneric(mapPosition, objectTemplate);

		case Obj::HOTA_CUSTOM_OBJECT_1:
			// 0 -> Ancient Lamp
			// 1 -> Sea Barrel
			// 2 -> Jetsam
			// 3 -> Vial of Mana
			if (subid == 0)
				return readRewardWithAmount(mapPosition, objectTemplate);
			if (subid == 1)
				return readLeanTo(mapPosition, objectTemplate);
			else
				return readRewardWithGarbage(mapPosition, objectTemplate);

		case Obj::HOTA_CUSTOM_OBJECT_2:
			if (subid == 0) // Seafaring Academy
				return readUniversity(mapPosition, objectTemplate);
			else
				return readGeneric(mapPosition, objectTemplate);

		case Obj::HOTA_CUSTOM_OBJECT_3:
			if (subid == 12) // Trapper Lodge?
				return readHotaTrapperLodge(mapPosition, objectTemplate);
			else
				return readGeneric(mapPosition, objectTemplate);

		case Obj::BLACK_MARKET:
			return readBlackMarket(mapPosition, objectTemplate);

		case Obj::UNIVERSITY:
			return readUniversity(mapPosition, objectTemplate);

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
		assert(map->isInTheMap(mapPosition) || map->isInTheMap(mapPosition - int3(0,8,0)) || map->isInTheMap(mapPosition - int3(8,0,0)) || map->isInTheMap(mapPosition - int3(8,8,0)));

		uint32_t defIndex = reader->readUInt32();

		std::shared_ptr<ObjectTemplate> originalTemplate = originalTemplates.at(defIndex);
		std::shared_ptr<ObjectTemplate> remappedTemplate = remappedTemplates.at(defIndex);
		auto originalID = originalTemplate->id;
		auto originalSubID = originalTemplate->subid;
		reader->skipZero(5);

		ObjectInstanceID newObjectID = map->allocateUniqueInstanceID();
		auto newObject = readObject(originalID, originalSubID, remappedTemplate, mapPosition, newObjectID);

		if(!newObject)
			continue;

		newObject->setAnchorPos(mapPosition);
		newObject->ID = remappedTemplate->id;
		newObject->id = newObjectID;
		if(newObject->ID != Obj::HERO && newObject->ID != Obj::HERO_PLACEHOLDER && newObject->ID != Obj::PRISON)
		{
			newObject->subID = remappedTemplate->subid;
		}
		newObject->appearance = remappedTemplate;

		if (newObject->isVisitable() && !map->isInTheMap(newObject->visitablePos()))
			logGlobal->error("Map '%s': Object at %s - outside of map borders!", mapName, mapPosition.toString());

		map->generateUniqueInstanceName(newObject.get());
		map->addNewObject(newObject);
	}
}

void CMapLoaderH3M::readCreatureSet(CArmedInstance * out, const ObjectInstanceID & idToBeGiven)
{
	constexpr int unitsToRead = 7;
	out->id = idToBeGiven;

	for(int index = 0; index < unitsToRead; ++index)
	{
		CreatureID creatureID = reader->readCreature();
		int count = reader->readUInt16();

		// Empty slot
		if(creatureID == CreatureID::NONE)
			continue;

		auto result = std::make_unique<CStackInstance>(map->cb);
		result->setCount(count);

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

		out->putStack(SlotID(index), std::move(result));
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

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readHero(const int3 & mapPosition, const ObjectInstanceID & objectInstanceID)
{
	if(features.levelAB)
	{
		unsigned int identifier = reader->readUInt32();
		questIdentifierToId[identifier] = objectInstanceID;
	}

	PlayerColor owner = reader->readPlayer();
	HeroTypeID heroType = reader->readHero();

	//If hero of this type has been predefined, use that as a base.
	//Instance data will overwrite the predefined values where appropriate.
	std::shared_ptr<CGHeroInstance> object;

	if (heroType.hasValue())
		object = map->tryTakeFromHeroPool(heroType);

	if (!object)
	{
		object = std::make_shared<CGHeroInstance>(map->cb);
		object->subID = heroType.getNum();
	}

	setOwnerAndValidate(mapPosition, object.get(), owner);

	for(auto & elem : map->disposedHeroes)
	{
		if(elem.heroId == object->getHeroTypeID())
		{
			object->nameCustomTextId = elem.name;
			object->customPortraitSource = elem.portrait;
			break;
		}
	}

	bool hasName = reader->readBool();
	if(hasName)
		object->nameCustomTextId = readLocalizedString(TextIdentifier("heroes", object->getHeroTypeID().getNum(), "name"));

	if(features.levelSOD)
	{
		bool hasCustomExperience = reader->readBool();
		if(hasCustomExperience)
			object->exp = reader->readUInt32();
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
		object->customPortraitSource = reader->readHeroPortrait();

	bool hasSecSkills = reader->readBool();
	if(hasSecSkills)
	{
		if(!object->secSkills.empty())
		{
			if(object->secSkills[0].first != SecondarySkill::NONE)
				logGlobal->debug("Map '%s': Hero %s subID=%d has set secondary skills twice (in map properties and on adventure map instance). Using the latter set...", mapName, object->getNameTextID(), object->subID);
			object->secSkills.clear();
		}

		uint32_t skillsCount = reader->readUInt32();
		object->secSkills.resize(skillsCount);
		for(int i = 0; i < skillsCount; ++i)
		{
			object->secSkills[i].first = reader->readSkill();
			object->secSkills[i].second = reader->readInt8Checked(1,3);
		}
	}

	bool hasGarison = reader->readBool();
	if(hasGarison)
		readCreatureSet(object.get(), objectInstanceID);

	object->formation = static_cast<EArmyFormation>(reader->readInt8Checked(0, 1));
	assert(object->formation == EArmyFormation::LOOSE || object->formation == EArmyFormation::TIGHT);

	loadArtifactsOfHero(object.get());
	object->patrol.patrolRadius = reader->readUInt8();
	object->patrol.patrolling = (object->patrol.patrolRadius != 0xff);

	if(features.levelAB)
	{
		bool hasCustomBiography = reader->readBool();
		if(hasCustomBiography)
			object->biographyCustomTextId = readLocalizedString(TextIdentifier("heroes", object->subID, "biography"));

		object->gender = static_cast<EHeroGender>(reader->readInt8Checked(-1, 1));
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
				object->spells.clear();
				logGlobal->debug("Hero %s subID=%d has spells set twice (in map properties and on adventure map instance). Using the latter set...", object->getNameTextID(), object->subID);
			}

			reader->readBitmaskSpells(object->spells, false);
			object->spells.insert(SpellID::PRESET); //placeholder "preset spells"
		}
	}
	else if(features.levelAB)
	{
		//we can read one spell
		SpellID spell = reader->readSpell();

		// workaround: VCMI uses 'PRESET' spell to indicate that spellbook has been preconfigured on map
		// but H3 uses 'PRESET' spell (-2) to indicate that game should give standard spell to hero
		if(spell == SpellID::NONE)
			object->spells.insert(SpellID::PRESET); //spellbook is preconfigured to be empty

		if (spell.hasValue())
			object->spells.insert(spell);
	}

	if(features.levelSOD)
	{
		bool hasCustomPrimSkills = reader->readBool();
		if(hasCustomPrimSkills)
		{
			auto ps = object->getAllBonuses(Selector::type()(BonusType::PRIMARY_SKILL).And(Selector::sourceType()(BonusSource::HERO_BASE_SKILL)), "");
			if(ps->size())
			{
				logGlobal->debug("Hero %s has set primary skills twice (in map properties and on adventure map instance). Using the latter set...", object->getHeroTypeID().getNum() );
				for(const auto & b : *ps)
					object->removeBonus(b);
			}

			for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; ++xx)
			{
				object->pushPrimSkill(static_cast<PrimarySkill>(xx), reader->readUInt8());
			}
		}
	}

	if (object->subID != MapObjectSubID())
		logGlobal->debug("Map '%s': Hero on map: %s at %s, owned by %s", mapName, object->getHeroType()->getJsonKey(), mapPosition.toString(), object->getOwner().toString());
	else
		logGlobal->debug("Map '%s': Hero on map: (random) at %s, owned by %s", mapName, mapPosition.toString(), object->getOwner().toString());

	reader->skipZero(16);

	if(features.levelHOTA5)
	{
		bool alwaysAddSkills = reader->readBool(); // prevent heroes from receiving additional random secondary skills at the start of the map if they are not of the first level
		bool cannotGainXP = reader->readBool();
		int32_t level = reader->readInt32(); // Needs investigation how this interacts with usual setting of level via experience
		assert(level > 0);

		if (!alwaysAddSkills)
			logGlobal->warn("Map '%s': Option to prevent hero %d from gaining skills on map start is not implemented!", mapName, object->subID.num);

		if (cannotGainXP)
			logGlobal->warn("Map '%s': Option to prevent hero %d from receiveing experience is not implemented!", mapName, object->subID.num);

		if (level > 1)
			logGlobal->warn("Map '%s': Option to set level of hero %d to %d is not implemented!", mapName, object->subID.num, level);
	}
	return object;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readSeerHut(const int3 & position, const ObjectInstanceID & idToBeGiven)
{
	auto hut = std::make_shared<CGSeerHut>(map->cb);

	uint32_t questsCount = 1;

	if(features.levelHOTA3)
		questsCount = reader->readUInt32();

	//TODO: HotA
	if(questsCount > 1)
		logGlobal->warn("Map '%s': Seer Hut at %s - %d quests are not implemented!", mapName, position.toString(), questsCount);

	for(size_t i = 0; i < questsCount; ++i)
		readSeerHutQuest(hut.get(), position, idToBeGiven);

	if(features.levelHOTA3)
	{
		uint32_t repeateableQuestsCount = reader->readUInt32();
		hut->getQuest().repeatedQuest = repeateableQuestsCount != 0;

		if(repeateableQuestsCount != 0)
			logGlobal->warn("Map '%s': Seer Hut at %s - %d repeatable quests are not implemented!", mapName, position.toString(), repeateableQuestsCount);

		for(size_t i = 0; i < repeateableQuestsCount; ++i)
			readSeerHutQuest(hut.get(), position, idToBeGiven);
	}

	reader->skipZero(2);

	return hut;
}

enum class ESeerHutRewardType : uint8_t
{
	NOTHING = 0,
	EXPERIENCE = 1,
	MANA_POINTS = 2,
	MORALE = 3,
	LUCK = 4,
	RESOURCES = 5,
	PRIMARY_SKILL = 6,
	SECONDARY_SKILL = 7,
	ARTIFACT = 8,
	SPELL = 9,
	CREATURE = 10,
};

void CMapLoaderH3M::readSeerHutQuest(CGSeerHut * hut, const int3 & position, const ObjectInstanceID & idToBeGiven)
{
	EQuestMission missionType = EQuestMission::NONE;
	if(features.levelAB)
	{
		missionType = readQuest(hut, position);
	}
	else
	{
		//RoE
		auto artID = reader->readArtifact();
		if(artID != ArtifactID::NONE)
		{
			//not none quest
			hut->getQuest().mission.artifacts.push_back(artID);
			missionType = EQuestMission::ARTIFACT;
		}
		hut->getQuest().lastDay = -1; //no timeout
		hut->getQuest().isCustomFirst = false;
		hut->getQuest().isCustomNext = false;
		hut->getQuest().isCustomComplete = false;
	}

	if(missionType != EQuestMission::NONE)
	{
		auto rewardType = static_cast<ESeerHutRewardType>(reader->readInt8Checked(0, 10));
		Rewardable::VisitInfo vinfo;
		auto & reward = vinfo.reward;
		switch(rewardType)
		{
			case ESeerHutRewardType::NOTHING:
			{
				// no-op
				break;
			}
			case ESeerHutRewardType::EXPERIENCE:
			{
				reward.heroExperience = reader->readUInt32();
				break;
			}
			case ESeerHutRewardType::MANA_POINTS:
			{
				reward.manaDiff = reader->readUInt32();
				break;
			}
			case ESeerHutRewardType::MORALE:
			{
				reward.heroBonuses.push_back(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::MORALE, BonusSource::OBJECT_INSTANCE, reader->readInt8Checked(-3, 3), BonusSourceID(idToBeGiven)));
				break;
			}
			case ESeerHutRewardType::LUCK:
			{
				reward.heroBonuses.push_back(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::LUCK, BonusSource::OBJECT_INSTANCE, reader->readInt8Checked(-3, 3), BonusSourceID(idToBeGiven)));
				break;
			}
			case ESeerHutRewardType::RESOURCES:
			{
				auto rId = reader->readGameResID();
				auto rVal = reader->readUInt32();

				reward.resources[rId] = rVal;
				break;
			}
			case ESeerHutRewardType::PRIMARY_SKILL:
			{
				auto rId = reader->readPrimary();
				auto rVal = reader->readUInt8();

				reward.primary.at(rId.getNum()) = rVal;
				break;
			}
			case ESeerHutRewardType::SECONDARY_SKILL:
			{
				auto rId = reader->readSkill();
				auto rVal = reader->readInt8Checked(1,3);

				reward.secondary[rId] = rVal;
				break;
			}
			case ESeerHutRewardType::ARTIFACT:
			{
				ArtifactID grantedArtifact = reader->readArtifact();
				if (features.levelHOTA5)
				{
					SpellID scrollSpell = reader->readSpell16();
					if (grantedArtifact == ArtifactID::SPELL_SCROLL)
						reward.grantedScrolls.push_back(scrollSpell);
					else
						reward.grantedArtifacts.push_back(grantedArtifact);
				}
				else
					reward.grantedArtifacts.push_back(grantedArtifact);
				break;
			}
			case ESeerHutRewardType::SPELL:
			{
				reward.spells.push_back(reader->readSpell());
				break;
			}
			case ESeerHutRewardType::CREATURE:
			{
				auto rId = reader->readCreature();
				auto rVal = reader->readUInt16();

				reward.creatures.emplace_back(rId, rVal);
				break;
			}
			default:
			{
				assert(0);
			}
		}

		vinfo.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		hut->configuration.info.push_back(vinfo);
	}
	else
	{
		// missionType==255
		reader->skipZero(1);
	}
}

EQuestMission CMapLoaderH3M::readQuest(IQuestObject * guard, const int3 & position)
{
	auto missionId = static_cast<EQuestMission>(reader->readInt8Checked(0, 10));

	switch(missionId)
	{
		case EQuestMission::NONE:
			return missionId;
		case EQuestMission::PRIMARY_SKILL:
		{
			for(int x = 0; x < 4; ++x)
			{
				guard->getQuest().mission.primary[x] = reader->readUInt8();
			}
			break;
		}
		case EQuestMission::LEVEL:
		{
			guard->getQuest().mission.heroLevel = reader->readUInt32();
			break;
		}
		case EQuestMission::KILL_HERO:
		case EQuestMission::KILL_CREATURE:
		{
			// NOTE: assert might fail on multi-quest seers
			//assert(questsToResolve.count(guard) == 0);
			questsToResolve[guard] = reader->readUInt32();
			break;
		}
		case EQuestMission::ARTIFACT:
		{
			size_t artNumber = reader->readUInt8();
			for(size_t yy = 0; yy < artNumber; ++yy)
			{
				ArtifactID requiredArtifact = reader->readArtifact();

				if (features.levelHOTA5)
				{
					SpellID scrollSpell = reader->readSpell16();
					if (requiredArtifact == ArtifactID::SPELL_SCROLL)
						guard->getQuest().mission.scrolls.push_back(scrollSpell);
					else
						guard->getQuest().mission.artifacts.push_back(requiredArtifact);
				}
				else
					guard->getQuest().mission.artifacts.push_back(requiredArtifact);

				map->allowedArtifact.erase(requiredArtifact); //these are unavailable for random generation
			}
			break;
		}
		case EQuestMission::ARMY:
		{
			size_t typeNumber = reader->readUInt8();
			guard->getQuest().mission.creatures.resize(typeNumber);
			for(size_t hh = 0; hh < typeNumber; ++hh)
			{
				guard->getQuest().mission.creatures[hh].setType(reader->readCreature().toCreature());
				guard->getQuest().mission.creatures[hh].setCount(reader->readUInt16());
			}
			break;
		}
		case EQuestMission::RESOURCES:
		{
			for(int x = 0; x < 7; ++x)
				guard->getQuest().mission.resources[x] = reader->readUInt32();

			break;
		}
		case EQuestMission::HERO:
		{
			guard->getQuest().mission.heroes.push_back(reader->readHero());
			break;
		}
		case EQuestMission::PLAYER:
		{
			guard->getQuest().mission.players.push_back(reader->readPlayer());
			break;
		}
		case EQuestMission::HOTA_MULTI:
		{
			uint32_t missionSubID = reader->readUInt32();
			assert(missionSubID < 4);

			if(missionSubID == 0)
			{
				missionId = EQuestMission::HOTA_HERO_CLASS;
				std::set<HeroClassID> heroClasses;
				reader->readBitmaskHeroClassesSized(heroClasses, false);
				for(auto & hc : heroClasses)
					guard->getQuest().mission.heroClasses.push_back(hc);
				break;
			}
			if(missionSubID == 1)
			{
				missionId = EQuestMission::HOTA_REACH_DATE;
				guard->getQuest().mission.daysPassed = reader->readUInt32() + 1;
				break;
			}
			if(missionSubID == 2)
			{
				missionId = EQuestMission::HOTA_GAME_DIFFICULTY;
				int32_t difficultyMask = reader->readUInt32();
				assert(difficultyMask > 0 && difficultyMask < 32);
				logGlobal->warn("Map '%s': Seer Hut at %s: Difficulty-specific quest (%d) is not implemented!", mapName, position.toString(), difficultyMask);
				break;
			}
			if(missionSubID == 3)
			{
				missionId = EQuestMission::HOTA_SCRIPTED;
				int32_t scriptID = reader->readUInt32();
				bool unknown = reader->readBool();
				logGlobal->warn("Map '%s': Seer Hut at %s: Scripted quest (%d/%d) is not implemented!", mapName, position.toString(), scriptID, unknown);
				break;
			}
			break;
		}
		default:
		{
			assert(0);
		}
	}

	guard->getQuest().lastDay = reader->readInt32();
	guard->getQuest().firstVisitText.appendTextID(readLocalizedString(TextIdentifier("quest", position.x, position.y, position.z, "firstVisit")));
	guard->getQuest().nextVisitText.appendTextID(readLocalizedString(TextIdentifier("quest", position.x, position.y, position.z, "nextVisit")));
	guard->getQuest().completedText.appendTextID(readLocalizedString(TextIdentifier("quest", position.x, position.y, position.z, "completed")));
	guard->getQuest().isCustomFirst = !guard->getQuest().firstVisitText.empty();
	guard->getQuest().isCustomNext = !guard->getQuest().nextVisitText.empty();
	guard->getQuest().isCustomComplete = !guard->getQuest().completedText.empty();
	return missionId;
}

std::shared_ptr<CGObjectInstance> CMapLoaderH3M::readTown(const int3 & position, std::shared_ptr<const ObjectTemplate> objectTemplate, const ObjectInstanceID & idToBeGiven)
{
	auto object = std::make_shared<CGTownInstance>(map->cb);
	if(features.levelAB)
		object->identifier = reader->readUInt32();

	setOwnerAndValidate(position, object.get(), reader->readPlayer());

	std::optional<FactionID> faction;
	if (objectTemplate->id == Obj::TOWN)
	{
		faction = FactionID(objectTemplate->subid);
		object->subID = objectTemplate->subid;
	}

	bool hasName = reader->readBool();
	if(hasName)
		object->setNameTextId(readLocalizedString(TextIdentifier("town", position.x, position.y, position.z, "name")));

	bool hasGarrison = reader->readBool();
	if(hasGarrison)
		readCreatureSet(object.get(), idToBeGiven);

	object->formation = static_cast<EArmyFormation>(reader->readInt8Checked(0, 1));
	assert(object->formation == EArmyFormation::LOOSE || object->formation == EArmyFormation::TIGHT);

	bool hasCustomBuildings = reader->readBool();
	if(hasCustomBuildings)
	{
		std::set<BuildingID> builtBuildings;
		reader->readBitmaskBuildings(builtBuildings, faction);
		for(const auto & building : builtBuildings)
			object->addBuilding(building);
		reader->readBitmaskBuildings(object->forbiddenBuildings, faction);
	}
	// Standard buildings
	else
	{
		bool hasFort = reader->readBool();
		if(hasFort)
			object->addBuilding(BuildingID::FORT);

		//means that set of standard building should be included
		object->addBuilding(BuildingID::DEFAULT);
	}

	if(features.levelAB)
	{
		std::set<SpellID> spellsMask;

		reader->readBitmaskSpells(spellsMask, false);
		std::copy(spellsMask.begin(), spellsMask.end(), std::back_inserter(object->obligatorySpells));
	}

	{
		std::set<SpellID> spellsMask = LIBRARY->spellh->getDefaultAllowed(); // by default - include spells from mods

		reader->readBitmaskSpells(spellsMask, true);
		std::copy(spellsMask.begin(), spellsMask.end(), std::back_inserter(object->possibleSpells));
	}

	if(features.levelHOTA1)
		object->spellResearchAllowed = reader->readBool();

	if(features.levelHOTA5)
	{
		// Most likely customization of special buildings per faction -> 4x11 table
		// presumably:
		// 0 -> default / allowed
		// 1 -> built?
		// 2 -> banned?
		uint32_t specialBuildingsSize = reader->readUInt32();

		for (int i = 0; i < specialBuildingsSize; ++i)
		{
			int factionID = i / 4;
			int buildingID = i % 4;

			int8_t specialBuildingBuilt = reader->readInt8Checked(0, 2);
			if (specialBuildingBuilt != 0)
				logGlobal->warn("Map '%s': Town at %s: Constructing / banning town-specific special building %d in faction %d on start is not implemented!", mapName, position.toString(), buildingID, factionID);
		}
	}

	// Read castle events
	uint32_t eventsCount = reader->readUInt32();

	for(int eventID = 0; eventID < eventsCount; ++eventID)
	{
		CCastleEvent event;
		event.creatures.resize(7);

		readEventCommon(event, TextIdentifier("town", position.x, position.y, position.z, "event", eventID, "description"));

		if(features.levelHOTA5)
		{
			int32_t creatureGrowth8 = reader->readInt32();

			// always 44
			int32_t hotaAmount = reader->readInt32();

			// contains bitmask on which town-specific buildings to build
			// 4 bits / town, for each of special building in town (special 1 - special 4)
			int32_t hotaSpecialA = reader->readInt32();
			int16_t hotaSpecialB = reader->readInt16();

			if (hotaSpecialA != 0 || hotaSpecialB != 0 || hotaAmount != 44)
				logGlobal->warn("Map '%s': Town at %s: Constructing town-specific special buildings in event is not implemented!", mapName, position.toString());

			event.creatures.push_back(creatureGrowth8);
		}

		if(features.levelHOTA7)
		{
			// TODO: hota
			[[maybe_unused]] bool neutralAffected = reader->readBool();
		}

		// New buildings
		reader->readBitmaskBuildings(event.buildings, faction);

		for(int i = 0; i < 7; ++i)
			event.creatures[i] = reader->readUInt16();

		reader->skipZero(4);
		object->events.push_back(event);
	}

	if(features.levelSOD)
	{
		object->alignmentToPlayer = PlayerColor::NEUTRAL; // "same as owner or random"

		 uint8_t alignment = reader->readUInt8();

		if(alignment != 255)
		{
			if(alignment < PlayerColor::PLAYER_LIMIT.getNum())
			{
				if (mapHeader->players[alignment].canAnyonePlay())
					object->alignmentToPlayer = PlayerColor(alignment);
				else
					logGlobal->warn("Map '%s': Alignment of town at %s is invalid! Player %d is not present on map!", mapName, position.toString(), static_cast<int>(alignment));
			}
			else
			{
				// TODO: HOTA support
				uint8_t invertedAlignment = alignment - PlayerColor::PLAYER_LIMIT.getNum();

				if(invertedAlignment < PlayerColor::PLAYER_LIMIT.getNum())
				{
					logGlobal->warn("Map '%s': Alignment of town at %s 'not as player %d' is not implemented!", mapName, position.toString(), alignment - PlayerColor::PLAYER_LIMIT.getNum());
				}
				else
				{
					logGlobal->warn("Map '%s': Alignment of town at %s is corrupted!!", mapName, position.toString());
				}
			}
		}
	}
	reader->skipZero(3);

	return object;
}

void CMapLoaderH3M::readEventCommon(CMapEvent & event, const TextIdentifier & messageID)
{
	event.name = readBasicString();
	event.message.appendTextID(readLocalizedString(messageID));

	reader->readResources(event.resources);

	reader->readBitmaskPlayers(event.players, false);
	if(features.levelSOD)
		event.humanAffected = reader->readBool();
	else
		event.humanAffected = true;

	event.computerAffected = reader->readBool();
	event.firstOccurrence = reader->readUInt16();
	event.nextOccurrence = reader->readUInt16();

	reader->skipZero(16);

	if (features.levelHOTA7)
	{
		int32_t affectedDifficulties = reader->readInt32();
		assert(affectedDifficulties > 0 && affectedDifficulties < 32);
		event.affectedDifficulties = MapDifficultySet(affectedDifficulties);
	}

	if(features.levelHOTA9)
	{
		bool usesEventSystem = reader->readBool();
		if(usesEventSystem)
		{
			int32_t eventID = reader->readInt32();
			bool syncronizeObjects = reader->readBool();
			logGlobal->warn("Map %s: Extended script system (event ID %d, synchronize %d) for timed/town event is not implemented!", mapName, eventID, syncronizeObjects);
		}
	}
}

void CMapLoaderH3M::readEvents()
{
	uint32_t eventsCount = reader->readUInt32();
	for(int eventID = 0; eventID < eventsCount; ++eventID)
	{
		CMapEvent event;
		readEventCommon(event, TextIdentifier("event", eventID, "description"));

		// garbage bytes that were present in HOTA5 & HOTA6
		if (features.levelHOTA5 && !features.levelHOTA7)
			reader->skipUnused(14);

		map->events.push_back(event);
	}
}

void CMapLoaderH3M::readMessageAndGuards(MetaString & message, CArmedInstance * guards, const int3 & position, const ObjectInstanceID & idToBeGiven)
{
	bool hasMessage = reader->readBool();
	if(hasMessage)
	{
		message.appendTextID(readLocalizedString(TextIdentifier("guards", position.x, position.y, position.z, "message")));
		bool hasGuards = reader->readBool();
		if(hasGuards)
			readCreatureSet(guards, idToBeGiven);

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

	return mapRegisterLocalizedString(modName, *mapHeader, fullIdentifier, mapString);
}

void CMapLoaderH3M::afterRead()
{
	//convert main town positions for all players to actual object position, in H3M it is position of active tile

	for(auto & p : map->players)
	{
		int3 posOfMainTown = p.posOfMainTown;
		if(posOfMainTown.isValid() && map->isInTheMap(posOfMainTown))
		{
			const TerrainTile & t = map->getTile(posOfMainTown);

			const CGObjectInstance * mainTown = nullptr;

			for(ObjectInstanceID objID : t.visitableObjects)
			{
				const CGObjectInstance * object = map->getObject(objID);

				if(object->ID == Obj::TOWN || object->ID == Obj::RANDOM_TOWN)
				{
					mainTown = object;
					break;
				}
			}

			if(mainTown == nullptr)
				continue;

			p.posOfMainTown = posOfMainTown + mainTown->getVisitableOffset();
		}
	}

	for (auto & quest : questsToResolve)
		quest.first->getQuest().killTarget = questIdentifierToId.at(quest.second);
}

VCMI_LIB_NAMESPACE_END
