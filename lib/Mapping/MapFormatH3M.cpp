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
#include "../Filesystem/CBinaryReader.h"
#include "../Filesystem/CInputStream.h"
#include "CMap.h"

#include "../CSpellHandler.h"
#include "../CCreatureHandler.h"
#include "../CHeroHandler.h"
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"

#include "../VCMI_Lib.h"


const bool CMapLoaderH3M::IS_PROFILING_ENABLED = false;

CMapLoaderH3M::CMapLoaderH3M(CInputStream * stream) : map(nullptr), buffer(nullptr), pos(-1), size(-1)
{
	initBuffer(stream);
}

CMapLoaderH3M::~CMapLoaderH3M()
{
	delete buffer;
}

void CMapLoaderH3M::initBuffer(CInputStream * stream)
{
	// Read map data into memory completely
	// TODO Replace with CBinaryReader later... (no need for reading whole
	// file in memory at once, storing pos & size separately...)
	stream->seek(0);
	size = stream->getSize();
	buffer = new ui8[size];
	stream->read(buffer, size);
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
	// Compute checksum
	boost::crc_32_type  result;
	result.process_bytes(buffer, size);
	map->checksum = result.checksum();

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
	pos = 0;

	// Check map for validity
	if(size < 50  || !buffer[4])
	{
		throw std::runtime_error("Corrupted map file.");
	}

	// Map version
	mapHeader->version = (EMapFormat::EMapFormat)(readUI32());
	if(mapHeader->version != EMapFormat::ROE && mapHeader->version != EMapFormat::AB && mapHeader->version != EMapFormat::SOD
			&& mapHeader->version != EMapFormat::WOG)
	{
		throw std::runtime_error("Invalid map format!");
	}

	// Read map name, description, dimensions,...
	mapHeader->areAnyPlayers = readBool();
	mapHeader->height = mapHeader->width = readUI32();
	mapHeader->twoLevel = readBool();
	mapHeader->name = readString();
	mapHeader->description = readString();
	mapHeader->difficulty = readSI8();
	if(mapHeader->version != EMapFormat::ROE)
	{
		mapHeader->levelLimit = readUI8();
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
		mapHeader->players[i].canHumanPlay = readBool();
		mapHeader->players[i].canComputerPlay = readBool();

		// If nobody can play with this player
		if((!(mapHeader->players[i].canHumanPlay || mapHeader->players[i].canComputerPlay)))
		{
			switch(mapHeader->version)
			{
			case EMapFormat::SOD:
			case EMapFormat::WOG:
				skip(13);
				break;
			case EMapFormat::AB:
				skip(12);
				break;
			case EMapFormat::ROE:
				skip(6);
				break;
			}
			continue;
		}

		mapHeader->players[i].aiTactic = static_cast<EAiTactic::EAiTactic>(readUI8());

		if(mapHeader->version == EMapFormat::SOD || mapHeader->version == EMapFormat::WOG)
		{
			mapHeader->players[i].p7 = readUI8();
		}
		else
		{
			mapHeader->players[i].p7 = -1;
		}

		// Factions this player can choose
		ui16 allowedFactions = readUI8();
		// How many factions will be read from map
		ui16 totalFactions = GameConstants::F_NUMBER;

		if(mapHeader->version != EMapFormat::ROE)
			allowedFactions += readUI8() * 256;
		else
			totalFactions--; //exclude conflux for ROE

		for(int fact = 0; fact < totalFactions; ++fact)
		{
			if(!(allowedFactions & (1 << fact)))
			{
				mapHeader->players[i].allowedFactions.erase(fact);
			}
		}

		mapHeader->players[i].isFactionRandom = readBool();
		mapHeader->players[i].hasMainTown = readBool();
		if(mapHeader->players[i].hasMainTown)
		{
			if(mapHeader->version != EMapFormat::ROE)
			{
				mapHeader->players[i].generateHeroAtMainTown = readBool();
				mapHeader->players[i].generateHero = readBool();
			}
			else
			{
				mapHeader->players[i].generateHeroAtMainTown = true;
				mapHeader->players[i].generateHero = false;
			}

			mapHeader->players[i].posOfMainTown = readInt3();
		}

		mapHeader->players[i].hasHero = readBool();
		mapHeader->players[i].customHeroID = readUI8();

		if(mapHeader->players[i].customHeroID != 0xff)
		{
			mapHeader->players[i].mainHeroPortrait = readUI8();
			if (mapHeader->players[i].mainHeroPortrait == 0xff)
				mapHeader->players[i].mainHeroPortrait = -1; //correct 1-byte -1 (0xff) into 4-byte -1

			mapHeader->players[i].mainHeroName = readString();
		}
		else
			mapHeader->players[i].customHeroID = -1; //correct 1-byte -1 (0xff) into 4-byte -1

		if(mapHeader->version != EMapFormat::ROE)
		{
			mapHeader->players[i].powerPlaceholders = readUI8(); //unknown byte
			int heroCount = readUI8();
			skip(3);
			for(int pp = 0; pp < heroCount; ++pp)
			{
				SHeroName vv;
				vv.heroId = readUI8();
				vv.heroName = readString();

				mapHeader->players[i].heroesNames.push_back(vv);
			}
		}
	}
}

