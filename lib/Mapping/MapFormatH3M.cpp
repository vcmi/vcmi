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

#include <boost/crc.hpp>

#include "../CStopWatch.h"

#include "../Filesystem/CResourceLoader.h"
#include "../Filesystem/CInputStream.h"
#include "CMap.h"

#include "../CSpellHandler.h"
#include "../CCreatureHandler.h"
#include "../CHeroHandler.h"
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"

#include "../VCMI_Lib.h"


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

	return std::unique_ptr<CMap>(dynamic_cast<CMap *>(mapHeader.release()));;
}

std::unique_ptr<CMapHeader> CMapLoaderH3M::loadMapHeader()
{
	// Read header
	mapHeader = make_unique<CMapHeader>();
	readHeader();

	return std::move(mapHeader);
}

void CMapLoaderH3M::init()
{
	//FIXME: get rid of double input process
	si64 temp_size = inputStream->getSize();
	inputStream->seek(0);

	ui8 * temp_buffer = new ui8[temp_size];
	inputStream->read(temp_buffer,temp_size);

	// Compute checksum
	boost::crc_32_type  result;
	result.process_bytes(temp_buffer, temp_size);
	map->checksum = result.checksum();

	delete temp_buffer;
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

	// Calculate blocked / visitable positions
	for(int f = 0; f < map->objects.size(); ++f)
	{
		if(!map->objects[f]->defInfo) continue;
		map->addBlockVisTiles(map->objects[f]);
	}
	times.push_back(MapLoadingTime("blocked/visitable tiles", sw.getDiff()));

	// Print profiling times
	if(IS_PROFILING_ENABLED)
	{
		BOOST_FOREACH(MapLoadingTime & mlt, times)
		{
			tlog0 << "\tReading " << mlt.name << " took " << mlt.time << " ms." << std::endl;
		}
	}
}

