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
#include <boost/crc.hpp>

#include "MapFormatH3M.h"
#include "CMap.h"

#include "../CStopWatch.h"
#include "../filesystem/Filesystem.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "../CCreatureHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CHeroHandler.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/MapObjects.h"
#include "../VCMI_Lib.h"
#include "../TerrainHandler.h"
#include "../RoadHandler.h"
#include "../RiverHandler.h"
#include "../NetPacksBase.h"

VCMI_LIB_NAMESPACE_BEGIN


const bool CMapLoaderH3M::IS_PROFILING_ENABLED = false;

CMapLoaderH3M::CMapLoaderH3M(CInputStream * stream) : map(nullptr), reader(stream),inputStream(stream)
{
}

CMapLoaderH3M::~CMapLoaderH3M()
{
}

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
	//FIXME: get rid of double input process
	si64 temp_size = inputStream->getSize();
	inputStream->seek(0);

	auto  temp_buffer = new ui8[temp_size];
	inputStream->read(temp_buffer,temp_size);

	// Compute checksum
	boost::crc_32_type  result;
	result.process_bytes(temp_buffer, temp_size);
	map->checksum = result.checksum();

	delete [] temp_buffer;
	inputStream->seek(0);

	CStopWatch sw;

	struct MapLoadingTime
	{
		std::string name;
		si64 time;

		MapLoadingTime(std::string name, si64 time) : name(name),
			time(time)
		{

		}
	};
	std::vector<MapLoadingTime> times;

	readHeader();
	times.push_back(MapLoadingTime("header", sw.getDiff()));

	map->allHeroes.resize(map->allowedHeroes.size());

	readDisposedHeroes();
	times.push_back(MapLoadingTime("disposed heroes", sw.getDiff()));

	readAllowedArtifacts();
	times.push_back(MapLoadingTime("allowed artifacts", sw.getDiff()));

	readAllowedSpellsAbilities();
	times.push_back(MapLoadingTime("allowed spells and abilities", sw.getDiff()));

	readRumors();
	times.push_back(MapLoadingTime("rumors", sw.getDiff()));

	readPredefinedHeroes();
	times.push_back(MapLoadingTime("predefined heroes", sw.getDiff()));

	readTerrain();
	times.push_back(MapLoadingTime("terrain", sw.getDiff()));

	readDefInfo();
	times.push_back(MapLoadingTime("def info", sw.getDiff()));

	readObjects();
	times.push_back(MapLoadingTime("objects", sw.getDiff()));

	readEvents();
	times.push_back(MapLoadingTime("events", sw.getDiff()));

	times.push_back(MapLoadingTime("blocked/visitable tiles", sw.getDiff()));

	// Print profiling times
	if(IS_PROFILING_ENABLED)
	{
		for(MapLoadingTime & mlt : times)
		{
			logGlobal->debug("\tReading %s took %d ms", mlt.name, mlt.time);
		}
	}
	map->calculateGuardingGreaturePositions();
	afterRead();
}

void CMapLoaderH3M::readHeader()
{
	// Check map for validity
	// Note: disabled, causes decompression of the entire file ( = SLOW)
	//if(inputStream->getSize() < 50)
	//{
	//	throw std::runtime_error("Corrupted map file.");
	//}

	// Map version
	mapHeader->version = (EMapFormat::EMapFormat)(reader.readUInt32());
	if(mapHeader->version != EMapFormat::ROE && mapHeader->version != EMapFormat::AB && mapHeader->version != EMapFormat::SOD
			&& mapHeader->version != EMapFormat::WOG)
	{
		throw std::runtime_error("Invalid map format!");
	}

	// Read map name, description, dimensions,...
	mapHeader->areAnyPlayers = reader.readBool();
	mapHeader->height = mapHeader->width = reader.readUInt32();
	mapHeader->twoLevel = reader.readBool();
	mapHeader->name = reader.readString();
	mapHeader->description = reader.readString();
	mapHeader->difficulty = reader.readInt8();
	if(mapHeader->version != EMapFormat::ROE)
	{
		mapHeader->levelLimit = reader.readUInt8();
	}
	else
	{
		mapHeader->levelLimit = 0;
	}

	readPlayerInfo();
	readVictoryLossConditions();
	readTeamInfo();
	readAllowedHeroes();
}

void CMapLoaderH3M::readPlayerInfo()
{
	for(int i = 0; i < mapHeader->players.size(); ++i)
	{
		mapHeader->players[i].canHumanPlay = reader.readBool();
		mapHeader->players[i].canComputerPlay = reader.readBool();

		// If nobody can play with this player
		if((!(mapHeader->players[i].canHumanPlay || mapHeader->players[i].canComputerPlay)))
		{
			switch(mapHeader->version)
			{
			case EMapFormat::SOD:
			case EMapFormat::WOG:
				reader.skip(13);
				break;
			case EMapFormat::AB:
				reader.skip(12);
				break;
			case EMapFormat::ROE:
				reader.skip(6);
				break;
			}
			continue;
		}

		mapHeader->players[i].aiTactic = static_cast<EAiTactic::EAiTactic>(reader.readUInt8());

		if(mapHeader->version == EMapFormat::SOD || mapHeader->version == EMapFormat::WOG)
		{
			mapHeader->players[i].p7 = reader.readUInt8();
		}
		else
		{
			mapHeader->players[i].p7 = -1;
		}

		// Factions this player can choose
		ui16 allowedFactions = reader.readUInt8();
		// How many factions will be read from map
		ui16 totalFactions = GameConstants::F_NUMBER;

		if(mapHeader->version != EMapFormat::ROE)
			allowedFactions += reader.readUInt8() * 256; // 256 = 2^8 = 0b100000000
		else
			totalFactions--; //exclude conflux for ROE

		const bool isFactionRandom = mapHeader->players[i].isFactionRandom = reader.readBool();
		const ui16 allFactionsMask = (mapHeader->version == EMapFormat::ROE)
			? 0b11111111   // 8 towns for ROE
			: 0b111111111; // 8 towns + Conflux
		const bool allFactionsAllowed = mapHeader->version == EMapFormat::VCMI
			|| (isFactionRandom && ((allowedFactions & allFactionsMask) == allFactionsMask));

		if(!allFactionsAllowed)
		{
			mapHeader->players[i].allowedFactions.clear();

			for(int fact = 0; fact < totalFactions; ++fact)
			{
				if(allowedFactions & (1 << fact))
					mapHeader->players[i].allowedFactions.insert(fact);
			}
		}

		mapHeader->players[i].hasMainTown = reader.readBool();
		if(mapHeader->players[i].hasMainTown)
		{
			if(mapHeader->version != EMapFormat::ROE)
			{
				mapHeader->players[i].generateHeroAtMainTown = reader.readBool();
				mapHeader->players[i].generateHero = reader.readBool();
			}
			else
			{
				mapHeader->players[i].generateHeroAtMainTown = true;
				mapHeader->players[i].generateHero = false;
			}

			mapHeader->players[i].posOfMainTown = readInt3();
		}

		mapHeader->players[i].hasRandomHero = reader.readBool();
		mapHeader->players[i].mainCustomHeroId = reader.readUInt8();

		if(mapHeader->players[i].mainCustomHeroId != 0xff)
		{
			mapHeader->players[i].mainCustomHeroPortrait = reader.readUInt8();
			if (mapHeader->players[i].mainCustomHeroPortrait == 0xff)
				mapHeader->players[i].mainCustomHeroPortrait = -1; //correct 1-byte -1 (0xff) into 4-byte -1

			mapHeader->players[i].mainCustomHeroName = reader.readString();
		}
		else
			mapHeader->players[i].mainCustomHeroId = -1; //correct 1-byte -1 (0xff) into 4-byte -1

		if(mapHeader->version != EMapFormat::ROE)
		{
			mapHeader->players[i].powerPlaceholders = reader.readUInt8(); //unknown byte
			int heroCount = reader.readUInt8();
			reader.skip(3);
			for(int pp = 0; pp < heroCount; ++pp)
			{
				SHeroName vv;
				vv.heroId = reader.readUInt8();
				vv.heroName = reader.readString();

				mapHeader->players[i].heroesNames.push_back(vv);
			}
		}
	}
}