void CMapLoaderH3M::readVictoryLossConditions()
{
	mapHeader->victoryCondition.obj = nullptr;
	mapHeader->victoryCondition.condition = (EVictoryConditionType::EVictoryConditionType)readUI8();

	// Specific victory conditions
	if(mapHeader->victoryCondition.condition != EVictoryConditionType::WINSTANDARD)
	{
		mapHeader->victoryCondition.allowNormalVictory = readBool();
		mapHeader->victoryCondition.appliesToAI = readBool();

		// Read victory conditions
//		int nr = 0;
		switch(mapHeader->victoryCondition.condition)
		{
		case EVictoryConditionType::ARTIFACT:
			{
				mapHeader->victoryCondition.objectId = readUI8();
				if (mapHeader->version != EMapFormat::ROE)
					skip(1);
				break;
			}
		case EVictoryConditionType::GATHERTROOP:
			{
				mapHeader->victoryCondition.objectId = readUI8();
				if (mapHeader->version != EMapFormat::ROE)
					skip(1);
				mapHeader->victoryCondition.count = readUI32();
				break;
			}
		case EVictoryConditionType::GATHERRESOURCE:
			{
				mapHeader->victoryCondition.objectId = readUI8();
				mapHeader->victoryCondition.count = readUI32();
				break;
			}
		case EVictoryConditionType::BUILDCITY:
			{
				mapHeader->victoryCondition.pos = readInt3();
				mapHeader->victoryCondition.count = readUI8();
				mapHeader->victoryCondition.objectId = readUI8();
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
				mapHeader->victoryCondition.objectId = readUI8();
				mapHeader->victoryCondition.pos = readInt3();
				break;
			}
		default:
			assert(0);
		}
	}

	// Read loss conditions
	mapHeader->lossCondition.typeOfLossCon = (ELossConditionType::ELossConditionType)buffer[pos++];
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
			mapHeader->lossCondition.timeLimit = readUI16();
			break;
		}
	}
}