void CMapLoaderH3M::readHeader()
{
	// Check map for validity
	if(inputStream->getSize() < 50)
	{
		throw std::runtime_error("Corrupted map file.");
	}

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
			allowedFactions += reader.readUInt8() * 256;
		else
			totalFactions--; //exclude conflux for ROE

		for(int fact = 0; fact < totalFactions; ++fact)
		{
			if(!(allowedFactions & (1 << fact)))
			{
				mapHeader->players[i].allowedFactions.erase(fact);
			}
		}

		mapHeader->players[i].isFactionRandom = reader.readBool();
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

		mapHeader->players[i].hasHero = reader.readBool();
		mapHeader->players[i].customHeroID = reader.readUInt8();

		if(mapHeader->players[i].customHeroID != 0xff)
		{
			mapHeader->players[i].mainHeroPortrait = reader.readUInt8();
			if (mapHeader->players[i].mainHeroPortrait == 0xff)
				mapHeader->players[i].mainHeroPortrait = -1; //correct 1-byte -1 (0xff) into 4-byte -1

			mapHeader->players[i].mainHeroName = reader.readString();
		}
		else
			mapHeader->players[i].customHeroID = -1; //correct 1-byte -1 (0xff) into 4-byte -1

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

void CMapLoaderH3M::readVictoryLossConditions()
{
	mapHeader->victoryCondition.obj = nullptr;
	mapHeader->victoryCondition.condition = (EVictoryConditionType::EVictoryConditionType)reader.readUInt8();

	// Specific victory conditions
	if(mapHeader->victoryCondition.condition != EVictoryConditionType::WINSTANDARD)
	{
		mapHeader->victoryCondition.allowNormalVictory = reader.readBool();
		mapHeader->victoryCondition.appliesToAI = reader.readBool();

		// Read victory conditions
//		int nr = 0;
		switch(mapHeader->victoryCondition.condition)
		{
		case EVictoryConditionType::ARTIFACT:
			{
				mapHeader->victoryCondition.objectId = reader.readUInt8();
				if (mapHeader->version != EMapFormat::ROE)
					reader.skip(1);
				break;
			}
		case EVictoryConditionType::GATHERTROOP:
			{
				mapHeader->victoryCondition.objectId = reader.readUInt8();
				if (mapHeader->version != EMapFormat::ROE)
					reader.skip(1);
				mapHeader->victoryCondition.count = reader.readUInt32();
				break;
			}
		case EVictoryConditionType::GATHERRESOURCE:
			{
				mapHeader->victoryCondition.objectId = reader.readUInt8();
				mapHeader->victoryCondition.count = reader.readUInt32();
				break;
			}
		case EVictoryConditionType::BUILDCITY:
			{
				mapHeader->victoryCondition.pos = readInt3();
				mapHeader->victoryCondition.count = reader.readUInt8();
				mapHeader->victoryCondition.objectId = reader.readUInt8();
				break;
			}
		case EVictoryConditionType::BUILDGRAIL:
			{
				int3 p = readInt3();
				if(p.z > 2)
				{
					p = int3(-1,-1,-1);
				}
				mapHeader->victoryCondition.pos = p;
				break;
			}
		case EVictoryConditionType::BEATHERO:
		case EVictoryConditionType::CAPTURECITY:
		case EVictoryConditionType::BEATMONSTER:
			{
				mapHeader->victoryCondition.pos = readInt3();
				break;
			}
		case EVictoryConditionType::TAKEDWELLINGS:
		case EVictoryConditionType::TAKEMINES:
			{
				break;
			}
		case EVictoryConditionType::TRANSPORTITEM:
			{
				mapHeader->victoryCondition.objectId = reader.readUInt8();
				mapHeader->victoryCondition.pos = readInt3();
				break;
			}
		default:
			assert(0);
		}
	}

	// Read loss conditions
	mapHeader->lossCondition.typeOfLossCon = (ELossConditionType::ELossConditionType) reader.readUInt8();
	switch(mapHeader->lossCondition.typeOfLossCon)
	{
	case ELossConditionType::LOSSCASTLE:
	case ELossConditionType::LOSSHERO:
		{
			mapHeader->lossCondition.pos = readInt3();
			break;
		}
	case ELossConditionType::TIMEEXPIRES:
		{
			mapHeader->lossCondition.timeLimit = reader.readUInt16();
			break;
		}
	}
}

void CMapLoaderH3M::readTeamInfo()
{
	mapHeader->howManyTeams = reader.readUInt8();
	if(mapHeader->howManyTeams > 0)
	{
		// Teams
		for(int i = 0; i < GameConstants::PLAYER_LIMIT; ++i)
		{
			mapHeader->players[i].team = reader.readUInt8();
		}
	}
	else
	{
		// No alliances
		for(int i = 0; i < GameConstants::PLAYER_LIMIT; i++)
		{
			if(mapHeader->players[i].canComputerPlay || mapHeader->players[i].canHumanPlay)
			{
				mapHeader->players[i].team = mapHeader->howManyTeams++;
			}
		}
	}
}

void CMapLoaderH3M::readAllowedHeroes()
{
	mapHeader->allowedHeroes.resize(VLC->heroh->heroes.size(), true);

	const int bytes = mapHeader->version == EMapFormat::ROE ? 16 : 20;

	readBitmask(mapHeader->allowedHeroes,bytes,GameConstants::HEROES_QUANTITY, false);

	// Probably reserved for further heroes
	if(mapHeader->version > EMapFormat::ROE)
	{
		int placeholdersQty = reader.readUInt32();
		for(int p = 0; p < placeholdersQty; ++p)
		{
			mapHeader->placeholdedHeroes.push_back(reader.readUInt8());
		}
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
	map->allowedArtifact.resize (VLC->arth->artifacts.size(),true); //handle new artifacts, make them allowed by default

	// Reading allowed artifacts:  17 or 18 bytes
	if(map->version != EMapFormat::ROE)
	{
		const int bytes = map->version == EMapFormat::AB ? 17 : 18;

		readBitmask(map->allowedArtifact,bytes,GameConstants::ARTIFACTS_QUANTITY);

	}

	// ban combo artifacts
	if (map->version == EMapFormat::ROE || map->version == EMapFormat::AB)
	{
		BOOST_FOREACH(CArtifact * artifact, VLC->arth->artifacts)
		{
			// combo
			if (artifact->constituents)
			{
				map->allowedArtifact[artifact->id] = false;
			}
		}
		if (map->version == EMapFormat::ROE)
		{
			// Armageddon's Blade
			map->allowedArtifact[128] = false;
		}
	}

	// Messy, but needed
	if(map->victoryCondition.condition == EVictoryConditionType::ARTIFACT
			|| map->victoryCondition.condition == EVictoryConditionType::TRANSPORTITEM)
	{
		map->allowedArtifact[map->victoryCondition.objectId] = false;
	}
}

void CMapLoaderH3M::readAllowedSpellsAbilities()
{
	// Read allowed spells
	map->allowedSpell.resize(GameConstants::SPELLS_QUANTITY, true);

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

				CGHeroInstance * hero = new CGHeroInstance();
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
						hero->secSkills[yy].first = static_cast<SecondarySkill::SecondarySkill>(reader.readUInt8());
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
		for(int pom = 0; pom < 16; pom++)
		{
			loadArtifactToSlot(hero, pom);
		}

		// misc5 art //17
		if(map->version >= EMapFormat::SOD)
		{
			if(!loadArtifactToSlot(hero, ArtifactPosition::MACH4))
			{
				// catapult by default
				hero->putArtifact(ArtifactPosition::MACH4, createArtifact(ArtifactID::CATAPULT));
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
			loadArtifactToSlot(hero, GameConstants::BACKPACK_START + hero->artifactsInBackpack.size());
		}
	}
}

bool CMapLoaderH3M::loadArtifactToSlot(CGHeroInstance * hero, int slot)
{
	const int artmask = map->version == EMapFormat::ROE ? 0xff : 0xffff;
	int aid;

	if(map->version == EMapFormat::ROE)
	{
		aid = reader.readUInt8();
	}
	else
	{
		aid = reader.readUInt16();
	}

	bool isArt  =  aid != artmask;
	if(isArt)
	{
		if(vstd::contains(VLC->arth->bigArtifacts, aid) && slot >= GameConstants::BACKPACK_START)
		{
			tlog3 << "Warning: A big artifact (war machine) in hero's backpack, ignoring..." << std::endl;
			return false;
		}
		if(aid == 0 && slot == ArtifactPosition::MISC5)
		{
			//TODO: check how H3 handles it -> art 0 in slot 18 in AB map
			tlog3 << "Spellbook to MISC5 slot? Putting it spellbook place. AB format peculiarity ? (format "
				  << static_cast<int>(map->version) << ")" << std::endl;
			slot = ArtifactPosition::SPELLBOOK;
		}

		hero->putArtifact(static_cast<ArtifactPosition::ArtifactPosition>(slot), createArtifact(aid));
	}

	return isArt;
}

CArtifactInstance * CMapLoaderH3M::createArtifact(int aid, int spellID /*= -1*/)
{
	CArtifactInstance * a = nullptr;
	if(aid >= 0)
	{
		if(spellID < 0)
		{
			a = CArtifactInstance::createNewArtifactInstance(aid);
		}
		else
		{
			a = CArtifactInstance::createScroll(VLC->spellh->spells[spellID]);
		}
	}
	else
	{
		a = new CArtifactInstance();
	}

	map->addNewArtifactInstance(a);

	//TODO make it nicer
	if(a->artType && a->artType->constituents)
	{
		CCombinedArtifactInstance * comb = dynamic_cast<CCombinedArtifactInstance *>(a);
		BOOST_FOREACH(CCombinedArtifactInstance::ConstituentInfo & ci, comb->constituentsInfo)
		{
			map->addNewArtifactInstance(ci.art);
		}
	}

	return a;
}

void CMapLoaderH3M::readTerrain()
{
	map->initTerrain();

	// Read terrain
	for(int a = 0; a < 2; ++a)
	{
		if(a == 1 && !map->twoLevel)
		{
			break;
		}

		for(int c = 0; c < map->width; c++)
		{
			for(int z = 0; z < map->height; z++)
			{
				map->terrain[z][c][a].terType = static_cast<ETerrainType::ETerrainType>(reader.readUInt8());
				map->terrain[z][c][a].terView = reader.readUInt8();
				map->terrain[z][c][a].riverType = static_cast<ERiverType::ERiverType>(reader.readUInt8());
				map->terrain[z][c][a].riverDir = reader.readUInt8();
				map->terrain[z][c][a].roadType = static_cast<ERoadType::ERoadType>(reader.readUInt8());
				map->terrain[z][c][a].roadDir = reader.readUInt8();
				map->terrain[z][c][a].extTileFlags = reader.readUInt8();
				map->terrain[z][c][a].blocked = (map->terrain[z][c][a].terType == ETerrainType::ROCK ? 1 : 0); //underground tiles are always blocked
				map->terrain[z][c][a].visitable = 0;
			}
		}
	}
}

void CMapLoaderH3M::readDefInfo()
{
	int defAmount = reader.readUInt32();

	map->customDefs.reserve(defAmount + 8);

	// Read custom defs
	for(int idd = 0; idd < defAmount; ++idd)
	{
		CGDefInfo * defInfo = new CGDefInfo();

		defInfo->name = reader.readString();
		std::transform(defInfo->name.begin(),defInfo->name.end(),defInfo->name.begin(),(int(*)(int))toupper);

		ui8 bytes[12];
		for(int v = 0; v < 12; ++v)
		{
			bytes[v] = reader.readUInt8();
		}

		defInfo->terrainAllowed = reader.readUInt16();
		defInfo->terrainMenu = reader.readUInt16();
		defInfo->id = Obj(reader.readUInt32());
		defInfo->subid = reader.readUInt32();
		defInfo->type = reader.readUInt8();
		defInfo->printPriority = reader.readUInt8();

		for(int zi = 0; zi < 6; ++zi)
		{
			defInfo->blockMap[zi] = reverse(bytes[zi]);
		}
		for(int zi = 0; zi < 6; ++zi)
		{
			defInfo->visitMap[zi] = reverse(bytes[6 + zi]);
		}

		reader.skip(16);
		if(defInfo->id != Obj::HERO && defInfo->id != Obj::RANDOM_HERO)
		{
			CGDefInfo * h = VLC->dobjinfo->gobjs[defInfo->id][defInfo->subid];
			if(!h)
			{
				//remove fake entry
				VLC->dobjinfo->gobjs[defInfo->id].erase(defInfo->subid);
				if(VLC->dobjinfo->gobjs[defInfo->id].size())
				{
					VLC->dobjinfo->gobjs.erase(defInfo->id);
				}
				tlog2 << "\t\tWarning: no defobjinfo entry for object ID="
					  << defInfo->id << " subID=" << defInfo->subid << std::endl;
			}
			else
			{
				defInfo->visitDir = VLC->dobjinfo->gobjs[defInfo->id][defInfo->subid]->visitDir;
			}
		}
		else
		{
			defInfo->visitDir = 0xff;
		}

		if(defInfo->id == Obj::EVENT)
		{
			std::memset(defInfo->blockMap, 255, 6);
		}

		//calculating coverageMap
		defInfo->fetchInfoFromMSK();

		map->customDefs.push_back(defInfo);
	}

	//add holes - they always can appear
	for(int i = 0; i < 8 ; ++i)
	{
		map->customDefs.push_back(VLC->dobjinfo->gobjs[Obj::HOLE][i]);
	}
}

void CMapLoaderH3M::readObjects()
{
	int howManyObjs = reader.readUInt32();

	for(int ww = 0; ww < howManyObjs; ++ww)
	{
		CGObjectInstance * nobj = 0;

		int3 objPos = readInt3();

		int defnum = reader.readUInt32();
		int idToBeGiven = map->objects.size();

		CGDefInfo * defInfo = map->customDefs.at(defnum);
		reader.skip(5);

		switch(defInfo->id)
		{
		case Obj::EVENT:
			{
				CGEvent * evnt = new CGEvent();
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
					evnt->abilities.push_back(static_cast<SecondarySkill::SecondarySkill>(reader.readUInt8()));
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
				nobj = readHero(idToBeGiven);
				break;
			}
		case Obj::ARENA:
		case Obj::MERCENARY_CAMP:
		case Obj::MARLETTO_TOWER:
		case Obj::STAR_AXIS:
		case Obj::GARDEN_OF_REVELATION:
		case Obj::LEARNING_STONE:
		case Obj::TREE_OF_KNOWLEDGE:
		case Obj::LIBRARY_OF_ENLIGHTENMENT:
		case Obj::SCHOOL_OF_MAGIC:
		case Obj::SCHOOL_OF_WAR:
			{
				nobj = new CGVisitableOPH();
				break;
			}
		case Obj::MYSTICAL_GARDEN:
		case Obj::WINDMILL:
		case Obj::WATER_WHEEL:
			{
				nobj = new CGVisitableOPW();
				break;
			}
		case Obj::MONOLITH1:
		case Obj::MONOLITH2:
		case Obj::MONOLITH3:
		case Obj::SUBTERRANEAN_GATE:
		case Obj::WHIRLPOOL:
			{
				nobj = new CGTeleport();
				break;
			}
		case Obj::CAMPFIRE:
		case Obj::FLOTSAM:
		case Obj::SEA_CHEST:
		case Obj::SHIPWRECK_SURVIVOR:
			{
				nobj = new CGPickable();
				break;
			}
		case Obj::TREASURE_CHEST:
				if(defInfo->subid == 0)
				{
					nobj = new CGPickable();
				}
				else
				{
					//WoG pickable object
					//TODO: possible special handling
					nobj = new CGObjectInstance();
				}
				break;
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
				CGCreature * cre = new CGCreature();
				nobj = cre;

				if(map->version > EMapFormat::ROE)
				{
					cre->identifier = reader.readUInt32();
					map->questIdentifierToId[cre->identifier] = idToBeGiven;
				}

				CStackInstance * hlp = new CStackInstance();
				hlp->count = reader.readUInt16();

				//type will be set during initialization
				cre->putStack(0, hlp);

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
				CGSignBottle * sb = new CGSignBottle();
				nobj = sb;
				sb->message = reader.readString();
				reader.skip(4);
				break;
			}
		case Obj::SEER_HUT:
			{
				nobj = readSeerHut();
				map->addQuest(nobj);
				break;
			}
		case Obj::WITCH_HUT:
			{
				CGWitchHut * wh = new CGWitchHut();
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
				}
				else
				{
					// RoE map
					for(int gg = 0; gg < GameConstants::SKILL_QUANTITY; ++gg)
					{
						wh->allowedAbilities.push_back(gg);
					}
				}
				break;
			}
		case Obj::SCHOLAR:
			{
				CGScholar * sch = new CGScholar();
				nobj = sch;
				sch->bonusType = static_cast<CGScholar::EBonusType>(reader.readUInt8());
				sch->bonusID = reader.readUInt8();
				reader.skip(6);
				break;
			}
		case Obj::GARRISON:
		case Obj::GARRISON2:
			{
				CGGarrison * gar = new CGGarrison();
				nobj = gar;
				nobj->setOwner(reader.readUInt8());
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
				int artID = -1;
				int spellID = -1;
				CGArtifact * art = new CGArtifact();
				nobj = art;

				readMessageAndGuards(art->message, art);

				if(defInfo->id == Obj::SPELL_SCROLL)
				{
					spellID = reader.readUInt32();
					artID = 1;
				}
				else if(defInfo->id == Obj::ARTIFACT)
				{
					//specific artifact
					artID = defInfo->subid;
				}

				art->storedArtifact = createArtifact(artID, spellID);
				break;
			}
		case Obj::RANDOM_RESOURCE:
		case Obj::RESOURCE:
			{
				CGResource * res = new CGResource();
				nobj = res;

				readMessageAndGuards(res->message, res);

				res->amount = reader.readUInt32();
				if(defInfo->subid == Res::GOLD)
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
				nobj = readTown(defInfo->subid);
				break;
			}
		case Obj::MINE:
		case Obj::ABANDONED_MINE:
			{
				nobj = new CGMine();
				nobj->setOwner(reader.readUInt8());
				reader.skip(3);
				break;
			}
		case Obj::CREATURE_GENERATOR1:
		case Obj::CREATURE_GENERATOR2:
		case Obj::CREATURE_GENERATOR3:
		case Obj::CREATURE_GENERATOR4:
			{
				nobj = new CGDwelling();
				nobj->setOwner(reader.readUInt8());
				reader.skip(3);
				break;
			}
		case Obj::REFUGEE_CAMP:
		case Obj::WAR_MACHINE_FACTORY:
			{
				nobj = new CGDwelling();
				break;
			}
		case Obj::SHRINE_OF_MAGIC_INCANTATION:
		case Obj::SHRINE_OF_MAGIC_GESTURE:
		case Obj::SHRINE_OF_MAGIC_THOUGHT:
			{
				CGShrine * shr = new CGShrine();
				nobj = shr;
				shr->spell = reader.readUInt8();
				reader.skip(3);
				break;
			}
		case Obj::PANDORAS_BOX:
			{
				CGPandoraBox * box = new CGPandoraBox();
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
					box->abilities.push_back(static_cast<SecondarySkill::SecondarySkill>(reader.readUInt8()));
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
				map->grailRadious = reader.readUInt32();
				continue;
			}
		case Obj::RANDOM_DWELLING: //same as castle + level range
		case Obj::RANDOM_DWELLING_LVL: //same as castle, fixed level
		case Obj::RANDOM_DWELLING_FACTION: //level range, fixed faction
			{
				nobj = new CGDwelling();
				CSpecObjInfo * spec = nullptr;
				switch(defInfo->id)
				{
					break; case Obj::RANDOM_DWELLING: spec = new CCreGenLeveledCastleInfo();
					break; case Obj::RANDOM_DWELLING_LVL: spec = new CCreGenAsCastleInfo();
					break; case Obj::RANDOM_DWELLING_FACTION: spec = new CCreGenLeveledInfo();
				}

				spec->player = reader.readUInt32();

				//216 and 217
				if (auto castleSpec = dynamic_cast<CCreGenAsCastleInfo *>(spec))
				{
					castleSpec->identifier =  reader.readUInt32();
					if(!castleSpec->identifier)
					{
						castleSpec->asCastle = false;
						castleSpec->castles[0] = reader.readUInt8();
						castleSpec->castles[1] = reader.readUInt8();
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
				nobj->setOwner(spec->player);
				static_cast<CGDwelling *>(nobj)->info = spec;
				break;
			}
		case Obj::QUEST_GUARD:
			{
				CGQuestGuard * guard = new CGQuestGuard();
				map->addQuest(guard);
				readQuest(guard);
				nobj = guard;
				break;
			}
		case Obj::FAERIE_RING:
		case Obj::SWAN_POND:
		case Obj::IDOL_OF_FORTUNE:
		case Obj::FOUNTAIN_OF_FORTUNE:
		case Obj::RALLY_FLAG:
		case Obj::OASIS:
		case Obj::TEMPLE:
		case Obj::WATERING_HOLE:
		case Obj::FOUNTAIN_OF_YOUTH:
		case Obj::BUOY:
		case Obj::MERMAID:
		case Obj::STABLES:
			{
				nobj = new CGBonusingObject();
				break;
			}
		case Obj::MAGIC_WELL:
			{
				nobj = new CGMagicWell();
				break;
			}
		case Obj::COVER_OF_DARKNESS:
		case Obj::REDWOOD_OBSERVATORY:
		case Obj::PILLAR_OF_FIRE:
			{
				nobj = new CGObservatory();
				break;
			}
		case Obj::CORPSE:
		case Obj::LEAN_TO:
		case Obj::WAGON:
		case Obj::WARRIORS_TOMB:
			{
				nobj = new CGOnceVisitable();
				break;
			}
		case Obj::BOAT:
			{
				nobj = new CGBoat();
				break;
			}
		case Obj::SIRENS:
			{
				nobj = new CGSirens();
				break;
			}
		case Obj::SHIPYARD:
			{
				nobj = new CGShipyard();
				nobj->setOwner(reader.readUInt32());
				break;
			}
		case Obj::HERO_PLACEHOLDER: //hero placeholder
			{
				CGHeroPlaceholder * hp = new CGHeroPlaceholder();
				nobj = hp;

				hp->setOwner(reader.readUInt8());

				int htid = reader.readUInt8();; //hero type id
				nobj->subID = htid;

				if(htid == 0xff)
				{
					hp->power = reader.readUInt8();
				}
				else
				{
					hp->power = 0;
				}

				break;
			}
		case Obj::KEYMASTER:
			{
				nobj = new CGKeymasterTent();
				break;
			}
		case Obj::BORDERGUARD:
			{
				nobj = new CGBorderGuard();
				map->addQuest(nobj);
				break;
			}
		case Obj::BORDER_GATE:
			{
				nobj = new CGBorderGate();
				map->addQuest (nobj);
				break;
			}
		case Obj::EYE_OF_MAGI:
		case Obj::HUT_OF_MAGI:
			{
				nobj = new CGMagi();
				break;
			}
		case Obj::CREATURE_BANK:
		case Obj::DERELICT_SHIP:
		case Obj::DRAGON_UTOPIA:
		case Obj::CRYPT:
		case Obj::SHIPWRECK:
			{
				nobj = new CBank();
				break;
			}
		case Obj::PYRAMID: //Pyramid of WoG object
			{
				if(defInfo->subid == 0)
				{
					nobj = new CGPyramid();
				}
				else
				{
					//WoG object
					//TODO: possible special handling
					nobj = new CGObjectInstance();
				}
				break;
			}
		case Obj::CARTOGRAPHER:
			{
				nobj = new CCartographer();
				break;
			}
		case Obj::MAGIC_SPRING:
			{
				nobj = new CGMagicSpring();
				break;
			}
		case Obj::DEN_OF_THIEVES:
			{
				nobj = new CGDenOfthieves();
				break;
			}
		case Obj::OBELISK:
			{
				nobj = new CGObelisk();
				break;
			}
		case Obj::LIGHTHOUSE: //Lighthouse
			{
				nobj = new CGLighthouse();
				nobj->tempOwner = reader.readUInt32();
				break;
			}
		case Obj::ALTAR_OF_SACRIFICE:
		case Obj::TRADING_POST:
		case Obj::FREELANCERS_GUILD:
		case Obj::TRADING_POST_SNOW:
			{
				nobj = new CGMarket();
				break;
			}
		case Obj::UNIVERSITY:
			{
				nobj = new CGUniversity();
				break;
			}
		case Obj::BLACK_MARKET:
			{
				nobj = new CGBlackMarket();
				break;
			}
		default: //any other object
			{
				nobj = new CGObjectInstance();
				break;
			}
		}

		nobj->pos = objPos;
		nobj->ID = defInfo->id;
		nobj->id = idToBeGiven;
		if(nobj->ID != Obj::HERO && nobj->ID != Obj::HERO_PLACEHOLDER && nobj->ID != Obj::PRISON)
		{
			nobj->subID = defInfo->subid;
		}
		nobj->defInfo = defInfo;
		assert(idToBeGiven == map->objects.size());
		map->objects.push_back(nobj);
		if(nobj->ID == Obj::TOWN)
		{
			map->towns.push_back(static_cast<CGTownInstance *>(nobj));
		}
		if(nobj->ID == Obj::HERO)
		{
			map->heroes.push_back(static_cast<CGHeroInstance*>(nobj));
		}
	}

	std::sort(map->heroes.begin(), map->heroes.end(), [](const ConstTransitivePtr<CGHeroInstance> & a, const ConstTransitivePtr<CGHeroInstance> & b)
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
		if(creID == maxID) continue;

		CStackInstance * hlp = new CStackInstance();
		hlp->count = count;

		if(creID > maxID - 0xf)
		{
			//this will happen when random object has random army
			creID = CreatureID(maxID + 1 - creID + VLC->creh->creatures.size());
			hlp->idRand = creID;
		}
		else
		{
			hlp->setType(creID);
		}

		out->putStack(ir, hlp);
	}

	out->validTypes(true);
}

CGObjectInstance * CMapLoaderH3M::readHero(int idToBeGiven)
{
	CGHeroInstance * nhi = new CGHeroInstance();

	int identifier = 0;
	if(map->version > EMapFormat::ROE)
	{
		identifier = reader.readUInt32();
		map->questIdentifierToId[identifier] = idToBeGiven;
	}

	ui8 owner = reader.readUInt8();
	nhi->subID = reader.readUInt8();

	for(int j = 0; j < map->predefinedHeroes.size(); ++j)
	{
		if(map->predefinedHeroes[j]->subID == nhi->subID)
		{
			tlog0 << "Hero " << nhi->subID << " will be taken from the predefined heroes list." << std::endl;
			delete nhi;
			nhi = map->predefinedHeroes[j];
			break;
		}
	}
	nhi->setOwner(owner);

	nhi->portrait = nhi->subID;

	for(int j = 0; j < map->disposedHeroes.size(); ++j)
	{
		if(map->disposedHeroes[j].heroId == nhi->subID)
		{
			nhi->name = map->disposedHeroes[j].name;
			nhi->portrait = map->disposedHeroes[j].portrait;
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
		int howMany = reader.readUInt32();
		nhi->secSkills.resize(howMany);
		for(int yy = 0; yy < howMany; ++yy)
		{
			nhi->secSkills[yy].first = static_cast<SecondarySkill::SecondarySkill>(reader.readUInt8());
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
	nhi->patrol.patrolRadious = reader.readUInt8();
	if(nhi->patrol.patrolRadious == 0xff)
	{
		nhi->patrol.patrolling = false;
	}
	else
	{
		nhi->patrol.patrolling = true;
	}

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
	CGSeerHut * hut = new CGSeerHut();

	if(map->version > EMapFormat::ROE)
	{
		readQuest(hut);
	}
	else
	{
		//RoE
		int artID = reader.readUInt8();
		if (artID != 255)
		{
			//not none quest
			hut->quest->m5arts.push_back (artID);
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
	case 0:
		return;
	case 2:
		{
			guard->quest->m2stats.resize(4);
			for(int x = 0; x < 4; ++x)
			{
				guard->quest->m2stats[x] = reader.readUInt8();
			}
		}
		break;
	case 1:
	case 3:
	case 4:
		{
			guard->quest->m13489val = reader.readUInt32();
			break;
		}
	case 5:
		{
			int artNumber = reader.readUInt8();
			for(int yy = 0; yy < artNumber; ++yy)
			{
				int artid = reader.readUInt16();
				guard->quest->m5arts.push_back(artid);
				map->allowedArtifact[artid] = false; //these are unavailable for random generation
			}
			break;
		}
	case 6:
		{
			int typeNumber = reader.readUInt8();
			guard->quest->m6creatures.resize(typeNumber);
			for(int hh = 0; hh < typeNumber; ++hh)
			{
				guard->quest->m6creatures[hh].type = VLC->creh->creatures[reader.readUInt16()];
				guard->quest->m6creatures[hh].count = reader.readUInt16();
			}
			break;
		}
	case 7:
		{
			guard->quest->m7resources.resize(7);
			for(int x = 0; x < 7; ++x)
			{
				guard->quest->m7resources[x] = reader.readUInt32();
			}
			break;
		}
	case 8:
	case 9:
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
	CGTownInstance * nt = new CGTownInstance();
	if(map->version > EMapFormat::ROE)
	{
		nt->identifier = reader.readUInt32();
	}
	nt->tempOwner = reader.readUInt8();
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
		nt->builtBuildings.insert(-50);
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
					if(c == (c | static_cast<ui8>(std::pow(2., yy))))
					{
						nt->obligatorySpells.push_back(i * 8 + yy);
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
			if(i * 8 + yy < GameConstants::SPELLS_QUANTITY)
			{
				if(c != (c | static_cast<ui8>(std::pow(2., yy))))
				{
					nt->possibleSpells.push_back(i * 8 + yy);
				}
			}
		}
	}

	// Read castle events
	int numberOfEvent = reader.readUInt32();

	for(int gh = 0; gh < numberOfEvent; ++gh)
	{
		CCastleEvent * nce = new CCastleEvent();
		nce->town = nt;
		nce->name = reader.readString();
		nce->message = reader.readString();

		readResourses(nce->resources);

		nce->players = reader.readUInt8();
		if(map->version > EMapFormat::AB)
		{
			nce->humanAffected = reader.readUInt8();
		}
		else
		{
			nce->humanAffected = true;
		}

		nce->computerAffected = reader.readUInt8();
		nce->firstOccurence = reader.readUInt16();
		nce->nextOccurence =  reader.readUInt8();

		reader.skip(17);

		// New buildings

		readBitmask(nce->buildings,6,48,false);

		nce->buildings = convertBuildings(nce->buildings, castleID, false);

		nce->creatures.resize(7);
		for(int vv = 0; vv < 7; ++vv)
		{
			nce->creatures[vv] = reader.readUInt16();
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

std::set<si32> CMapLoaderH3M::convertBuildings(const std::set<si32> h3m, int castleID, bool addAuxiliary /*= true*/)
{
	std::map<int, int> mapa;
	std::set<si32> ret;

	// Note: this file is parsed many times.
	const JsonNode config(ResourceID("config/buildings5.json"));

	BOOST_FOREACH(const JsonNode & entry, config["table"].Vector())
	{
		int town = entry["town"].Float();

		if (town == castleID || town == -1)
		{
			mapa[entry["h3"].Float()] = entry["vcmi"].Float();
		}
	}

	for(auto i = h3m.begin(); i != h3m.end(); ++i)
	{
		if(mapa[*i] >= 0)
		{
			ret.insert(mapa[*i]);
		}
		// horde buildings
		else if(mapa[*i] >= (-GameConstants::CREATURES_PER_TOWN))
		{
			int level = (mapa[*i]);

			//(-30)..(-36) - horde buildings (for game loading only), don't see other way to handle hordes in random towns
			ret.insert(level - 30);
		}
		else
		{
			tlog3 << "Conversion warning: unknown building " << *i << " in castle "
				  << castleID << std::endl;
		}
	}

	if(addAuxiliary)
	{
		//village hall is always present
		ret.insert(BuildingID::VILLAGE_HALL);
	}

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

	return ret;
}

void CMapLoaderH3M::readEvents()
{
	int numberOfEvents = reader.readUInt32();
	for(int yyoo = 0; yyoo < numberOfEvents; ++yyoo)
	{
		CMapEvent * ne = new CMapEvent();
		ne->name = reader.readString();
		ne->message = reader.readString();

		readResourses(ne->resources);
		ne->players = reader.readUInt8();
		if(map->version > EMapFormat::AB)
		{
			ne->humanAffected = reader.readUInt8();
		}
		else
		{
			ne->humanAffected = true;
		}
		ne->computerAffected = reader.readUInt8();
		ne->firstOccurence = reader.readUInt16();
		ne->nextOccurence = reader.readUInt8();

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

template <class Indenifier>
void CMapLoaderH3M::readBitmask(std::set<Indenifier>& dest, const int byteCount, const int limit, bool negate)
{
	std::vector<bool> temp;
	temp.resize(limit,true);
	readBitmask(temp, byteCount, limit, negate);

	for(int i = 0; i< std::min(temp.size(), static_cast<size_t>(limit)); i++)
	{
		if(temp[i])
		{
			dest.insert(static_cast<Indenifier>(i));
		}
//		else
//		{
//			dest.erase(static_cast<Indenifier>(i));
//		}
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
				if((negate && flag) || (!negate && !flag))
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