namespace EVictoryConditionType
{
	enum EVictoryConditionType { ARTIFACT, GATHERTROOP, GATHERRESOURCE, BUILDCITY, BUILDGRAIL, BEATHERO,
		CAPTURECITY, BEATMONSTER, TAKEDWELLINGS, TAKEMINES, TRANSPORTITEM, WINSTANDARD = 255 };
}

namespace ELossConditionType
{
	enum ELossConditionType { LOSSCASTLE, LOSSHERO, TIMEEXPIRES, LOSSSTANDARD = 255 };
}

void CMapLoaderH3M::readVictoryLossConditions()
{
	mapHeader->triggeredEvents.clear();

	auto vicCondition = (EVictoryConditionType::EVictoryConditionType)reader.readUInt8();

	EventCondition victoryCondition(EventCondition::STANDARD_WIN);
	EventCondition defeatCondition(EventCondition::DAYS_WITHOUT_TOWN);
	defeatCondition.value = 7;

	TriggeredEvent standardVictory;
	standardVictory.effect.type = EventEffect::VICTORY;
	standardVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[5];
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill = VLC->generaltexth->allTexts[659];
	standardVictory.trigger = EventExpression(victoryCondition);

	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage = VLC->generaltexth->allTexts[8];
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill = VLC->generaltexth->allTexts[7];
	standardDefeat.trigger = EventExpression(defeatCondition);

	// Specific victory conditions
	if(vicCondition == EVictoryConditionType::WINSTANDARD)
	{
		// create normal condition
		mapHeader->triggeredEvents.push_back(standardVictory);
		mapHeader->victoryIconIndex = 11;
		mapHeader->victoryMessage = VLC->generaltexth->victoryConditions[0];
	}
	else
	{
		TriggeredEvent specialVictory;
		specialVictory.effect.type = EventEffect::VICTORY;
		specialVictory.identifier = "specialVictory";
		specialVictory.description.clear(); // TODO: display in quest window

		mapHeader->victoryIconIndex = ui16(vicCondition);
		mapHeader->victoryMessage = VLC->generaltexth->victoryConditions[size_t(vicCondition) + 1];

		bool allowNormalVictory = reader.readBool();
		bool appliesToAI = reader.readBool();

		if (allowNormalVictory)
		{
			size_t playersOnMap = boost::range::count_if(mapHeader->players,[](const PlayerInfo & info) { return info.canAnyonePlay();});

			if (playersOnMap == 1)
			{
				logGlobal->warn("Map %s has only one player but allows normal victory?", mapHeader->name);
				allowNormalVictory = false; // makes sense? Not much. Works as H3? Yes!
			}
		}

		switch(vicCondition)
		{
		case EVictoryConditionType::ARTIFACT:
			{
				EventCondition cond(EventCondition::HAVE_ARTIFACT);
				cond.objectType = reader.readUInt8();
				if (mapHeader->version != EMapFormat::ROE)
					reader.skip(1);

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[281];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[280];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::GATHERTROOP:
			{
				EventCondition cond(EventCondition::HAVE_CREATURES);
				cond.objectType = reader.readUInt8();
				if (mapHeader->version != EMapFormat::ROE)
					reader.skip(1);
				cond.value = reader.readUInt32();

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[277];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[276];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::GATHERRESOURCE:
			{
				EventCondition cond(EventCondition::HAVE_RESOURCES);
				cond.objectType = reader.readUInt8();
				cond.value = reader.readUInt32();

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[279];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[278];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::BUILDCITY:
			{
				EventExpression::OperatorAll oper;
				EventCondition cond(EventCondition::HAVE_BUILDING);
				cond.position = readInt3();
				cond.objectType = BuildingID::TOWN_HALL + reader.readUInt8();
				oper.expressions.push_back(cond);
				cond.objectType = BuildingID::FORT + reader.readUInt8();
				oper.expressions.push_back(cond);

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[283];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[282];
				specialVictory.trigger = EventExpression(oper);
				break;
			}
		case EVictoryConditionType::BUILDGRAIL:
			{
				EventCondition cond(EventCondition::HAVE_BUILDING);
				cond.objectType = BuildingID::GRAIL;
				cond.position = readInt3();
				if(cond.position.z > 2)
					cond.position = int3(-1,-1,-1);

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[285];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[284];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::BEATHERO:
			{
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = Obj::HERO;
				cond.position = readInt3();

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[253];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[252];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::CAPTURECITY:
			{
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::TOWN;
				cond.position = readInt3();

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[250];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[249];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::BEATMONSTER:
			{
				EventCondition cond(EventCondition::DESTROY);
				cond.objectType = Obj::MONSTER;
				cond.position = readInt3();

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[287];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[286];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::TAKEDWELLINGS:
			{
				EventExpression::OperatorAll oper;
				oper.expressions.push_back(EventCondition(EventCondition::CONTROL, 0, Obj::CREATURE_GENERATOR1));
				oper.expressions.push_back(EventCondition(EventCondition::CONTROL, 0, Obj::CREATURE_GENERATOR4));

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[289];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[288];
				specialVictory.trigger = EventExpression(oper);
				break;
			}
		case EVictoryConditionType::TAKEMINES:
			{
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::MINE;

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[291];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[290];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		case EVictoryConditionType::TRANSPORTITEM:
			{
				EventCondition cond(EventCondition::TRANSPORT);
				cond.objectType = reader.readUInt8();
				cond.position = readInt3();

				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[293];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[292];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
		default:
			assert(0);
		}

		// if condition is human-only turn it into following construction: AllOf(human, condition)
		if (!appliesToAI)
		{
			EventExpression::OperatorAll oper;
			EventCondition notAI(EventCondition::IS_HUMAN);
			notAI.value = 1;
			oper.expressions.push_back(notAI);
			oper.expressions.push_back(specialVictory.trigger.get());
			specialVictory.trigger = EventExpression(oper);
		}

		// if normal victory allowed - add one more quest
		if (allowNormalVictory)
		{
			mapHeader->victoryMessage += " / ";
			mapHeader->victoryMessage += VLC->generaltexth->victoryConditions[0];
			mapHeader->triggeredEvents.push_back(standardVictory);
		}
		mapHeader->triggeredEvents.push_back(specialVictory);
	}

	// Read loss conditions
	auto lossCond = (ELossConditionType::ELossConditionType)reader.readUInt8();
	if (lossCond == ELossConditionType::LOSSSTANDARD)
	{
		mapHeader->defeatIconIndex = 3;
		mapHeader->defeatMessage = VLC->generaltexth->lossCondtions[0];
	}
	else
	{
		TriggeredEvent specialDefeat;
		specialDefeat.effect.type = EventEffect::DEFEAT;
		specialDefeat.effect.toOtherMessage = VLC->generaltexth->allTexts[5];
		specialDefeat.identifier = "specialDefeat";
		specialDefeat.description.clear(); // TODO: display in quest window

		mapHeader->defeatIconIndex = ui16(lossCond);
		mapHeader->defeatMessage = VLC->generaltexth->lossCondtions[size_t(lossCond) + 1];

		switch(lossCond)
		{
		case ELossConditionType::LOSSCASTLE:
			{
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::TOWN;
				cond.position = readInt3();

				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill = VLC->generaltexth->allTexts[251];
				specialDefeat.trigger = EventExpression(noneOf);
				break;
			}
		case ELossConditionType::LOSSHERO:
			{
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::HERO;
				cond.position = readInt3();

				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill = VLC->generaltexth->allTexts[253];
				specialDefeat.trigger = EventExpression(noneOf);
				break;
			}
		case ELossConditionType::TIMEEXPIRES:
			{
				EventCondition cond(EventCondition::DAYS_PASSED);
				cond.value = reader.readUInt16();

				specialDefeat.onFulfill = VLC->generaltexth->allTexts[254];
				specialDefeat.trigger = EventExpression(cond);
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

		allOf.expressions.push_back(isHuman);
		allOf.expressions.push_back(specialDefeat.trigger.get());
		specialDefeat.trigger = EventExpression(allOf);

		mapHeader->triggeredEvents.push_back(specialDefeat);
	}
	mapHeader->triggeredEvents.push_back(standardDefeat);
}

void CMapLoaderH3M::readTeamInfo()
{
	mapHeader->howManyTeams = reader.readUInt8();
	if(mapHeader->howManyTeams > 0)
	{
		// Teams
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		{
			mapHeader->players[i].team = TeamID(reader.readUInt8());
		}
	}
	else
	{
		// No alliances
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
		{
			if(mapHeader->players[i].canComputerPlay || mapHeader->players[i].canHumanPlay)
			{
				mapHeader->players[i].team = TeamID(mapHeader->howManyTeams++);
			}
		}
	}
}

void CMapLoaderH3M::readAllowedHeroes()
{
	mapHeader->allowedHeroes.resize(VLC->heroh->size(), true);

	const int bytes = mapHeader->version == EMapFormat::ROE ? 16 : 20;

	readBitmask(mapHeader->allowedHeroes,bytes,GameConstants::HEROES_QUANTITY, false);

	// Probably reserved for further heroes
	if(mapHeader->version > EMapFormat::ROE)
	{
		int placeholdersQty = reader.readUInt32();

		reader.skip(placeholdersQty * 1);

//		std::vector<ui16> placeholdedHeroes;
//
//		for(int p = 0; p < placeholdersQty; ++p)
//		{
//			placeholdedHeroes.push_back(reader.readUInt8());
//		}
	}
}

void CMapLoaderH3M::readDisposedHeroes()
{
	// Reading disposed heroes (20 bytes)
	if(map->version >= EMapFormat::SOD)
	{
		ui8 disp = reader.readUInt8();
		map->disposedHeroes.resize(disp);
		for(int g = 0; g < disp; ++g)
		{
			map->disposedHeroes[g].heroId = reader.readUInt8();
			map->disposedHeroes[g].portrait = reader.readUInt8();
			map->disposedHeroes[g].name = reader.readString();
			map->disposedHeroes[g].players = reader.readUInt8();
		}
	}

	//omitting NULLS
	reader.skip(31);
}

void CMapLoaderH3M::readAllowedArtifacts()
{
	map->allowedArtifact.resize (VLC->arth->objects.size(),true); //handle new artifacts, make them allowed by default

	// Reading allowed artifacts:  17 or 18 bytes
	if(map->version != EMapFormat::ROE)
	{
		const int bytes = map->version == EMapFormat::AB ? 17 : 18;

		readBitmask(map->allowedArtifact,bytes,GameConstants::ARTIFACTS_QUANTITY);

	}

	// ban combo artifacts
	if (map->version == EMapFormat::ROE || map->version == EMapFormat::AB)
	{
		for(CArtifact * artifact : VLC->arth->objects)
		{
			// combo
			if (artifact->constituents)
			{
				map->allowedArtifact[artifact->id] = false;
			}
		}
		if (map->version == EMapFormat::ROE)
		{
			map->allowedArtifact[ArtifactID::ARMAGEDDONS_BLADE] = false;
		}
	}

	// Messy, but needed
	for (TriggeredEvent & event : map->triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			if (cond.condition == EventCondition::HAVE_ARTIFACT ||
				cond.condition == EventCondition::TRANSPORT)
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
	// Read allowed spells, including new ones
	map->allowedSpell.resize(VLC->spellh->objects.size(), true);

	// Read allowed abilities
	map->allowedAbilities.resize(GameConstants::SKILL_QUANTITY, true);

	if(map->version >= EMapFormat::SOD)
	{
		// Reading allowed spells (9 bytes)
		const int spell_bytes = 9;
		readBitmask(map->allowedSpell, spell_bytes, GameConstants::SPELLS_QUANTITY);

		// Allowed hero's abilities (4 bytes)
		const int abil_bytes = 4;
		readBitmask(map->allowedAbilities, abil_bytes, GameConstants::SKILL_QUANTITY);
	}

	//do not generate special abilities and spells
	for (auto spell : VLC->spellh->objects)
		if (spell->isSpecial() || spell->isCreatureAbility())
			map->allowedSpell[spell->id] = false;
}

void CMapLoaderH3M::readRumors()
{
	int rumNr = reader.readUInt32();

	for(int it = 0; it < rumNr; it++)
	{
		Rumor ourRumor;
		ourRumor.name = reader.readString();
		ourRumor.text = reader.readString();
		map->rumors.push_back(ourRumor);
	}
}

void CMapLoaderH3M::readPredefinedHeroes()
{
	switch(map->version)
	{
	case EMapFormat::WOG:
	case EMapFormat::SOD:
		{
			// Disposed heroes
			for(int z = 0; z < GameConstants::HEROES_QUANTITY; z++)
			{
				int custom =  reader.readUInt8();
				if(!custom) continue;

				auto  hero = new CGHeroInstance();
				hero->ID = Obj::HERO;
				hero->subID = z;

				bool hasExp = reader.readBool();
				if(hasExp)
				{
					hero->exp = reader.readUInt32();
				}
				else
				{
					hero->exp = 0;
				}

				bool hasSecSkills = reader.readBool();
				if(hasSecSkills)
				{
					int howMany = reader.readUInt32();
					hero->secSkills.resize(howMany);
					for(int yy = 0; yy < howMany; ++yy)
					{
						hero->secSkills[yy].first = SecondarySkill(reader.readUInt8());
						hero->secSkills[yy].second = reader.readUInt8();
					}
				}

				loadArtifactsOfHero(hero);

				bool hasCustomBio = reader.readBool();
				if(hasCustomBio)
				{
					hero->biography = reader.readString();
				}

				// 0xFF is default, 00 male, 01 female
				hero->sex = reader.readUInt8();

				bool hasCustomSpells = reader.readBool();
				if(hasCustomSpells)
				{
					readSpells(hero->spells);
				}

				bool hasCustomPrimSkills = reader.readBool();
				if(hasCustomPrimSkills)
				{
					for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; xx++)
					{
						hero->pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(xx), reader.readUInt8());
					}
				}
				map->predefinedHeroes.push_back(hero);
			}
			break;
		}
	case EMapFormat::ROE:
		break;
	}
}

void CMapLoaderH3M::loadArtifactsOfHero(CGHeroInstance * hero)
{
	bool artSet = reader.readBool();

	// True if artifact set is not default (hero has some artifacts)
	if(artSet)
	{
		if(hero->artifactsWorn.size() ||  hero->artifactsInBackpack.size())
		{
			logGlobal->warn("Hero %s at %s has set artifacts twice (in map properties and on adventure map instance). Using the latter set...", hero->name, hero->pos.toString());
			hero->artifactsInBackpack.clear();
			while(hero->artifactsWorn.size())
				hero->eraseArtSlot(hero->artifactsWorn.begin()->first);
		}

		for(int pom = 0; pom < 16; pom++)
		{
			loadArtifactToSlot(hero, pom);
		}

		// misc5 art //17
		if(map->version >= EMapFormat::SOD)
		{
			assert(!hero->getArt(ArtifactPosition::MACH4));
			if(!loadArtifactToSlot(hero, ArtifactPosition::MACH4))
			{
				// catapult by default
				assert(!hero->getArt(ArtifactPosition::MACH4));
				hero->putArtifact(ArtifactPosition::MACH4, CArtifactInstance::createArtifact(map, ArtifactID::CATAPULT));
			}
		}

		loadArtifactToSlot(hero, ArtifactPosition::SPELLBOOK);

		// 19 //???what is that? gap in file or what? - it's probably fifth slot..
		if(map->version > EMapFormat::ROE)
		{
			loadArtifactToSlot(hero, ArtifactPosition::MISC5);
		}
		else
		{
			reader.skip(1);
		}

		// bag artifacts //20
		// number of artifacts in hero's bag
		int amount = reader.readUInt16();
		for(int ss = 0; ss < amount; ++ss)
		{
			loadArtifactToSlot(hero, GameConstants::BACKPACK_START + (int)hero->artifactsInBackpack.size());
		}
	}
}

bool CMapLoaderH3M::loadArtifactToSlot(CGHeroInstance * hero, int slot)
{
	const int artmask = map->version == EMapFormat::ROE ? 0xff : 0xffff;
	ArtifactID aid;

	if(map->version == EMapFormat::ROE)
	{
		aid = ArtifactID(reader.readUInt8());
	}
	else
	{
		aid = ArtifactID(reader.readUInt16());
	}

	bool isArt  =  aid != artmask;
	if(isArt)
	{
		const Artifact * art = ArtifactID(aid).toArtifact(VLC->artifacts());

		if(nullptr == art)
		{
			logGlobal->warn("Invalid artifact in hero's backpack, ignoring...");
			return false;
		}

		if(art->isBig() && slot >= GameConstants::BACKPACK_START)
		{
			logGlobal->warn("A big artifact (war machine) in hero's backpack, ignoring...");
			return false;
		}
		if(aid == 0 && slot == ArtifactPosition::MISC5)
		{
			//TODO: check how H3 handles it -> art 0 in slot 18 in AB map
			logGlobal->warn("Spellbook to MISC5 slot? Putting it spellbook place. AB format peculiarity? (format %d)", static_cast<int>(map->version));
			slot = ArtifactPosition::SPELLBOOK;
		}

		// this is needed, because some H3M maps (last scenario of ROE map) contain invalid data like misplaced artifacts
		auto artifact =  CArtifactInstance::createArtifact(map, aid);
		auto artifactPos = ArtifactPosition(slot);
		if (artifact->canBePutAt(ArtifactLocation(hero, artifactPos)))
		{
			hero->putArtifact(artifactPos, artifact);
		}
		else
		{
			logGlobal->debug("Artifact can't be put at the specified location."); //TODO add more debugging information
		}
	}

	return isArt;
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
				tile.terType = const_cast<TerrainType*>(VLC->terrainTypeHandler->getByIndex(reader.readUInt8()));
				tile.terView = reader.readUInt8();
				tile.riverType = const_cast<RiverType*>(VLC->riverTypeHandler->getByIndex(reader.readUInt8()));
				tile.riverDir = reader.readUInt8();
				tile.roadType = const_cast<RoadType*>(VLC->roadTypeHandler->getByIndex(reader.readUInt8()));
				tile.roadDir = reader.readUInt8();
				tile.extTileFlags = reader.readUInt8();
				tile.blocked = !tile.terType->isPassable();
				tile.visitable = 0;

				assert(tile.terType->getId() != ETerrainId::NONE);
			}
		}
	}
}

void CMapLoaderH3M::readDefInfo()
{
	int defAmount = reader.readUInt32();

	templates.reserve(defAmount);

	// Read custom defs
	for(int idd = 0; idd < defAmount; ++idd)
	{
		auto tmpl = new ObjectTemplate;
		tmpl->readMap(reader);
		templates.push_back(std::shared_ptr<const ObjectTemplate>(tmpl));
	}
}

void CMapLoaderH3M::readObjects()
{
	int howManyObjs = reader.readUInt32();

	for(int ww = 0; ww < howManyObjs; ++ww)
	{
		CGObjectInstance * nobj = nullptr;

		int3 objPos = readInt3();

		int defnum = reader.readUInt32();
		ObjectInstanceID idToBeGiven = ObjectInstanceID((si32)map->objects.size());

		std::shared_ptr<const ObjectTemplate> objTempl = templates.at(defnum);
		reader.skip(5);

		switch(objTempl->id)
		{
		case Obj::EVENT:
			{
				auto  evnt = new CGEvent();
				nobj = evnt;

				readMessageAndGuards(evnt->message, evnt);

				evnt->gainedExp = reader.readUInt32();
				evnt->manaDiff = reader.readUInt32();
				evnt->moraleDiff = reader.readInt8();
				evnt->luckDiff = reader.readInt8();

				readResourses(evnt->resources);

				evnt->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x = 0; x < 4; ++x)
				{
					evnt->primskills[x] = static_cast<PrimarySkill::PrimarySkill>(reader.readUInt8());
				}

				int gabn = reader.readUInt8(); // Number of gained abilities
				for(int oo = 0; oo < gabn; ++oo)
				{
					evnt->abilities.push_back(SecondarySkill(reader.readUInt8()));
					evnt->abilityLevels.push_back(reader.readUInt8());
				}

				int gart = reader.readUInt8(); // Number of gained artifacts
				for(int oo = 0; oo < gart; ++oo)
				{
					if(map->version == EMapFormat::ROE)
					{
						evnt->artifacts.push_back(ArtifactID(reader.readUInt8()));
					}
					else
					{
						evnt->artifacts.push_back(ArtifactID(reader.readUInt16()));
					}
				}

				int gspel = reader.readUInt8(); // Number of gained spells
				for(int oo = 0; oo < gspel; ++oo)
				{
					evnt->spells.push_back(SpellID(reader.readUInt8()));
				}

				int gcre = reader.readUInt8(); //number of gained creatures
				readCreatureSet(&evnt->creatures, gcre);

				reader.skip(8);
				evnt->availableFor = reader.readUInt8();
				evnt->computerActivate = reader.readUInt8();
				evnt->removeAfterVisit = reader.readUInt8();
				evnt->humanActivate = true;

				reader.skip(4);
				break;
			}
		case Obj::HERO:
		case Obj::RANDOM_HERO:
		case Obj::PRISON:
			{
				nobj = readHero(idToBeGiven, objPos);
				break;
			}
		case Obj::MONSTER:  //Monster
		case Obj::RANDOM_MONSTER:
		case Obj::RANDOM_MONSTER_L1:
		case Obj::RANDOM_MONSTER_L2:
		case Obj::RANDOM_MONSTER_L3:
		case Obj::RANDOM_MONSTER_L4:
		case Obj::RANDOM_MONSTER_L5:
		case Obj::RANDOM_MONSTER_L6:
		case Obj::RANDOM_MONSTER_L7:
			{
				auto  cre = new CGCreature();
				nobj = cre;

				if(map->version > EMapFormat::ROE)
				{
					cre->identifier = reader.readUInt32();
					map->questIdentifierToId[cre->identifier] = idToBeGiven;
				}

				auto  hlp = new CStackInstance();
				hlp->count = reader.readUInt16();

				//type will be set during initialization
				cre->putStack(SlotID(0), hlp);

				cre->character = reader.readUInt8();

				bool hasMessage = reader.readBool();
				if(hasMessage)
				{
					cre->message = reader.readString();
					readResourses(cre->resources);

					int artID;
					if (map->version == EMapFormat::ROE)
					{
						artID = reader.readUInt8();
					}
					else
					{
						artID = reader.readUInt16();
					}

					if(map->version == EMapFormat::ROE)
					{
						if(artID != 0xff)
						{
							cre->gainedArtifact = ArtifactID(artID);
						}
						else
						{
							cre->gainedArtifact = ArtifactID::NONE;
						}
					}
					else
					{
						if(artID != 0xffff)
						{
							cre->gainedArtifact = ArtifactID(artID);
						}
						else
						{
							cre->gainedArtifact = ArtifactID::NONE;
						}
					}
				}
				cre->neverFlees = reader.readUInt8();
				cre->notGrowingTeam =reader.readUInt8();
				reader.skip(2);
				break;
			}
		case Obj::OCEAN_BOTTLE:
		case Obj::SIGN:
			{
				auto  sb = new CGSignBottle();
				nobj = sb;
				sb->message = reader.readString();
				reader.skip(4);
				break;
			}
		case Obj::SEER_HUT:
			{
				nobj = readSeerHut();
				break;
			}
		case Obj::WITCH_HUT:
			{
				auto  wh = new CGWitchHut();
				nobj = wh;

				// in RoE we cannot specify it - all are allowed (I hope)
				if(map->version > EMapFormat::ROE)
				{
					for(int i = 0 ; i < 4; ++i)
					{
						ui8 c = reader.readUInt8();
						for(int yy = 0; yy < 8; ++yy)
						{
							if(i * 8 + yy < GameConstants::SKILL_QUANTITY)
							{
								if(c == (c | static_cast<ui8>(std::pow(2., yy))))
								{
									wh->allowedAbilities.push_back(i * 8 + yy);
								}
							}
						}
					}
					// enable new (modded) skills
					if(wh->allowedAbilities.size() != 1)
					{
						for(int skillID = GameConstants::SKILL_QUANTITY; skillID < VLC->skillh->size(); ++skillID)
							wh->allowedAbilities.push_back(skillID);
					}
				}
				else
				{
					// RoE map
					for(int skillID = 0; skillID < VLC->skillh->size(); ++skillID)
						wh->allowedAbilities.push_back(skillID);
				}
				break;
			}
		case Obj::SCHOLAR:
			{
				auto  sch = new CGScholar();
				nobj = sch;
				sch->bonusType = static_cast<CGScholar::EBonusType>(reader.readUInt8());
				sch->bonusID = reader.readUInt8();
				reader.skip(6);
				break;
			}
		case Obj::GARRISON:
		case Obj::GARRISON2:
			{
				auto  gar = new CGGarrison();
				nobj = gar;
				nobj->setOwner(PlayerColor(reader.readUInt8()));
				reader.skip(3);
				readCreatureSet(gar, 7);
				if(map->version > EMapFormat::ROE)
				{
					gar->removableUnits = reader.readBool();
				}
				else
				{
					gar->removableUnits = true;
				}
				reader.skip(8);
				break;
			}
		case Obj::ARTIFACT:
		case Obj::RANDOM_ART:
		case Obj::RANDOM_TREASURE_ART:
		case Obj::RANDOM_MINOR_ART:
		case Obj::RANDOM_MAJOR_ART:
		case Obj::RANDOM_RELIC_ART:
		case Obj::SPELL_SCROLL:
			{
				auto artID = ArtifactID::NONE; //random, set later
				int spellID = -1;
				auto  art = new CGArtifact();
				nobj = art;

				readMessageAndGuards(art->message, art);

				if(objTempl->id == Obj::SPELL_SCROLL)
				{
					spellID = reader.readUInt32();
					artID = ArtifactID::SPELL_SCROLL;
				}
				else if(objTempl->id == Obj::ARTIFACT)
				{
					//specific artifact
					artID = ArtifactID(objTempl->subid);
				}

				art->storedArtifact = CArtifactInstance::createArtifact(map, artID, spellID);
				break;
			}
		case Obj::RANDOM_RESOURCE:
		case Obj::RESOURCE:
			{
				auto  res = new CGResource();
				nobj = res;

				readMessageAndGuards(res->message, res);

				res->amount = reader.readUInt32();
				if(objTempl->subid == Res::GOLD)
				{
					// Gold is multiplied by 100.
					res->amount *= 100;
				}
				reader.skip(4);
				break;
			}
		case Obj::RANDOM_TOWN:
		case Obj::TOWN:
			{
				nobj = readTown(objTempl->subid);
				break;
			}
		case Obj::MINE:
		case Obj::ABANDONED_MINE:
			{
				nobj = new CGMine();
				nobj->setOwner(PlayerColor(reader.readUInt8()));
				reader.skip(3);
				break;
			}
		case Obj::CREATURE_GENERATOR1:
		case Obj::CREATURE_GENERATOR2:
		case Obj::CREATURE_GENERATOR3:
		case Obj::CREATURE_GENERATOR4:
			{
				nobj = new CGDwelling();
				nobj->setOwner(PlayerColor(reader.readUInt8()));
				reader.skip(3);
				break;
			}
		case Obj::SHRINE_OF_MAGIC_INCANTATION:
		case Obj::SHRINE_OF_MAGIC_GESTURE:
		case Obj::SHRINE_OF_MAGIC_THOUGHT:
			{
				auto  shr = new CGShrine();
				nobj = shr;
				ui8 raw_id = reader.readUInt8();

				if (255 == raw_id)
				{
					shr->spell = SpellID(SpellID::NONE);
				}
				else
				{
					shr->spell = SpellID(raw_id);
				}

				reader.skip(3);
				break;
			}
		case Obj::PANDORAS_BOX:
			{
				auto  box = new CGPandoraBox();
				nobj = box;
				readMessageAndGuards(box->message, box);

				box->gainedExp = reader.readUInt32();
				box->manaDiff = reader.readUInt32();
				box->moraleDiff = reader.readInt8();
				box->luckDiff = reader.readInt8();

				readResourses(box->resources);

				box->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x = 0; x < 4; ++x)
				{
					box->primskills[x] = static_cast<PrimarySkill::PrimarySkill>(reader.readUInt8());
				}

				int gabn = reader.readUInt8();//number of gained abilities
				for(int oo = 0; oo < gabn; ++oo)
				{
					box->abilities.push_back(SecondarySkill(reader.readUInt8()));
					box->abilityLevels.push_back(reader.readUInt8());
				}
				int gart = reader.readUInt8(); //number of gained artifacts
				for(int oo = 0; oo < gart; ++oo)
				{
					if(map->version > EMapFormat::ROE)
					{
						box->artifacts.push_back(ArtifactID(reader.readUInt16()));
					}
					else
					{
						box->artifacts.push_back(ArtifactID(reader.readUInt8()));
					}
				}
				int gspel = reader.readUInt8(); //number of gained spells
				for(int oo = 0; oo < gspel; ++oo)
				{
					box->spells.push_back(SpellID(reader.readUInt8()));
				}
				int gcre = reader.readUInt8(); //number of gained creatures
				readCreatureSet(&box->creatures, gcre);
				reader.skip(8);
				break;
			}
		case Obj::GRAIL:
			{
				map->grailPos = objPos;
				map->grailRadius = reader.readUInt32();
				continue;
			}
		case Obj::RANDOM_DWELLING: //same as castle + level range
		case Obj::RANDOM_DWELLING_LVL: //same as castle, fixed level
		case Obj::RANDOM_DWELLING_FACTION: //level range, fixed faction
			{
				auto dwelling = new CGDwelling();
				nobj = dwelling;
				CSpecObjInfo * spec = nullptr;
				switch(objTempl->id)
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
				spec->owner = dwelling;

				nobj->setOwner(PlayerColor(reader.readUInt32()));

				//216 and 217
				if (auto castleSpec = dynamic_cast<CCreGenAsCastleInfo *>(spec))
				{
					castleSpec->instanceId = "";
					castleSpec->identifier = reader.readUInt32();
					if(!castleSpec->identifier)
					{
						castleSpec->asCastle = false;
						const int MASK_SIZE = 8;
						ui8 mask[2];
						mask[0] = reader.readUInt8();
						mask[1] = reader.readUInt8();

						castleSpec->allowedFactions.clear();
						castleSpec->allowedFactions.resize(VLC->townh->size(), false);

						for(int i = 0; i < MASK_SIZE; i++)
							castleSpec->allowedFactions[i] = ((mask[0] & (1 << i))>0);

						for(int i = 0; i < (GameConstants::F_NUMBER-MASK_SIZE); i++)
							castleSpec->allowedFactions[i+MASK_SIZE] = ((mask[1] & (1 << i))>0);
					}
					else
					{
						castleSpec->asCastle = true;
					}
				}

				//216 and 218
				if (auto lvlSpec = dynamic_cast<CCreGenLeveledInfo *>(spec))
				{
					lvlSpec->minLevel = std::max(reader.readUInt8(), ui8(1));
					lvlSpec->maxLevel = std::min(reader.readUInt8(), ui8(7));
				}
				dwelling->info = spec;
				break;
			}
		case Obj::QUEST_GUARD:
			{
				auto  guard = new CGQuestGuard();
				readQuest(guard);
				nobj = guard;
				break;
			}
		case Obj::SHIPYARD:
			{
				nobj = new CGShipyard();
				nobj->setOwner(PlayerColor(reader.readUInt32()));
				break;
			}
		case Obj::HERO_PLACEHOLDER: //hero placeholder
			{
				auto  hp = new CGHeroPlaceholder();
				nobj = hp;

				hp->setOwner(PlayerColor(reader.readUInt8()));

				int htid = reader.readUInt8(); //hero type id
				nobj->subID = htid;

				if(htid == 0xff)
				{
					hp->power = reader.readUInt8();
					logGlobal->info("Hero placeholder: by power at %s", objPos.toString());
				}
				else
				{
					logGlobal->info("Hero placeholder: %s at %s", VLC->heroh->objects[htid]->name, objPos.toString());
					hp->power = 0;
				}

				break;
			}
		case Obj::BORDERGUARD:
			{
				nobj = new CGBorderGuard();
				break;
			}
		case Obj::BORDER_GATE:
			{
				nobj = new CGBorderGate();
				break;
			}
		case Obj::PYRAMID: //Pyramid of WoG object
			{
				if(objTempl->subid == 0)
				{
					nobj = new CBank();
				}
				else
				{
					//WoG object
					//TODO: possible special handling
					nobj = new CGObjectInstance();
				}
				break;
			}
		case Obj::LIGHTHOUSE: //Lighthouse
			{
				nobj = new CGLighthouse();
				nobj->tempOwner = PlayerColor(reader.readUInt32());
				break;
			}
		default: //any other object
			{
				if (VLC->objtypeh->knownSubObjects(objTempl->id).count(objTempl->subid))
				{
					nobj = VLC->objtypeh->getHandlerFor(objTempl->id, objTempl->subid)->create(objTempl);
				}
				else
				{
					logGlobal->warn("Unrecognized object: %d:%d at %s on map %s", objTempl->id.toEnum(), objTempl->subid, objPos.toString(), map->name);
					nobj = new CGObjectInstance();
				}
				break;
			}
		}

		nobj->pos = objPos;
		nobj->ID = objTempl->id;
		nobj->id = idToBeGiven;
		if(nobj->ID != Obj::HERO && nobj->ID != Obj::HERO_PLACEHOLDER && nobj->ID != Obj::PRISON)
		{
			nobj->subID = objTempl->subid;
		}
		nobj->appearance = objTempl;
		assert(idToBeGiven == ObjectInstanceID((si32)map->objects.size()));

		{
			//TODO: define valid typeName and subtypeName fro H3M maps
			//boost::format fmt("%s_%d");
			//fmt % nobj->typeName % nobj->id.getNum();
			boost::format fmt("obj_%d");
			fmt % nobj->id.getNum();
			nobj->instanceName = fmt.str();
		}
		map->addNewObject(nobj);
	}

	std::sort(map->heroesOnMap.begin(), map->heroesOnMap.end(), [](const ConstTransitivePtr<CGHeroInstance> & a, const ConstTransitivePtr<CGHeroInstance> & b)
	{
		return a->subID < b->subID;
	});
}

void CMapLoaderH3M::readCreatureSet(CCreatureSet * out, int number)
{
	const bool version = (map->version > EMapFormat::ROE);
	const int maxID = version ? 0xffff : 0xff;

	for(int ir = 0; ir < number; ++ir)
	{
		CreatureID creID;
		int count;

		if (version)
		{
			creID = CreatureID(reader.readUInt16());
		}
		else
		{
			creID = CreatureID(reader.readUInt8());
		}
		count = reader.readUInt16();

		// Empty slot
		if(creID == maxID)
			continue;

		auto  hlp = new CStackInstance();
		hlp->count = count;

		if(creID > maxID - 0xf)
		{
			//this will happen when random object has random army
			hlp->idRand = maxID - creID - 1;
		}
		else
		{
			hlp->setType(creID);
		}

		out->putStack(SlotID(ir), hlp);
	}

	out->validTypes(true);
}

CGObjectInstance * CMapLoaderH3M::readHero(ObjectInstanceID idToBeGiven, const int3 & initialPos)
{
	auto nhi = new CGHeroInstance();

	if(map->version > EMapFormat::ROE)
	{
		unsigned int identifier = reader.readUInt32();
		map->questIdentifierToId[identifier] = idToBeGiven;
	}

	PlayerColor owner = PlayerColor(reader.readUInt8());
	nhi->subID = reader.readUInt8();

	assert(!nhi->getArt(ArtifactPosition::MACH4));

	//If hero of this type has been predefined, use that as a base.
	//Instance data will overwrite the predefined values where appropriate.
	for(auto & elem : map->predefinedHeroes)
	{
		if(elem->subID == nhi->subID)
		{
			logGlobal->debug("Hero %d will be taken from the predefined heroes list.", nhi->subID);
			delete nhi;
			nhi = elem;
			break;
		}
	}
	nhi->setOwner(owner);

	nhi->portrait = nhi->subID;

	for(auto & elem : map->disposedHeroes)
	{
		if(elem.heroId == nhi->subID)
		{
			nhi->name = elem.name;
			nhi->portrait = elem.portrait;
			break;
		}
	}

	bool hasName = reader.readBool();
	if(hasName)
	{
		nhi->name = reader.readString();
	}
	if(map->version > EMapFormat::AB)
	{
		bool hasExp = reader.readBool();
		if(hasExp)
		{
			nhi->exp = reader.readUInt32();
		}
		else
		{
			nhi->exp = 0xffffffff;
		}
	}
	else
	{
		nhi->exp = reader.readUInt32();

		//0 means "not set" in <=AB maps
		if(!nhi->exp)
		{
			nhi->exp = 0xffffffff;
		}
	}

	bool hasPortrait = reader.readBool();
	if(hasPortrait)
	{
		nhi->portrait = reader.readUInt8();
	}

	bool hasSecSkills = reader.readBool();
	if(hasSecSkills)
	{
		if(nhi->secSkills.size())
		{
			nhi->secSkills.clear();
			//logGlobal->warn("Hero %s subID=%d has set secondary skills twice (in map properties and on adventure map instance). Using the latter set...", nhi->name, nhi->subID);
		}

		int howMany = reader.readUInt32();
		nhi->secSkills.resize(howMany);
		for(int yy = 0; yy < howMany; ++yy)
		{
			nhi->secSkills[yy].first = SecondarySkill(reader.readUInt8());
			nhi->secSkills[yy].second = reader.readUInt8();
		}
	}

	bool hasGarison = reader.readBool();
	if(hasGarison)
	{
		readCreatureSet(nhi, 7);
	}

	nhi->formation = reader.readUInt8();
	loadArtifactsOfHero(nhi);
	nhi->patrol.patrolRadius = reader.readUInt8();
	nhi->patrol.patrolling = (nhi->patrol.patrolRadius != 0xff);

	if(map->version > EMapFormat::ROE)
	{
		bool hasCustomBiography = reader.readBool();
		if(hasCustomBiography)
		{
			nhi->biography = reader.readString();
		}
		nhi->sex = reader.readUInt8();

		// Remove trash
		if (nhi->sex != 0xFF)
		{
			nhi->sex &= 1;
		}
	}
	else
	{
		nhi->sex = 0xFF;
	}

	// Spells
	if(map->version > EMapFormat::AB)
	{
		bool hasCustomSpells = reader.readBool();
		if(nhi->spells.size())
		{
			nhi->clear();
			logGlobal->warn("Hero %s subID=%d has spells set twice (in map properties and on adventure map instance). Using the latter set...", nhi->name, nhi->subID);
		}

		if(hasCustomSpells)
		{
			nhi->spells.insert(SpellID::PRESET); //placeholder "preset spells"

			readSpells(nhi->spells);
		}
	}
	else if(map->version == EMapFormat::AB)
	{
		//we can read one spell
		ui8 buff = reader.readUInt8();
		if(buff != 254)
		{
			nhi->spells.insert(SpellID::PRESET); //placeholder "preset spells"
			if(buff < 254) //255 means no spells
			{
				nhi->spells.insert(SpellID(buff));
			}
		}
	}

	if(map->version > EMapFormat::AB)
	{
		bool hasCustomPrimSkills = reader.readBool();
		if(hasCustomPrimSkills)
		{
			auto ps = nhi->getAllBonuses(Selector::type()(Bonus::PRIMARY_SKILL)
								.And(Selector::sourceType()(Bonus::HERO_BASE_SKILL)), nullptr);
			if(ps->size())
			{
				logGlobal->warn("Hero %s subID=%d has set primary skills twice (in map properties and on adventure map instance). Using the latter set...", nhi->name, nhi->subID);
				for(auto b : *ps)
					nhi->removeBonus(b);
			}


			for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; ++xx)
			{
				nhi->pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(xx), reader.readUInt8());
			}
		}
	}
	reader.skip(16);
	return nhi;
}

CGSeerHut * CMapLoaderH3M::readSeerHut()
{
	auto  hut = new CGSeerHut();

	if(map->version > EMapFormat::ROE)
	{
		readQuest(hut);
	}
	else
	{
		//RoE
		auto artID = ArtifactID(reader.readUInt8());
		if (artID != 255)
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
		hut->quest->isCustomFirst = hut->quest->isCustomNext = hut->quest->isCustomComplete = false;
	}

	if (hut->quest->missionType)
	{
		auto rewardType = static_cast<CGSeerHut::ERewardType>(reader.readUInt8());
		hut->rewardType = rewardType;
		switch(rewardType)
		{
		case CGSeerHut::EXPERIENCE:
			{
				hut->rVal = reader.readUInt32();
				break;
			}
		case CGSeerHut::MANA_POINTS:
			{
				hut->rVal = reader.readUInt32();
				break;
			}
		case CGSeerHut::MORALE_BONUS:
			{
				hut->rVal = reader.readUInt8();
				break;
			}
		case CGSeerHut::LUCK_BONUS:
			{
				hut->rVal = reader.readUInt8();
				break;
			}
		case CGSeerHut::RESOURCES:
			{
				hut->rID = reader.readUInt8();
				// Only the first 3 bytes are used. Skip the 4th.
				hut->rVal = reader.readUInt32() & 0x00ffffff;
				break;
			}
		case CGSeerHut::PRIMARY_SKILL:
			{
				hut->rID = reader.readUInt8();
				hut->rVal = reader.readUInt8();
				break;
			}
		case CGSeerHut::SECONDARY_SKILL:
			{
				hut->rID = reader.readUInt8();
				hut->rVal = reader.readUInt8();
				break;
			}
		case CGSeerHut::ARTIFACT:
			{
				if (map->version == EMapFormat::ROE)
				{
					hut->rID = reader.readUInt8();
				}
				else
				{
					hut->rID = reader.readUInt16();
				}
				break;
			}
		case CGSeerHut::SPELL:
			{
				hut->rID = reader.readUInt8();
				break;
			}
		case CGSeerHut::CREATURE:
			{
				if(map->version > EMapFormat::ROE)
				{
					hut->rID = reader.readUInt16();
					hut->rVal = reader.readUInt16();
				}
				else
				{
					hut->rID = reader.readUInt8();
					hut->rVal = reader.readUInt16();
				}
				break;
			}
		}
		reader.skip(2);
	}
	else
	{
		// missionType==255
		reader.skip(3);
	}

	return hut;
}

void CMapLoaderH3M::readQuest(IQuestObject * guard)
{
	guard->quest->missionType = static_cast<CQuest::Emission>(reader.readUInt8());

	switch(guard->quest->missionType)
	{
	case CQuest::MISSION_NONE:
		return;
	case CQuest::MISSION_PRIMARY_STAT:
		{
			guard->quest->m2stats.resize(4);
			for(int x = 0; x < 4; ++x)
			{
				guard->quest->m2stats[x] = reader.readUInt8();
			}
		}
		break;
	case CQuest::MISSION_LEVEL:
	case CQuest::MISSION_KILL_HERO:
	case CQuest::MISSION_KILL_CREATURE:
		{
			guard->quest->m13489val = reader.readUInt32();
			break;
		}
	case CQuest::MISSION_ART:
		{
			int artNumber = reader.readUInt8();
			for(int yy = 0; yy < artNumber; ++yy)
			{
				auto artid = ArtifactID(reader.readUInt16());
				guard->quest->addArtifactID(artid);
				map->allowedArtifact[artid] = false; //these are unavailable for random generation
			}
			break;
		}
	case CQuest::MISSION_ARMY:
		{
			int typeNumber = reader.readUInt8();
			guard->quest->m6creatures.resize(typeNumber);
			for(int hh = 0; hh < typeNumber; ++hh)
			{
				guard->quest->m6creatures[hh].type = VLC->creh->objects[reader.readUInt16()];
				guard->quest->m6creatures[hh].count = reader.readUInt16();
			}
			break;
		}
	case CQuest::MISSION_RESOURCES:
		{
			guard->quest->m7resources.resize(7);
			for(int x = 0; x < 7; ++x)
			{
				guard->quest->m7resources[x] = reader.readUInt32();
			}
			break;
		}
	case CQuest::MISSION_HERO:
	case CQuest::MISSION_PLAYER:
		{
			guard->quest->m13489val = reader.readUInt8();
			break;
		}
	}

	int limit = reader.readUInt32();
	if(limit == (static_cast<int>(0xffffffff)))
	{
		guard->quest->lastDay = -1;
	}
	else
	{
		guard->quest->lastDay = limit;
	}
	guard->quest->firstVisitText = reader.readString();
	guard->quest->nextVisitText = reader.readString();
	guard->quest->completedText = reader.readString();
	guard->quest->isCustomFirst = guard->quest->firstVisitText.size() > 0;
	guard->quest->isCustomNext = guard->quest->nextVisitText.size() > 0;
	guard->quest->isCustomComplete = guard->quest->completedText.size() > 0;
}

CGTownInstance * CMapLoaderH3M::readTown(int castleID)
{
	auto  nt = new CGTownInstance();
	if(map->version > EMapFormat::ROE)
	{
		nt->identifier = reader.readUInt32();
	}
	nt->tempOwner = PlayerColor(reader.readUInt8());
	bool hasName = reader.readBool();
	if(hasName)
	{
		nt->name = reader.readString();
	}

	bool hasGarrison = reader.readBool();
	if(hasGarrison)
	{
		readCreatureSet(nt, 7);
	}
	nt->formation = reader.readUInt8();

	bool hasCustomBuildings = reader.readBool();
	if(hasCustomBuildings)
	{
		readBitmask(nt->builtBuildings,6,48,false);

		readBitmask(nt->forbiddenBuildings,6,48,false);

		nt->builtBuildings = convertBuildings(nt->builtBuildings, castleID);
		nt->forbiddenBuildings = convertBuildings(nt->forbiddenBuildings, castleID);
	}
	// Standard buildings
	else
	{
		bool hasFort = reader.readBool();
		if(hasFort)
		{
			nt->builtBuildings.insert(BuildingID::FORT);
		}

		//means that set of standard building should be included
		nt->builtBuildings.insert(BuildingID::DEFAULT);
	}

	if(map->version > EMapFormat::ROE)
	{
		for(int i = 0; i < 9; ++i)
		{
			ui8 c = reader.readUInt8();
			for(int yy = 0; yy < 8; ++yy)
			{
				if(i * 8 + yy < GameConstants::SPELLS_QUANTITY)
				{
					if(c == (c | static_cast<ui8>(std::pow(2., yy)))) //add obligatory spell even if it's banned on a map (?)
					{
						nt->obligatorySpells.push_back(SpellID(i * 8 + yy));
					}
				}
			}
		}
	}

	for(int i = 0; i < 9; ++i)
	{
		ui8 c = reader.readUInt8();
		for(int yy = 0; yy < 8; ++yy)
		{
			int spellid = i * 8 + yy;
			if(spellid < GameConstants::SPELLS_QUANTITY)
			{
				if(c != (c | static_cast<ui8>(std::pow(2., yy))) && map->allowedSpell[spellid]) //add random spell only if it's allowed on entire map
				{
					nt->possibleSpells.push_back(SpellID(spellid));
				}
			}
		}
	}
	//add all spells from mods
	//TODO: allow customize new spells in towns
	for (int i = SpellID::AFTER_LAST; i < VLC->spellh->objects.size(); ++i)
	{
		nt->possibleSpells.push_back(SpellID(i));
	}

	// Read castle events
	int numberOfEvent = reader.readUInt32();

	for(int gh = 0; gh < numberOfEvent; ++gh)
	{
		CCastleEvent nce;
		nce.town = nt;
		nce.name = reader.readString();
		nce.message = reader.readString();

		readResourses(nce.resources);

		nce.players = reader.readUInt8();
		if(map->version > EMapFormat::AB)
		{
			nce.humanAffected = reader.readUInt8();
		}
		else
		{
			nce.humanAffected = true;
		}

		nce.computerAffected = reader.readUInt8();
		nce.firstOccurence = reader.readUInt16();
		nce.nextOccurence =  reader.readUInt8();

		reader.skip(17);

		// New buildings

		readBitmask(nce.buildings,6,48,false);

		nce.buildings = convertBuildings(nce.buildings, castleID, false);

		nce.creatures.resize(7);
		for(int vv = 0; vv < 7; ++vv)
		{
			nce.creatures[vv] = reader.readUInt16();
		}
		reader.skip(4);
		nt->events.push_back(nce);
	}

	if(map->version > EMapFormat::AB)
	{
		nt->alignment = reader.readUInt8();
	}
	reader.skip(3);

	return nt;
}

std::set<BuildingID> CMapLoaderH3M::convertBuildings(const std::set<BuildingID> h3m, int castleID, bool addAuxiliary)
{
	std::map<int, BuildingID> mapa;
	std::set<BuildingID> ret;

	// Note: this file is parsed many times.
	const JsonNode config(ResourceID("config/buildings5.json"));

	for(const JsonNode & entry : config["table"].Vector())
	{
		int town = static_cast<int>(entry["town"].Float());

		if (town == castleID || town == -1)
		{
			mapa[(int)entry["h3"].Float()] = BuildingID((si32)entry["vcmi"].Float());
		}
	}

	for(auto & elem : h3m)
	{
		if(mapa[elem] >= 0)
		{
			ret.insert(mapa[elem]);
		}
		// horde buildings
		else if(mapa[elem] >= (-GameConstants::CREATURES_PER_TOWN))
		{
			int level = (mapa[elem]);

			//(-30)..(-36) - horde buildings (for game loading only), don't see other way to handle hordes in random towns
			ret.insert(BuildingID(level - 30));
		}
		else
		{
			logGlobal->warn("Conversion warning: unknown building %d in castle %d", elem.num, castleID);
		}
	}

	if(addAuxiliary)
	{
		//village hall is always present
		ret.insert(BuildingID::VILLAGE_HALL);

		if(ret.find(BuildingID::CITY_HALL) != ret.end())
		{
			ret.insert(BuildingID::EXTRA_CITY_HALL);
		}
		if(ret.find(BuildingID::TOWN_HALL) != ret.end())
		{
			ret.insert(BuildingID::EXTRA_TOWN_HALL);
		}
		if(ret.find(BuildingID::CAPITOL) != ret.end())
		{
			ret.insert(BuildingID::EXTRA_CAPITOL);
		}
	}

	return ret;
}

void CMapLoaderH3M::readEvents()
{
	int numberOfEvents = reader.readUInt32();
	for(int yyoo = 0; yyoo < numberOfEvents; ++yyoo)
	{
		CMapEvent ne;
		ne.name = reader.readString();
		ne.message = reader.readString();

		readResourses(ne.resources);
		ne.players = reader.readUInt8();
		if(map->version > EMapFormat::AB)
		{
			ne.humanAffected = reader.readUInt8();
		}
		else
		{
			ne.humanAffected = true;
		}
		ne.computerAffected = reader.readUInt8();
		ne.firstOccurence = reader.readUInt16();
		ne.nextOccurence = reader.readUInt8();

		reader.skip(17);

		map->events.push_back(ne);
	}
}

void CMapLoaderH3M::readMessageAndGuards(std::string& message, CCreatureSet* guards)
{
	bool hasMessage = reader.readBool();
	if(hasMessage)
	{
		message = reader.readString();
		bool hasGuards = reader.readBool();
		if(hasGuards)
		{
			readCreatureSet(guards, 7);
		}
		reader.skip(4);
	}
}

void CMapLoaderH3M::readSpells(std::set<SpellID>& dest)
{
	readBitmask(dest,9,GameConstants::SPELLS_QUANTITY,false);
}

void CMapLoaderH3M::readResourses(TResources& resources)
{
	resources.resize(GameConstants::RESOURCE_QUANTITY); //needed?
	for(int x = 0; x < 7; ++x)
	{
		resources[x] = reader.readUInt32();
	}
}

template <class Indentifier>
void CMapLoaderH3M::readBitmask(std::set<Indentifier>& dest, const int byteCount, const int limit, bool negate)
{
	std::vector<bool> temp;
	temp.resize(limit,true);
	readBitmask(temp, byteCount, limit, negate);

	for(int i = 0; i< std::min(temp.size(), static_cast<size_t>(limit)); i++)
	{
		if(temp[i])
		{
			dest.insert(static_cast<Indentifier>(i));
		}
	}
}

void CMapLoaderH3M::readBitmask(std::vector<bool>& dest, const int byteCount, const int limit, bool negate)
{
	for(int byte = 0; byte < byteCount; ++byte)
	{
		const ui8 mask = reader.readUInt8();
		for(int bit = 0; bit < 8; ++bit)
		{
			if(byte * 8 + bit < limit)
			{
				const bool flag = mask & (1 << bit);
				if((negate && flag) || (!negate && !flag)) // FIXME: check PR388
					dest[byte * 8 + bit] = false;
			}
		}
	}
}

ui8 CMapLoaderH3M::reverse(ui8 arg)
{
	ui8 ret = 0;
	for(int i = 0; i < 8; ++i)
	{
		if((arg & (1 << i)) >> i)
		{
			ret |= (128 >> i);
		}
	}
	return ret;
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

			for(auto obj : t.visitableObjects)
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