void CMapLoaderH3M::readTeamInfo()
{
	mapHeader->howManyTeams = readUI8();
	if(mapHeader->howManyTeams > 0)
	{
		// Teams
		for(int i = 0; i < GameConstants::PLAYER_LIMIT; ++i)
		{
			mapHeader->players[i].team = readUI8();
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
		int placeholdersQty = readUI32();
		for(int p = 0; p < placeholdersQty; ++p)
		{
			mapHeader->placeholdedHeroes.push_back(readUI8());
		}
	}
}

void CMapLoaderH3M::readDisposedHeroes()
{
	// Reading disposed heroes (20 bytes)
	if(map->version >= EMapFormat::SOD)
	{
		ui8 disp = readUI8();
		map->disposedHeroes.resize(disp);
		for(int g = 0; g < disp; ++g)
		{
			map->disposedHeroes[g].heroId = readUI8();
			map->disposedHeroes[g].portrait = readUI8();
			map->disposedHeroes[g].name = readString();
			map->disposedHeroes[g].players = readUI8();
		}
	}

	//omitting NULLS
	skip(31);
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
	int rumNr = readUI32();

	for(int it = 0; it < rumNr; it++)
	{
		Rumor ourRumor;
		ourRumor.name = readString();
		ourRumor.text = readString();
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
				int custom =  readUI8();
				if(!custom) continue;

				CGHeroInstance * hero = new CGHeroInstance();
				hero->ID = Obj::HERO;
				hero->subID = z;

				// True if hore's experience is greater than 0
				if(readBool())
				{
					hero->exp = readUI32();
				}
				else
				{
					hero->exp = 0;
				}

				// True if hero has specified abilities
				if(readBool())
				{
					int howMany = readUI32();
					hero->secSkills.resize(howMany);
					for(int yy = 0; yy < howMany; ++yy)
					{
						hero->secSkills[yy].first = static_cast<SecondarySkill::SecondarySkill>(readUI8());
						hero->secSkills[yy].second = readUI8();
					}
				}

				loadArtifactsOfHero(hero);

				// custom bio
				if(readChar(buffer, pos))
				{
					hero->biography = readString();
				}

				// 0xFF is default, 00 male, 01 female
				hero->sex = readUI8();

				// are spells
				if(readBool())
				{
					readSpells(hero->spells);
				}

				// customPrimSkills
				if(readBool())
				{
					for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; xx++)
					{
						hero->pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(xx), readUI8());
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
	bool artSet = readBool();

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
				hero->putArtifact(ArtifactPosition::MACH4, createArtifact(GameConstants::ID_CATAPULT));
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
			skip(1);
		}

		// bag artifacts //20
		// number of artifacts in hero's bag
		int amount = readUI16();
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
		aid = readUI8();
	}
	else
	{
		aid = readUI16();
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

		hero->putArtifact(slot, createArtifact(aid));
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
				map->terrain[z][c][a].terType = static_cast<ETerrainType::ETerrainType>(readUI8());
				map->terrain[z][c][a].terView = readUI8();
				map->terrain[z][c][a].riverType = static_cast<ERiverType::ERiverType>(readUI8());
				map->terrain[z][c][a].riverDir = readUI8();
				map->terrain[z][c][a].roadType = static_cast<ERoadType::ERoadType>(readUI8());
				map->terrain[z][c][a].roadDir = readUI8();
				map->terrain[z][c][a].extTileFlags = readUI8();
				map->terrain[z][c][a].blocked = (map->terrain[z][c][a].terType == ETerrainType::ROCK ? 1 : 0); //underground tiles are always blocked
				map->terrain[z][c][a].visitable = 0;
			}
		}
	}
}

void CMapLoaderH3M::readDefInfo()
{
	int defAmount = readUI32();

	map->customDefs.reserve(defAmount + 8);

	// Read custom defs
	for(int idd = 0; idd < defAmount; ++idd)
	{
		CGDefInfo * defInfo = new CGDefInfo();

		defInfo->name = readString();
		std::transform(defInfo->name.begin(),defInfo->name.end(),defInfo->name.begin(),(int(*)(int))toupper);

		ui8 bytes[12];
		for(int v = 0; v < 12; ++v)
		{
			bytes[v] = readUI8();
		}

		defInfo->terrainAllowed = readUI16();
		defInfo->terrainMenu = readUI16();
		defInfo->id = readUI32();
		defInfo->subid = readUI32();
		defInfo->type = readUI8();
		defInfo->printPriority = readUI8();

		for(int zi = 0; zi < 6; ++zi)
		{
			defInfo->blockMap[zi] = reverse(bytes[zi]);
		}
		for(int zi = 0; zi < 6; ++zi)
		{
			defInfo->visitMap[zi] = reverse(bytes[6 + zi]);
		}

		skip(16);
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
	int howManyObjs = readUI32();

	for(int ww = 0; ww < howManyObjs; ++ww)
	{
		CGObjectInstance * nobj = 0;

		int3 objPos = readInt3();

		int defnum = readUI32();
		int idToBeGiven = map->objects.size();

		CGDefInfo * defInfo = map->customDefs.at(defnum);
		skip(5);

		switch(defInfo->id)
		{
		case Obj::EVENT:
			{
				CGEvent * evnt = new CGEvent();
				nobj = evnt;

				if(readBool())
				{
					evnt->message = readString();

					if(readBool())
					{
						readCreatureSet(evnt, 7);
					}
					skip(4);
				}
				evnt->gainedExp = readUI32();
				evnt->manaDiff = readUI32();
				evnt->moraleDiff = readSI8();
				evnt->luckDiff = readSI8();

				readResourses(evnt->resources);

				evnt->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x = 0; x < 4; ++x)
				{
					evnt->primskills[x] = static_cast<PrimarySkill::PrimarySkill>(readUI8());
				}

				int gabn = readUI8(); // Number of gained abilities
				for(int oo = 0; oo < gabn; ++oo)
				{
					evnt->abilities.push_back(static_cast<SecondarySkill::SecondarySkill>(readUI8()));
					evnt->abilityLevels.push_back(readUI8());
				}

				int gart = readUI8(); // Number of gained artifacts
				for(int oo = 0; oo < gart; ++oo)
				{
					if(map->version == EMapFormat::ROE)
					{
						evnt->artifacts.push_back(readUI8());
					}
					else
					{
						evnt->artifacts.push_back(readUI16());
					}
				}

				int gspel = readUI8(); // Number of gained spells
				for(int oo = 0; oo < gspel; ++oo)
				{
					evnt->spells.push_back(readUI8());
				}

				int gcre = readUI8(); //number of gained creatures
				readCreatureSet(&evnt->creatures, gcre);

				skip(8);
				evnt->availableFor = readUI8();
				evnt->computerActivate = readUI8();
				evnt->removeAfterVisit = readUI8();
				evnt->humanActivate = true;

				skip(4);
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
					cre->identifier = readUI32();
					map->questIdentifierToId[cre->identifier] = idToBeGiven;
				}

				CStackInstance * hlp = new CStackInstance();
				hlp->count = readUI16();

				//type will be set during initialization
				cre->putStack(0, hlp);

				cre->character = readUI8();
				if(readBool()) //true if there is message or treasury
				{
					cre->message = readString();
					readResourses(cre->resources);

					int artID;
					if (map->version == EMapFormat::ROE)
					{
						artID = readUI8();
					}
					else
					{
						artID = readUI16();
					}

					if(map->version == EMapFormat::ROE)
					{
						if(artID != 0xff)
						{
							cre->gainedArtifact = artID;
						}
						else
						{
							cre->gainedArtifact = -1;
						}
					}
					else
					{
						if(artID != 0xffff)
						{
							cre->gainedArtifact = artID;
						}
						else
						{
							cre->gainedArtifact = -1;
						}
					}
				}
				cre->neverFlees = readUI8();
				cre->notGrowingTeam =readUI8();
				skip(2);
				break;
			}
		case Obj::OCEAN_BOTTLE:
		case Obj::SIGN:
			{
				CGSignBottle * sb = new CGSignBottle();
				nobj = sb;
				sb->message = readString();
				skip(4);
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
						ui8 c = readUI8();
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
				sch->bonusType = static_cast<CGScholar::EBonusType>(readUI8());
				sch->bonusID = readUI8();
				skip(6);
				break;
			}
		case Obj::GARRISON:
		case Obj::GARRISON2:
			{
				CGGarrison * gar = new CGGarrison();
				nobj = gar;
				nobj->setOwner(readUI8());
				skip(3);
				readCreatureSet(gar, 7);
				if(map->version > EMapFormat::ROE)
				{
					gar->removableUnits = readBool();
				}
				else
				{
					gar->removableUnits = true;
				}
				skip(8);
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

				if(readBool())
				{
					art->message = readString();
					if(readBool())
					{
						readCreatureSet(art, 7);
					}
					skip(4);
				}

				if(defInfo->id == Obj::SPELL_SCROLL)
				{
					spellID = readUI32();
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

				if(readBool())
				{
					res->message = readString();
					if(readBool())
					{
						readCreatureSet(res, 7);
					}
					skip(4);
				}
				res->amount = readUI32();
				if(defInfo->subid == Res::GOLD)
				{
					// Gold is multiplied by 100.
					res->amount *= 100;
				}
				skip(4);
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
				nobj->setOwner(readUI8());
				skip(3);
				break;
			}
		case Obj::CREATURE_GENERATOR1:
		case Obj::CREATURE_GENERATOR2:
		case Obj::CREATURE_GENERATOR3:
		case Obj::CREATURE_GENERATOR4:
			{
				nobj = new CGDwelling();
				nobj->setOwner(readUI8());
				skip(3);
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
				shr->spell = readUI8();
				skip(3);
				break;
			}
		case Obj::PANDORAS_BOX:
			{
				CGPandoraBox * box = new CGPandoraBox();
				nobj = box;
				if(readBool())
				{
					box->message = readString();
					if(readBool())
					{
						readCreatureSet(box, 7);
					}
					skip(4);
				}

				box->gainedExp = readUI32();
				box->manaDiff = readUI32();
				box->moraleDiff = readSI8();
				box->luckDiff = readSI8();

				readResourses(box->resources);

				box->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x = 0; x < 4; ++x)
				{
					box->primskills[x] = static_cast<PrimarySkill::PrimarySkill>(readUI8());
				}

				int gabn = readUI8();//number of gained abilities
				for(int oo = 0; oo < gabn; ++oo)
				{
					box->abilities.push_back(static_cast<SecondarySkill::SecondarySkill>(readUI8()));
					box->abilityLevels.push_back(readUI8());
				}
				int gart = readUI8(); //number of gained artifacts
				for(int oo = 0; oo < gart; ++oo)
				{
					if(map->version > EMapFormat::ROE)
					{
						box->artifacts.push_back(readUI16());
					}
					else
					{
						box->artifacts.push_back(readUI8());
					}
				}
				int gspel = readUI8(); //number of gained spells
				for(int oo = 0; oo < gspel; ++oo)
				{
					box->spells.push_back(readUI8());
				}
				int gcre = readUI8(); //number of gained creatures
				readCreatureSet(&box->creatures, gcre);
				skip(8);
				break;
			}
		case Obj::GRAIL:
			{
				map->grailPos = objPos;
				map->grailRadious = readUI32();
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

				spec->player = readUI32();

				//216 and 217
				if (auto castleSpec = dynamic_cast<CCreGenAsCastleInfo *>(spec))
				{
					castleSpec->identifier =  readUI32();
					if(!castleSpec->identifier)
					{
						castleSpec->asCastle = false;
						castleSpec->castles[0] = readUI8();
						castleSpec->castles[1] = readUI8();
					}
					else
					{
						castleSpec->asCastle = true;
					}
				}

				//216 and 218
				if (auto lvlSpec = dynamic_cast<CCreGenLeveledInfo *>(spec))
				{
					lvlSpec->minLevel = std::max(readUI8(), ui8(1));
					lvlSpec->maxLevel = std::min(readUI8(), ui8(7));
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
				nobj->setOwner(readUI32());
				break;
			}
		case Obj::HERO_PLACEHOLDER: //hero placeholder
			{
				CGHeroPlaceholder * hp = new CGHeroPlaceholder();
				nobj = hp;

				int a = readUI8();//unknown byte, seems to be always 0 (if not - scream!)
				tlog2 << "Unhandled Hero Placeholder detected: " << a << std::endl;

				int htid = readUI8();; //hero type id
				nobj->subID = htid;

				if(htid == 0xff)
				{
					hp->power = readUI8();
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
				nobj->tempOwner = readUI32();
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
		int creID;
		int count;

		if (version)
		{
			creID = readUI16();
		}
		else
		{
			creID = readUI8();
		}
		count = readUI16();

		// Empty slot
		if(creID == maxID) continue;

		CStackInstance * hlp = new CStackInstance();
		hlp->count = count;

		if(creID > maxID - 0xf)
		{
			//this will happen when random object has random army
			creID = maxID + 1 - creID + VLC->creh->creatures.size();
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
		identifier = readUI32();
		map->questIdentifierToId[identifier] = idToBeGiven;
	}

	ui8 owner = readUI8();
	nhi->subID = readUI8();

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

	// True if hero has nonstandard name
	if(readBool())
	{
		nhi->name = readString();
	}
	if(map->version > EMapFormat::AB)
	{
		// True if hero's experience is greater than 0
		if(readBool())
		{
			nhi->exp = readUI32();
		}
		else
		{
			nhi->exp = 0xffffffff;
		}
	}
	else
	{
		nhi->exp = readUI32();

		//0 means "not set" in <=AB maps
		if(!nhi->exp)
		{
			nhi->exp = 0xffffffff;
		}
	}

	// True if hero has specified portrait
	if(readBool())
	{
		nhi->portrait = readUI8();
	}

	// True if hero has specified abilities
	if(readBool())
	{
		int howMany = readUI32();
		nhi->secSkills.resize(howMany);
		for(int yy = 0; yy < howMany; ++yy)
		{
			nhi->secSkills[yy].first = static_cast<SecondarySkill::SecondarySkill>(readUI8());
			nhi->secSkills[yy].second = readUI8();
		}
	}

	// True if hero has nonstandard garrison
	if(readBool())
	{
		readCreatureSet(nhi, 7);
	}

	nhi->formation = readUI8();
	loadArtifactsOfHero(nhi);
	nhi->patrol.patrolRadious = readUI8();
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
		// True if hero has nonstandard (mapmaker defined) biography
		if(readBool())
		{
			nhi->biography = readString();
		}
		nhi->sex = readUI8();

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
		if(readBool())
		{
			nhi->spells.insert(0xffffffff); //placeholder "preset spells"

			readSpells(nhi->spells);
		}
	}
	else if(map->version == EMapFormat::AB)
	{
		//we can read one spell
		ui8 buff = readUI8();
		if(buff != 254)
		{
			nhi->spells.insert(0xffffffff); //placeholder "preset spells"
			if(buff < 254) //255 means no spells
			{
				nhi->spells.insert(buff);
			}
		}
	}

	if(map->version > EMapFormat::AB)
	{
		//customPrimSkills
		if(readBool())
		{
			for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; ++xx)
			{
				nhi->pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(xx), readUI8());
			}
		}
	}
	skip(16);
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
		int artID = readUI8();
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
		ui8 rewardType = readUI8();
		hut->rewardType = static_cast<CGSeerHut::ERewardType>(rewardType);

		switch(rewardType)
		{
		case 1:
			{
				hut->rVal = readUI32();
				break;
			}
		case 2:
			{
				hut->rVal = readUI32();
				break;
			}
		case 3:
			{
				hut->rVal = readUI8();
				break;
			}
		case 4:
			{
				hut->rVal = readUI8();
				break;
			}
		case 5:
			{
				hut->rID = readUI8();
				// Only the first 3 bytes are used. Skip the 4th.
				hut->rVal = readUI32() & 0x00ffffff;
				break;
			}
		case 6:
			{
				hut->rID = readUI8();
				hut->rVal = readUI8();
				break;
			}
		case 7:
			{
				hut->rID = readUI8();
				hut->rVal = readUI8();
				break;
			}
		case 8:
			{
				if (map->version == EMapFormat::ROE)
				{
					hut->rID = readUI8();
				}
				else
				{
					hut->rID = readUI16();
				}
				break;
			}
		case 9:
			{
				hut->rID = readUI8();
				break;
			}
		case 10:
			{
				if(map->version > EMapFormat::ROE)
				{
					hut->rID = readUI16();
					hut->rVal = readUI16();
				}
				else
				{
					hut->rID = readUI8();
					hut->rVal = readUI16();
				}
				break;
			}
		}
		skip(2);
	}
	else
	{
		// missionType==255
		skip(3);
	}

	return hut;
}

void CMapLoaderH3M::readQuest(IQuestObject * guard)
{
	guard->quest->missionType = static_cast<CQuest::Emission>(readUI8());

	switch(guard->quest->missionType)
	{
	case 0:
		return;
	case 2:
		{
			guard->quest->m2stats.resize(4);
			for(int x = 0; x < 4; ++x)
			{
				guard->quest->m2stats[x] = readUI8();
			}
		}
		break;
	case 1:
	case 3:
	case 4:
		{
			guard->quest->m13489val = readUI32();
			break;
		}
	case 5:
		{
			int artNumber = readUI8();
			for(int yy = 0; yy < artNumber; ++yy)
			{
				int artid = readUI16();
				guard->quest->m5arts.push_back(artid);
				map->allowedArtifact[artid] = false; //these are unavailable for random generation
			}
			break;
		}
	case 6:
		{
			int typeNumber = readUI8();
			guard->quest->m6creatures.resize(typeNumber);
			for(int hh = 0; hh < typeNumber; ++hh)
			{
				guard->quest->m6creatures[hh].type = VLC->creh->creatures[readUI16()];
				guard->quest->m6creatures[hh].count = readUI16();
			}
			break;
		}
	case 7:
		{
			guard->quest->m7resources.resize(7);
			for(int x = 0; x < 7; ++x)
			{
				guard->quest->m7resources[x] = readUI32();
			}
			break;
		}
	case 8:
	case 9:
		{
			guard->quest->m13489val = readUI8();
			break;
		}
	}

	int limit = readUI32();
	if(limit == (static_cast<int>(0xffffffff)))
	{
		guard->quest->lastDay = -1;
	}
	else
	{
		guard->quest->lastDay = limit;
	}
	guard->quest->firstVisitText = readString();
	guard->quest->nextVisitText = readString();
	guard->quest->completedText = readString();
	guard->quest->isCustomFirst = guard->quest->firstVisitText.size() > 0;
	guard->quest->isCustomNext = guard->quest->nextVisitText.size() > 0;
	guard->quest->isCustomComplete = guard->quest->completedText.size() > 0;
}

CGTownInstance * CMapLoaderH3M::readTown(int castleID)
{
	CGTownInstance * nt = new CGTownInstance();
	if(map->version > EMapFormat::ROE)
	{
		nt->identifier = readUI32();
	}
	nt->tempOwner = readUI8();
	if(readBool())// Has name
	{
		nt->name = readString();
	}

	// True if garrison isn't empty
	if(readBool())
	{
		readCreatureSet(nt, 7);
	}
	nt->formation = readUI8();

	// Custom buildings info
	if(readBool())
	{
		// Built buildings
		for(int byte = 0; byte < 6; ++byte)
		{
			ui8 m = readUI8();
			for(int bit = 0; bit < 8; ++bit)
			{
				if(m & (1 << bit))
				{
					nt->builtBuildings.insert(byte * 8 + bit);
				}
			}
		}

		// Forbidden buildings
		for(int byte = 6; byte < 12; ++byte)
		{
			ui8 m = readUI8();
			for(int bit = 0; bit < 8; ++bit)
			{
				if(m & (1 << bit))
				{
					nt->forbiddenBuildings.insert((byte - 6) * 8 + bit);
				}
			}
		}
		nt->builtBuildings = convertBuildings(nt->builtBuildings, castleID);
		nt->forbiddenBuildings = convertBuildings(nt->forbiddenBuildings, castleID);
	}
	// Standard buildings
	else
	{
		if(readBool())
		{
			// Has fort
			nt->builtBuildings.insert(EBuilding::FORT);
		}

		//means that set of standard building should be included
		nt->builtBuildings.insert(-50);
	}

	if(map->version > EMapFormat::ROE)
	{
		for(int i = 0; i < 9; ++i)
		{
			ui8 c = readUI8();
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
		ui8 c = readUI8();
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
	int numberOfEvent = readUI32();

	for(int gh = 0; gh < numberOfEvent; ++gh)
	{
		CCastleEvent * nce = new CCastleEvent();
		nce->town = nt;
		nce->name = readString();
		nce->message = readString();

		readResourses(nce->resources);

		nce->players = readUI8();
		if(map->version > EMapFormat::AB)
		{
			nce->humanAffected = readUI8();
		}
		else
		{
			nce->humanAffected = true;
		}

		nce->computerAffected = readUI8();
		nce->firstOccurence = readUI16();
		nce->nextOccurence =  readUI8();

		skip(17);

		// New buildings
		for(int byte = 0; byte < 6; ++byte)
		{
			ui8 m = readUI8();
			for(int bit = 0; bit < 8; ++bit)
			{
				if(m & (1 << bit))
				{
					nce->buildings.insert(byte * 8 + bit);
				}
			}
		}
		nce->buildings = convertBuildings(nce->buildings, castleID, false);

		nce->creatures.resize(7);
		for(int vv = 0; vv < 7; ++vv)
		{
			nce->creatures[vv] = readUI16();
		}
		skip(4);
		nt->events.push_back(nce);
	}

	if(map->version > EMapFormat::AB)
	{
		nt->alignment = readUI8();
	}
	skip(3);

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
		ret.insert(EBuilding::VILLAGE_HALL);
	}

	if(ret.find(EBuilding::CITY_HALL) != ret.end())
	{
		ret.insert(EBuilding::EXTRA_CITY_HALL);
	}
	if(ret.find(EBuilding::TOWN_HALL) != ret.end())
	{
		ret.insert(EBuilding::EXTRA_TOWN_HALL);
	}
	if(ret.find(EBuilding::CAPITOL) != ret.end())
	{
		ret.insert(EBuilding::EXTRA_CAPITOL);
	}

	return ret;
}

void CMapLoaderH3M::readEvents()
{
	int numberOfEvents = readUI32();
	for(int yyoo = 0; yyoo < numberOfEvents; ++yyoo)
	{
		CMapEvent * ne = new CMapEvent();
		ne->name = readString();
		ne->message = readString();

		readResourses(ne->resources);
		ne->players = readUI8();
		if(map->version > EMapFormat::AB)
		{
			ne->humanAffected = readUI8();
		}
		else
		{
			ne->humanAffected = true;
		}
		ne->computerAffected = readUI8();
		ne->firstOccurence = readUI16();
		ne->nextOccurence = readUI8();

		skip(17);

		map->events.push_back(ne);
	}
}

void CMapLoaderH3M::readSpells(std::set<TSpell>& dest)
{
	for(int byte  = 0; byte < 9; ++byte)
	{
		ui8 c = readUI8();
		for(int bit = 0; bit < 8; ++bit)
		{
			if(byte * 8 + bit < GameConstants::SPELLS_QUANTITY)
			{
				if(c & (1 << bit))
				{
					dest.insert(byte * 8 + bit);
				}
			}
		}
	}
}

void CMapLoaderH3M::readResourses(TResources& resources)
{
	resources.resize(GameConstants::RESOURCE_QUANTITY); //needed?
	for(int x = 0; x < 7; ++x)
	{
		resources[x] = readUI32();
	}
}


void CMapLoaderH3M::readBitmask(std::vector<bool>& dest, const int byteCount, const int limit, bool negate)
{
	for(int byte = 0; byte < byteCount; ++byte)
	{
		const ui8 mask = readUI8();
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
