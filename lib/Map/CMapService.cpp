#include "StdInc.h"
#include "CMapService.h"

#include "../Filesystem/CResourceLoader.h"
#include "../Filesystem/CBinaryReader.h"
#include "../Filesystem/CCompressedStream.h"
#include "../Filesystem/CMemoryStream.h"
#include "CMap.h"
#include <boost/crc.hpp>
#include "../vcmi_endian.h"
#include "../CStopWatch.h"
#include "../VCMI_Lib.h"
#include "../CSpellHandler.h"
#include "../CCreatureHandler.h"
#include "../CHeroHandler.h"
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"

std::unique_ptr<CMap> CMapService::loadMap(const std::string & name)
{
	auto stream = getStreamFromFS(name);
	return getMapLoader(stream)->loadMap();
}

std::unique_ptr<CMapHeader> CMapService::loadMapHeader(const std::string & name)
{
	auto stream = getStreamFromFS(name);
	return getMapLoader(stream)->loadMapHeader();
}

std::unique_ptr<CMap> CMapService::loadMap(const ui8 * buffer, int size)
{
	auto stream = getStreamFromMem(buffer, size);
	return getMapLoader(stream)->loadMap();
}

std::unique_ptr<CMapHeader> CMapService::loadMapHeader(const ui8 * buffer, int size)
{
	auto stream = getStreamFromMem(buffer, size);
	return getMapLoader(stream)->loadMapHeader();
}

std::unique_ptr<CInputStream> CMapService::getStreamFromFS(const std::string & name)
{
	return CResourceHandler::get()->load(ResourceID(name, EResType::MAP));
}

std::unique_ptr<CInputStream> CMapService::getStreamFromMem(const ui8 * buffer, int size)
{
	return std::unique_ptr<CInputStream>(new CMemoryStream(buffer, size));
}

std::unique_ptr<IMapLoader> CMapService::getMapLoader(std::unique_ptr<CInputStream> & stream)
{
	// Read map header
	CBinaryReader reader(stream.get());
	ui32 header = reader.readUInt32();
	reader.getStream()->seek(0);

	// Check which map format is used
	// gzip header is 3 bytes only in size
	switch(header & 0xffffff)
	{
		// gzip header magic number, reversed for LE
		case 0x00088B1F:
			stream = std::unique_ptr<CInputStream>(new CCompressedStream(std::move(stream), true));
			return std::unique_ptr<IMapLoader>(new CMapLoaderH3M(stream.get()));
		case EMapFormat::WOG :
		case EMapFormat::AB  :
		case EMapFormat::ROE :
		case EMapFormat::SOD :
			return std::unique_ptr<IMapLoader>(new CMapLoaderH3M(stream.get()));
		default :
			throw std::runtime_error("Unknown map format");
	}
}

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
	mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader);
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
		addBlockVisibleTiles(map->objects[f]);
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
	mapHeader->version = (EMapFormat::EMapFormat)(read_le_u32(buffer + pos));
	pos += 4;
	if(mapHeader->version != EMapFormat::ROE && mapHeader->version != EMapFormat::AB && mapHeader->version != EMapFormat::SOD
			&& mapHeader->version != EMapFormat::WOG)
	{
		throw std::runtime_error("Invalid map format!");
	}

	// Read map name, description, dimensions,...
	mapHeader->areAnyPlayers = static_cast<bool>(readChar(buffer, pos));
	mapHeader->height = mapHeader->width = (read_le_u32(buffer + pos));
	pos += 4;
	mapHeader->twoLevel = static_cast<bool>(readChar(buffer, pos));
	mapHeader->name = readString(buffer, pos);
	mapHeader->description = readString(buffer, pos);
	mapHeader->difficulty = readChar(buffer, pos);
	if(mapHeader->version != EMapFormat::ROE)
	{
		mapHeader->levelLimit = readChar(buffer, pos);
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
	mapHeader->players.resize(8);
	for(int i = 0; i < 8; ++i)
	{
		mapHeader->players[i].canHumanPlay = static_cast<bool>(buffer[pos++]);
		mapHeader->players[i].canComputerPlay = static_cast<bool>(buffer[pos++]);

		// If nobody can play with this player
		if((!(mapHeader->players[i].canHumanPlay || mapHeader->players[i].canComputerPlay)))
		{
			switch(mapHeader->version)
			{
			case EMapFormat::SOD:
			case EMapFormat::WOG:
				pos += 13;
				break;
			case EMapFormat::AB:
				pos += 12;
				break;
			case EMapFormat::ROE:
				pos += 6;
				break;
			}
			continue;
		}

		mapHeader->players[i].aiTactic = static_cast<EAiTactic::EAiTactic>(buffer[pos++]);

		if(mapHeader->version == EMapFormat::SOD || mapHeader->version == EMapFormat::WOG)
		{
			mapHeader->players[i].p7 = buffer[pos++];
		}
		else
		{
			mapHeader->players[i].p7 = -1;
		}

		// Factions this player can choose
		ui16 allowedFactions = buffer[pos++];
		// How many factions will be read from map
		ui16 totalFactions = GameConstants::F_NUMBER;

		if(mapHeader->version != EMapFormat::ROE)
			allowedFactions += (buffer[pos++]) * 256;
		else
			totalFactions--; //exclude conflux for ROE

		for(int fact = 0; fact < totalFactions; ++fact)
		{
			if(!(allowedFactions & (1 << fact)))
			{
				mapHeader->players[i].allowedFactions.erase(fact);
			}
		}

		mapHeader->players[i].isFactionRandom = static_cast<bool>(buffer[pos++]);
		mapHeader->players[i].hasMainTown = static_cast<bool>(buffer[pos++]);
		if(mapHeader->players[i].hasMainTown)
		{
			if(mapHeader->version != EMapFormat::ROE)
			{
				mapHeader->players[i].generateHeroAtMainTown = static_cast<bool>(buffer[pos++]);
				mapHeader->players[i].generateHero = static_cast<bool>(buffer[pos++]);
			}
			else
			{
				mapHeader->players[i].generateHeroAtMainTown = true;
				mapHeader->players[i].generateHero = false;
			}

			mapHeader->players[i].posOfMainTown.x = buffer[pos++];
			mapHeader->players[i].posOfMainTown.y = buffer[pos++];
			mapHeader->players[i].posOfMainTown.z = buffer[pos++];
		}

		mapHeader->players[i].hasHero = buffer[pos++];
		mapHeader->players[i].customHeroID = buffer[pos++];

		if(mapHeader->players[i].customHeroID != 0xff)
		{
			mapHeader->players[i].mainHeroPortrait = buffer[pos++];
			if (mapHeader->players[i].mainHeroPortrait == 0xff)
				mapHeader->players[i].mainHeroPortrait = -1; //correct 1-byte -1 (0xff) into 4-byte -1

			mapHeader->players[i].mainHeroName = readString(buffer, pos);
		}
		else
			mapHeader->players[i].customHeroID = -1; //correct 1-byte -1 (0xff) into 4-byte -1

		if(mapHeader->version != EMapFormat::ROE)
		{
			mapHeader->players[i].powerPlaceholders = buffer[pos++]; //unknown byte
			int heroCount = buffer[pos++];
			pos += 3;
			for(int pp = 0; pp < heroCount; ++pp)
			{
				SHeroName vv;
				vv.heroId = buffer[pos++];
				int hnl = buffer[pos++];
				pos += 3;
				for(int zz = 0; zz < hnl; ++zz)
				{
					vv.heroName += buffer[pos++];
				}
				mapHeader->players[i].heroesNames.push_back(vv);
			}
		}
	}
}

void CMapLoaderH3M::readVictoryLossConditions()
{
	mapHeader->victoryCondition.obj = nullptr;
	mapHeader->victoryCondition.condition = (EVictoryConditionType::EVictoryConditionType)buffer[pos++];

	// Specific victory conditions
	if(mapHeader->victoryCondition.condition != EVictoryConditionType::WINSTANDARD)
	{
		// Read victory conditions
		int nr = 0;
		switch(mapHeader->victoryCondition.condition)
		{
		case EVictoryConditionType::ARTIFACT:
			{
				mapHeader->victoryCondition.objectId = buffer[pos + 2];
				nr = (mapHeader->version == EMapFormat::ROE ? 1 : 2);
				break;
			}
		case EVictoryConditionType::GATHERTROOP:
			{
				mapHeader->victoryCondition.objectId = buffer[pos + 2];
				mapHeader->victoryCondition.count = read_le_u32(buffer + pos + (mapHeader->version == EMapFormat::ROE ? 3 : 4));
				nr = (mapHeader->version == EMapFormat::ROE ? 5 : 6);
				break;
			}
		case EVictoryConditionType::GATHERRESOURCE:
			{
				mapHeader->victoryCondition.objectId = buffer[pos + 2];
				mapHeader->victoryCondition.count = read_le_u32(buffer + pos + 3);
				nr = 5;
				break;
			}
		case EVictoryConditionType::BUILDCITY:
			{
				mapHeader->victoryCondition.pos.x = buffer[pos + 2];
				mapHeader->victoryCondition.pos.y = buffer[pos + 3];
				mapHeader->victoryCondition.pos.z = buffer[pos + 4];
				mapHeader->victoryCondition.count = buffer[pos + 5];
				mapHeader->victoryCondition.objectId = buffer[pos + 6];
				nr = 5;
				break;
			}
		case EVictoryConditionType::BUILDGRAIL:
			{
				if(buffer[pos + 4] > 2)
				{
					mapHeader->victoryCondition.pos = int3(-1,-1,-1);
				}
				else
				{
					mapHeader->victoryCondition.pos.x = buffer[pos + 2];
					mapHeader->victoryCondition.pos.y = buffer[pos + 3];
					mapHeader->victoryCondition.pos.z = buffer[pos + 4];
				}
				nr = 3;
				break;
			}
		case EVictoryConditionType::BEATHERO:
		case EVictoryConditionType::CAPTURECITY:
		case EVictoryConditionType::BEATMONSTER:
			{
				mapHeader->victoryCondition.pos.x = buffer[pos + 2];
				mapHeader->victoryCondition.pos.y = buffer[pos + 3];
				mapHeader->victoryCondition.pos.z = buffer[pos + 4];
				nr = 3;
				break;
			}
		case EVictoryConditionType::TAKEDWELLINGS:
		case EVictoryConditionType::TAKEMINES:
			{
				nr = 0;
				break;
			}
		case EVictoryConditionType::TRANSPORTITEM:
			{
				mapHeader->victoryCondition.objectId =  buffer[pos + 2];
				mapHeader->victoryCondition.pos.x = buffer[pos + 3];
				mapHeader->victoryCondition.pos.y = buffer[pos + 4];
				mapHeader->victoryCondition.pos.z = buffer[pos + 5];
				nr = 4;
				break;
			}
		default:
			assert(0);
		}
		mapHeader->victoryCondition.allowNormalVictory = static_cast<bool>(buffer[pos++]);
		mapHeader->victoryCondition.appliesToAI = static_cast<bool>(buffer[pos++]);
		pos += nr;
	}

	// Read loss conditions
	mapHeader->lossCondition.typeOfLossCon = (ELossConditionType::ELossConditionType)buffer[pos++];
	switch(mapHeader->lossCondition.typeOfLossCon)
	{
	case ELossConditionType::LOSSCASTLE:
	case ELossConditionType::LOSSHERO:
		{
			mapHeader->lossCondition.pos.x = buffer[pos++];
			mapHeader->lossCondition.pos.y = buffer[pos++];
			mapHeader->lossCondition.pos.z = buffer[pos++];
			break;
		}
	case ELossConditionType::TIMEEXPIRES:
		{
			mapHeader->lossCondition.timeLimit = read_le_u16(buffer + pos);
			pos += 2;
			break;
		}
	}
}

void CMapLoaderH3M::readTeamInfo()
{
	mapHeader->howManyTeams = buffer[pos++];
	if(mapHeader->howManyTeams > 0)
	{
		// Teams
		for(int i = 0; i < 8; ++i)
		{
			mapHeader->players[i].team = buffer[pos++];
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
	int pom = pos;
	mapHeader->allowedHeroes.resize(VLC->heroh->heroes.size(), true);
	for(; pos < pom + (mapHeader->version == EMapFormat::ROE ? 16 : 20) ; ++pos)
	{
		ui8 c = buffer[pos];
		for(int yy = 0; yy < 8; ++yy)
		{
			if((pos - pom) * 8 + yy < GameConstants::HEROES_QUANTITY)
			{
				if(c != (c | static_cast<ui8>(std::pow(2., yy))))
				{
					mapHeader->allowedHeroes[(pos - pom) * 8 + yy] = false;
				}
			}
		}
	}

	// Probably reserved for further heroes
	if(mapHeader->version > EMapFormat::ROE)
	{
		int placeholdersQty = read_le_u32(buffer + pos);
		pos += 4;
		for(int p = 0; p < placeholdersQty; ++p)
		{
			mapHeader->placeholdedHeroes.push_back(buffer[pos++]);
		}
	}
}

void CMapLoaderH3M::readDisposedHeroes()
{
	// Reading disposed heroes (20 bytes)
	ui8 disp = 0;
	if(map->version >= EMapFormat::SOD)
	{
		disp = buffer[pos++];
		map->disposedHeroes.resize(disp);
		for(int g = 0; g < disp; ++g)
		{
			map->disposedHeroes[g].heroId = buffer[pos++];
			map->disposedHeroes[g].portrait = buffer[pos++];
			int lenbuf = read_le_u32(buffer + pos);
			pos += 4;
			for(int zz = 0; zz < lenbuf; zz++)
			{
				map->disposedHeroes[g].name += buffer[pos++];
			}
			map->disposedHeroes[g].players = buffer[pos++];
		}
	}

	//omitting NULLS
	pos += 31;
}

void CMapLoaderH3M::readAllowedArtifacts()
{
	map->allowedArtifact.resize (VLC->arth->artifacts.size()); //handle new artifacts, make them allowed by default
	for (ui32 x = 0; x < map->allowedArtifact.size(); ++x)
	{
		map->allowedArtifact[x] = true;
	}

	// Reading allowed artifacts:  17 or 18 bytes
	if(map->version != EMapFormat::ROE)
	{
		// Starting i for loop
		int ist = pos;
		for(; pos < ist + (map->version == EMapFormat::AB ? 17 : 18); ++pos)
		{
			ui8 c = buffer[pos];
			for(int yy = 0; yy < 8; ++yy)
			{
				if((pos - ist) * 8 + yy < GameConstants::ARTIFACTS_QUANTITY)
				{
					if(c == (c | static_cast<ui8>(std::pow(2., yy))))
					{
						map->allowedArtifact[(pos - ist) * 8 + yy] = false;
					}
				}
			}
		}
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
	map->allowedSpell.resize(GameConstants::SPELLS_QUANTITY);
	for(ui32 x = 0; x < map->allowedSpell.size(); x++)
	{
		map->allowedSpell[x] = true;
	}

	// Read allowed abilities
	map->allowedAbilities.resize(GameConstants::SKILL_QUANTITY);
	for(ui32 x = 0; x < map->allowedAbilities.size(); x++)
	{
		map->allowedAbilities[x] = true;
	}

	if(map->version >= EMapFormat::SOD)
	{
		// Reading allowed spells (9 bytes)
		int ist = pos;
		for(; pos < ist + 9; ++pos)
		{
			ui8 c = buffer[pos];
			for(int yy = 0; yy < 8; ++yy)
			{
				if((pos - ist) * 8 + yy < GameConstants::SPELLS_QUANTITY)
				{
					if(c == (c | static_cast<ui8>(std::pow(2., yy))))
					{
						map->allowedSpell[(pos - ist) * 8 + yy] = false;
					}
				}
			}
		}


		// Allowed hero's abilities (4 bytes)
		ist = pos;
		for(; pos < ist + 4; ++pos)
		{
			ui8 c = buffer[pos];
			for(int yy = 0; yy < 8; ++yy)
			{
				if((pos - ist) * 8 + yy < GameConstants::SKILL_QUANTITY)
				{
					if(c == (c | static_cast<ui8>(std::pow(2., yy))))
					{
						map->allowedAbilities[(pos - ist) * 8 + yy] = false;
					}
				}
			}
		}
	}
}

void CMapLoaderH3M::readRumors()
{
	int rumNr = read_le_u32(buffer + pos);
	pos += 4;

	for(int it = 0; it < rumNr; it++)
	{
		Rumor ourRumor;

		// Read rumor name and text
		int nameL = read_le_u32(buffer + pos);
		pos += 4;
		for(int zz = 0; zz < nameL; zz++)
		{
			ourRumor.name += buffer[pos++];
		}

		nameL = read_le_u32(buffer + pos);
		pos += 4;
		for(int zz = 0; zz < nameL; zz++)
		{
			ourRumor.text += buffer[pos++];
		}

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
				int custom =  buffer[pos++];
				if(!custom) continue;

				CGHeroInstance * hero = new CGHeroInstance();
				hero->ID = Obj::HERO;
				hero->subID = z;

				// True if hore's experience is greater than 0
				if(readChar(buffer, pos))
				{
					hero->exp = read_le_u32(buffer + pos);
					pos += 4;
				}
				else
				{
					hero->exp = 0;
				}

				// True if hero has specified abilities
				if(readChar(buffer, pos))
				{
					int howMany = read_le_u32(buffer + pos);
					pos += 4;
					hero->secSkills.resize(howMany);
					for(int yy = 0; yy < howMany; ++yy)
					{
						hero->secSkills[yy].first = buffer[pos];
						++pos;
						hero->secSkills[yy].second = buffer[pos];
						++pos;
					}
				}

				loadArtifactsOfHero(hero);

				// custom bio
				if(readChar(buffer, pos))
				{
					hero->biography = readString(buffer, pos);
				}

				// 0xFF is default, 00 male, 01 female
				hero->sex = buffer[pos++];

				// are spells
				if(readChar(buffer, pos))
				{
					int ist = pos;
					for(; pos < ist + 9; ++pos)
					{
						ui8 c = buffer[pos];
						for(int yy = 0; yy < 8; ++yy)
						{
							if((pos - ist) * 8 + yy < GameConstants::SPELLS_QUANTITY)
							{
								if(c == (c | static_cast<ui8>(std::pow(2., yy))))
								{
									hero->spells.insert((pos - ist) * 8 + yy);
								}
							}
						}
					}
				}

				// customPrimSkills
				if(readChar(buffer, pos))
				{
					for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; xx++)
					{
						hero->pushPrimSkill(xx, buffer[pos++]);
					}
				}
				map->predefinedHeroes.push_back(hero);
			}
			break;
		}
	case EMapFormat::ROE:
		pos+=0;
		break;
	}
}

void CMapLoaderH3M::loadArtifactsOfHero(CGHeroInstance * hero)
{
	bool artSet = buffer[pos];
	++pos;

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
			++pos;
		}

		// bag artifacts //20
		// number of artifacts in hero's bag
		int amount = read_le_u16(buffer + pos);
		pos += 2;
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
		aid = buffer[pos];
		pos++;
	}
	else
	{
		aid = read_le_u16(buffer + pos);
		pos += 2;
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

	addNewArtifactInstance(a);

	//TODO make it nicer
	if(a->artType && a->artType->constituents)
	{
		CCombinedArtifactInstance * comb = dynamic_cast<CCombinedArtifactInstance *>(a);
		BOOST_FOREACH(CCombinedArtifactInstance::ConstituentInfo & ci, comb->constituentsInfo)
		{
			addNewArtifactInstance(ci.art);
		}
	}

	return a;
}

void CMapLoaderH3M::addNewArtifactInstance(CArtifactInstance * art)
{
	art->id = map->artInstances.size();
	map->artInstances.push_back(art);
}

void CMapLoaderH3M::readTerrain()
{
	// Allocate memory for terrain data
	map->terrain = new TerrainTile**[map->width];
	for(int ii = 0; ii < map->width; ii++)
	{
		map->terrain[ii] = new TerrainTile*[map->height];
		for(int jj = 0; jj < map->height; jj++)
		{
			map->terrain[ii][jj] = new TerrainTile[map->twoLevel ? 2 : 1];
		}
	}

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
				map->terrain[z][c][a].terType = static_cast<ETerrainType::ETerrainType>(buffer[pos++]);
				map->terrain[z][c][a].terView = buffer[pos++];
				map->terrain[z][c][a].riverType = static_cast<ERiverType::ERiverType>(buffer[pos++]);
				map->terrain[z][c][a].riverDir = buffer[pos++];
				map->terrain[z][c][a].roadType = static_cast<ERoadType::ERoadType>(buffer[pos++]);
				map->terrain[z][c][a].roadDir = buffer[pos++];
				map->terrain[z][c][a].extTileFlags = buffer[pos++];
				map->terrain[z][c][a].blocked = (map->terrain[z][c][a].terType == ETerrainType::ROCK ? 1 : 0); //underground tiles are always blocked
				map->terrain[z][c][a].visitable = 0;
			}
		}
	}
}

void CMapLoaderH3M::readDefInfo()
{
	int defAmount = read_le_u32(buffer + pos);
	pos += 4;
	map->customDefs.reserve(defAmount + 8);

	// Read custom defs
	for(int idd = 0; idd < defAmount; ++idd)
	{
		CGDefInfo * defInfo = new CGDefInfo();

		// Read name
		int nameLength = read_le_u32(buffer + pos);
		pos += 4;
		defInfo->name.reserve(nameLength);
		for(int cd = 0; cd < nameLength; ++cd)
		{
			defInfo->name += buffer[pos++];
		}
		std::transform(defInfo->name.begin(),defInfo->name.end(),defInfo->name.begin(),(int(*)(int))toupper);

		ui8 bytes[12];
		for(int v = 0; v < 12; ++v)
		{
			bytes[v] = buffer[pos++];
		}

		defInfo->terrainAllowed = read_le_u16(buffer + pos);
		pos += 2;
		defInfo->terrainMenu = read_le_u16(buffer + pos);
		pos += 2;
		defInfo->id = read_le_u32(buffer + pos);
		pos += 4;
		defInfo->subid = read_le_u32(buffer + pos);
		pos += 4;
		defInfo->type = buffer[pos++];
		defInfo->printPriority = buffer[pos++];
		for(int zi = 0; zi < 6; ++zi)
		{
			defInfo->blockMap[zi] = reverse(bytes[zi]);
		}
		for(int zi = 0; zi < 6; ++zi)
		{
			defInfo->visitMap[zi] = reverse(bytes[6 + zi]);
		}
		pos += 16;
		if(defInfo->id != Obj::HERO && defInfo->id != 70)
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
		map->customDefs.push_back(VLC->dobjinfo->gobjs[124][i]);
	}
}

void CMapLoaderH3M::readObjects()
{
	int howManyObjs = read_le_u32(buffer + pos);
	pos += 4;

	for(int ww = 0; ww < howManyObjs; ++ww)
	{
		CGObjectInstance * nobj = 0;

		int3 objPos;
		objPos.x = buffer[pos++];
		objPos.y = buffer[pos++];
		objPos.z = buffer[pos++];

		int defnum = read_le_u32(buffer + pos);
		pos += 4;
		int idToBeGiven = map->objects.size();

		CGDefInfo * defInfo = map->customDefs.at(defnum);
		pos += 5;

		switch(defInfo->id)
		{
		case Obj::EVENT:
			{
				CGEvent * evnt = new CGEvent();
				nobj = evnt;

				bool guardMess = buffer[pos];
				++pos;
				if(guardMess)
				{
					int messLong = read_le_u32(buffer + pos);
					pos += 4;
					if(messLong > 0)
					{
						for(int yy = 0; yy < messLong; ++yy)
						{
							evnt->message += buffer[pos + yy];
						}
						pos += messLong;
					}
					if(buffer[pos++])
					{
						readCreatureSet(evnt, 7, map->version > EMapFormat::ROE);
					}
					pos += 4;
				}
				evnt->gainedExp = read_le_u32(buffer + pos);
				pos += 4;
				evnt->manaDiff = read_le_u32(buffer + pos);
				pos += 4;
				evnt->moraleDiff = static_cast<char>(buffer[pos]);
				++pos;
				evnt->luckDiff = static_cast<char>(buffer[pos]);
				++pos;

				evnt->resources.resize(GameConstants::RESOURCE_QUANTITY);
				for(int x = 0; x < 7; ++x)
				{
					evnt->resources[x] = read_le_u32(buffer + pos);
					pos += 4;
				}

				evnt->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x = 0; x < 4; ++x)
				{
					evnt->primskills[x] = buffer[pos];
					++pos;
				}

				int gabn; // Number of gained abilities
				gabn = buffer[pos];
				++pos;
				for(int oo = 0; oo < gabn; ++oo)
				{
					evnt->abilities.push_back(buffer[pos]);
					++pos;
					evnt->abilityLevels.push_back(buffer[pos]);
					++pos;
				}

				int gart = buffer[pos]; // Number of gained artifacts
				++pos;
				for(int oo = 0; oo < gart; ++oo)
				{
					if(map->version == EMapFormat::ROE)
					{
						evnt->artifacts.push_back(buffer[pos]);
						++pos;
					}
					else
					{
						evnt->artifacts.push_back(read_le_u16(buffer + pos));
						pos += 2;
					}
				}

				int gspel = buffer[pos]; // Number of gained spells
				++pos;
				for(int oo = 0; oo < gspel; ++oo)
				{
					evnt->spells.push_back(buffer[pos]);
					++pos;
				}

				int gcre = buffer[pos]; //number of gained creatures
				++pos;
				readCreatureSet(&evnt->creatures, gcre, map->version > EMapFormat::ROE);

				pos += 8;
				evnt->availableFor = buffer[pos];
				++pos;
				evnt->computerActivate = buffer[pos];
				++pos;
				evnt->removeAfterVisit = buffer[pos];
				++pos;
				evnt->humanActivate = true;

				pos += 4;
				break;
			}
		case 34: case 70: case 62: //34 - hero; 70 - random hero; 62 - prison
			{
				nobj = readHero(idToBeGiven);
				break;
			}
		case 4: //Arena
		case 51: //Mercenary Camp
		case 23: //Marletto Tower
		case 61: // Star Axis
		case 32: // Garden of Revelation
		case 100: //Learning Stone
		case 102: //Tree of Knowledge
		case 41: //Library of Enlightenment
		case 47: //School of Magic
		case 107: //School of War
			{
				nobj = new CGVisitableOPH();
				break;
			}
		case 55: //mystical garden
		case 112://windmill
		case 109://water wheel
			{
				nobj = new CGVisitableOPW();
				break;
			}
		case 43: //teleport
		case 44: //teleport
		case 45: //teleport
		case 103://subterranean gate
		case 111://Whirlpool
			{
				nobj = new CGTeleport();
				break;
			}
		case 12: //campfire
		case 29: //Flotsam
		case 82: //Sea Chest
		case 86: //Shipwreck Survivor
		case 101://treasure chest
			{
				nobj = new CGPickable();
				break;
			}
		case 54:  //Monster
		case 71: case 72: case 73: case 74: case 75:	// Random Monster 1 - 4
		case 162: case 163: case 164:					// Random Monster 5 - 7
			{
				CGCreature * cre = new CGCreature();
				nobj = cre;

				if(map->version > EMapFormat::ROE)
				{
					cre->identifier = read_le_u32(buffer + pos);
					pos += 4;
					map->questIdentifierToId[cre->identifier] = idToBeGiven;
				}

				CStackInstance * hlp = new CStackInstance();
				hlp->count = read_le_u16(buffer + pos);
				pos += 2;

				//type will be set during initialization
				cre->putStack(0, hlp);

				cre->character = buffer[pos];
				++pos;
				bool isMesTre = buffer[pos]; //true if there is message or treasury
				++pos;
				if(isMesTre)
				{
					cre->message = readString(buffer, pos);
					cre->resources.resize(GameConstants::RESOURCE_QUANTITY);
					for(int j = 0; j < 7; ++j)
					{
						cre->resources[j] = read_le_u32(buffer + pos);
						pos += 4;
					}

					int artID;
					if (map->version == EMapFormat::ROE)
					{
						artID = buffer[pos];
						++pos;
					}
					else
					{
						artID = read_le_u16(buffer + pos);
						pos += 2;
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
				cre->neverFlees = buffer[pos];
				++pos;
				cre->notGrowingTeam = buffer[pos];
				++pos;
				pos += 2;
				break;
			}
		case 59: case 91: //ocean bottle and sign
			{
				CGSignBottle * sb = new CGSignBottle();
				nobj = sb;
				sb->message = readString(buffer, pos);
				pos += 4;
				break;
			}
		case 83: //seer's hut
			{
				nobj = readSeerHut();
				addQuest(nobj);
				break;
			}
		case 113: //witch hut
			{
				CGWitchHut * wh = new CGWitchHut();
				nobj = wh;

				// in reo we cannot specify it - all are allowed (I hope)
				if(map->version > EMapFormat::ROE)
				{
					int ist=pos;
					for(; pos < ist + 4; ++pos)
					{
						ui8 c = buffer[pos];
						for(int yy = 0; yy < 8; ++yy)
						{
							if((pos - ist) * 8 + yy < GameConstants::SKILL_QUANTITY)
							{
								if(c == (c | static_cast<ui8>(std::pow(2., yy))))
								{
									wh->allowedAbilities.push_back((pos - ist) * 8 + yy);
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
		case 81: //scholar
			{
				CGScholar * sch = new CGScholar();
				nobj = sch;
				sch->bonusType = buffer[pos++];
				sch->bonusID = buffer[pos++];
				pos += 6;
				break;
			}
		case 33: case 219: //garrison
			{
				CGGarrison * gar = new CGGarrison();
				nobj = gar;
				nobj->setOwner(buffer[pos++]);
				pos += 3;
				readCreatureSet(gar, 7, map->version > EMapFormat::ROE);
				if(map->version > EMapFormat::ROE)
				{
					gar->removableUnits = buffer[pos];
					++pos;
				}
				else
				{
					gar->removableUnits = true;
				}
				pos += 8;
				break;
			}
		case 5: //artifact
		case 65: case 66: case 67: case 68: case 69: //random artifact
		case 93: //spell scroll
			{
				int artID = -1;
				int spellID = -1;
				CGArtifact * art = new CGArtifact();
				nobj = art;

				bool areSettings = buffer[pos++];
				if(areSettings)
				{
					art->message = readString(buffer, pos);
					bool areGuards = buffer[pos++];
					if(areGuards)
					{
						readCreatureSet(art, 7, map->version > EMapFormat::ROE);
					}
					pos += 4;
				}

				if(defInfo->id == 93)
				{
					spellID = read_le_u32(buffer + pos);
					pos += 4;
					artID = 1;
				}
				else if(defInfo->id == 5)
				{
					//specific artifact
					artID = defInfo->subid;
				}

				art->storedArtifact = createArtifact(artID, spellID);
				break;
			}
		case 76: case 79: //random resource; resource
			{
				CGResource * res = new CGResource();
				nobj = res;

				bool isMessGuard = buffer[pos];
				++pos;
				if(isMessGuard)
				{
					res->message = readString(buffer, pos);
					if(buffer[pos++])
					{
						readCreatureSet(res, 7, map->version > EMapFormat::ROE);
					}
					pos += 4;
				}
				res->amount = read_le_u32(buffer + pos);
				pos += 4;
				if(defInfo->subid == 6)
				{
					// Gold is multiplied by 100.
					res->amount *= 100;
				}
				pos += 4;

				break;
			}
		case 77: case 98: //random town; town
			{
				nobj = readTown(defInfo->subid);
				break;
			}
		case 53:
		case 220://mine (?)
			{
				nobj = new CGMine();
				nobj->setOwner(buffer[pos++]);
				pos += 3;
				break;
			}
		case 17: case 18: case 19: case 20: //dwellings
			{
				nobj = new CGDwelling();
				nobj->setOwner(buffer[pos++]);
				pos += 3;
				break;
			}
		case 78: //Refugee Camp
		case 106: //War Machine Factory
			{
				nobj = new CGDwelling();
				break;
			}
		case 88: case 89: case 90: //spell shrine
			{
				CGShrine * shr = new CGShrine();
				nobj = shr;
				shr->spell = buffer[pos];
				pos += 4;
				break;
			}
		case 6: //pandora's box
			{
				CGPandoraBox * box = new CGPandoraBox();
				nobj = box;
				bool messg = buffer[pos];
				++pos;
				if(messg)
				{
					box->message = readString(buffer, pos);
					if(buffer[pos++])
					{
						readCreatureSet(box, 7, map->version > EMapFormat::ROE);
					}
					pos += 4;
				}

				box->gainedExp = read_le_u32(buffer + pos);
				pos += 4;
				box->manaDiff = read_le_u32(buffer + pos);
				pos += 4;
				box->moraleDiff = static_cast<si8>(buffer[pos]);
				++pos;
				box->luckDiff = static_cast<si8>(buffer[pos]);
				++pos;

				box->resources.resize(GameConstants::RESOURCE_QUANTITY);
				for(int x = 0; x < 7; ++x)
				{
					box->resources[x] = read_le_u32(buffer + pos);
					pos += 4;
				}

				box->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x = 0; x < 4; ++x)
				{
					box->primskills[x] = buffer[pos];
					++pos;
				}

				int gabn; //number of gained abilities
				gabn = buffer[pos];
				++pos;
				for(int oo = 0; oo < gabn; ++oo)
				{
					box->abilities.push_back(buffer[pos]);
					++pos;
					box->abilityLevels.push_back(buffer[pos]);
					++pos;
				}
				int gart = buffer[pos]; //number of gained artifacts
				++pos;
				for(int oo = 0; oo < gart; ++oo)
				{
					if(map->version > EMapFormat::ROE)
					{
						box->artifacts.push_back(read_le_u16(buffer + pos));
						pos += 2;
					}
					else
					{
						box->artifacts.push_back(buffer[pos]);
						++pos;
					}
				}
				int gspel = buffer[pos]; //number of gained spells
				++pos;
				for(int oo = 0; oo < gspel; ++oo)
				{
					box->spells.push_back(buffer[pos]);
					++pos;
				}
				int gcre = buffer[pos]; //number of gained creatures
				++pos;
				readCreatureSet(&box->creatures, gcre, map->version > EMapFormat::ROE);
				pos += 8;
				break;
			}
		case 36: //grail
			{
				map->grailPos = objPos;
				map->grailRadious = read_le_u32(buffer + pos);
				pos += 4;
				continue;
			}
		//dwellings
		case 216: //same as castle + level range
		case 217: //same as castle
		case 218: //level range
			{
				nobj = new CGDwelling();
				CSpecObjInfo * spec = nullptr;
				switch(defInfo->id)
				{
					break; case 216: spec = new CCreGenLeveledCastleInfo();
					break; case 217: spec = new CCreGenAsCastleInfo();
					break; case 218: spec = new CCreGenLeveledInfo();
				}

				spec->player = read_le_u32(buffer + pos);
				pos += 4;

				//216 and 217
				if (auto castleSpec = dynamic_cast<CCreGenAsCastleInfo *>(spec))
				{
					castleSpec->identifier =  read_le_u32(buffer + pos);
					pos += 4;
					if(!castleSpec->identifier)
					{
						castleSpec->asCastle = false;
						castleSpec->castles[0] = buffer[pos];
						++pos;
						castleSpec->castles[1] = buffer[pos];
						++pos;
					}
					else
					{
						castleSpec->asCastle = true;
					}
				}

				//216 and 218
				if (auto lvlSpec = dynamic_cast<CCreGenLeveledInfo *>(spec))
				{
					lvlSpec->minLevel = std::max(buffer[pos], ui8(1));
					++pos;
					lvlSpec->maxLevel = std::min(buffer[pos], ui8(7));
					++pos;
				}
				nobj->setOwner(spec->player);
				static_cast<CGDwelling *>(nobj)->info = spec;
				break;
			}
		case 215:
			{
				CGQuestGuard * guard = new CGQuestGuard();
				addQuest(guard);
				readQuest(guard);
				nobj = guard;
				break;
			}
		case 28: //faerie ring
		case 14: //Swan pond
		case 38: //idol of fortune
		case 30: //Fountain of Fortune
		case 64: //Rally Flag
		case 56: //oasis
		case 96: //temple
		case 110://Watering Hole
		case 31: //Fountain of Youth
		case 11: //Buoy
		case 52: //Mermaid
		case 94: //Stables
			{
				nobj = new CGBonusingObject();
				break;
			}
		case 49: //Magic Well
			{
				nobj = new CGMagicWell();
				break;
			}
		case 15: //Cover of darkness
		case 58: //Redwood Observatory
		case 60: //Pillar of Fire
			{
				nobj = new CGObservatory();
				break;
			}
		case 22: //Corpse
		case 39: //Lean To
		case 105://Wagon
		case 108://Warrior's Tomb
			{
				nobj = new CGOnceVisitable();
				break;
			}
		case 8: //Boat
			{
				nobj = new CGBoat();
				break;
			}
		case 92: //Sirens
			{
				nobj = new CGSirens();
				break;
			}
		case 87: //Shipyard
			{
				nobj = new CGShipyard();
				nobj->setOwner(read_le_u32(buffer + pos));
				pos += 4;
				break;
			}
		case 214: //hero placeholder
			{
				CGHeroPlaceholder * hp = new CGHeroPlaceholder();
				nobj = hp;

				int a = buffer[pos++]; //unknown byte, seems to be always 0 (if not - scream!)
				tlog2 << "Unhandled Hero Placeholder detected: " << a << std::endl;

				int htid = buffer[pos++]; //hero type id
				nobj->subID = htid;

				if(htid == 0xff)
				{
					hp->power = buffer[pos++];
				}
				else
				{
					hp->power = 0;
				}

				break;
			}
		case 10: //Keymaster
			{
				nobj = new CGKeymasterTent();
				break;
			}
		case 9: //Border Guard
			{
				nobj = new CGBorderGuard();
				addQuest(nobj);
				break;
			}
		case 212: //Border Gate
			{
				nobj = new CGBorderGate();
				addQuest (nobj);
				break;
			}
		case 27: case 37: //Eye and Hut of Magi
			{
				nobj = new CGMagi();
				break;
			}
		case 16: case 24: case 25: case 84: case 85: //treasure bank
			{
				nobj = new CBank();
				break;
			}
		case 63: //Pyramid
			{
				nobj = new CGPyramid();
				break;
			}
		case 13: //Cartographer
			{
				nobj = new CCartographer();
				break;
			}
		case 48: //Magic Spring
			{
				nobj = new CGMagicSpring();
				break;
			}
		case 97: //den of thieves
			{
				nobj = new CGDenOfthieves();
				break;
			}
		case 57: //Obelisk
			{
				nobj = new CGObelisk();
				break;
			}
		case 42: //Lighthouse
			{
				nobj = new CGLighthouse();
				nobj->tempOwner = read_le_u32(buffer + pos);
				pos += 4;
				break;
			}
		case 2: //Altar of Sacrifice
		case 99: //Trading Post
		case 213: //Freelancer's Guild
		case 221: //Trading Post (snow)
			{
				nobj = new CGMarket();
				break;
			}
		case 104: //University
			{
				nobj = new CGUniversity();
				break;
			}
		case 7: //Black Market
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

void CMapLoaderH3M::readCreatureSet(CCreatureSet * out, int number, bool version)
{
	const int bytesPerCre = version ? 4 : 3;
	const int maxID = version ? 0xffff : 0xff;

	for(int ir = 0; ir < number; ++ir)
	{
		int creID;
		int count;

		if(version)
		{
			creID = read_le_u16(buffer + pos + ir * bytesPerCre);
			count = read_le_u16(buffer + pos + ir * bytesPerCre + 2);
		}
		else
		{
			creID = buffer[pos + ir * bytesPerCre];
			count = read_le_u16(buffer + pos + ir * bytesPerCre + 1);
		}

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

	pos += number * bytesPerCre;
	out->validTypes(true);
}

CGObjectInstance * CMapLoaderH3M::readHero(int idToBeGiven)
{
	CGHeroInstance * nhi = new CGHeroInstance();

	int identifier = 0;
	if(map->version > EMapFormat::ROE)
	{
		identifier = read_le_u32(buffer + pos);
		pos += 4;
		map->questIdentifierToId[identifier] = idToBeGiven;
	}

	ui8 owner = buffer[pos++];
	nhi->subID = buffer[pos++];

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
	if(readChar(buffer, pos))
	{
		nhi->name = readString(buffer, pos);
	}
	if(map->version > EMapFormat::AB)
	{
		// True if hero's experience is greater than 0
		if(readChar(buffer, pos))
		{
			nhi->exp = read_le_u32(buffer + pos);
			pos += 4;
		}
		else
		{
			nhi->exp = 0xffffffff;
		}
	}
	else
	{
		nhi->exp = read_le_u32(buffer + pos);
		pos += 4;

		//0 means "not set" in <=AB maps
		if(!nhi->exp)
		{
			nhi->exp = 0xffffffff;
		}
	}

	bool portrait = buffer[pos];
	++pos;
	if(portrait)
	{
		nhi->portrait = buffer[pos++];
	}

	// True if hero has specified abilities
	if(readChar(buffer, pos))
	{
		int howMany = read_le_u32(buffer + pos);
		pos += 4;
		nhi->secSkills.resize(howMany);
		for(int yy = 0; yy < howMany; ++yy)
		{
			nhi->secSkills[yy].first = buffer[pos++];
			nhi->secSkills[yy].second = buffer[pos++];
		}
	}

	// True if hero has nonstandard garrison
	if(readChar(buffer, pos))
	{
		readCreatureSet(nhi, 7, map->version > EMapFormat::ROE);
	}

	nhi->formation = buffer[pos];
	++pos;
	loadArtifactsOfHero(nhi);
	nhi->patrol.patrolRadious = buffer[pos];
	++pos;
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
		if(readChar(buffer, pos))
		{
			nhi->biography = readString(buffer, pos);
		}
		nhi->sex = buffer[pos];
		++pos;

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
		bool areSpells = buffer[pos];
		++pos;

		if(areSpells)
		{
			nhi->spells.insert(0xffffffff); //placeholder "preset spells"
			int ist = pos;
			for(; pos < ist + 9; ++pos)
			{
				ui8 c = buffer[pos];
				for(int yy = 0; yy < 8; ++yy)
				{
					if((pos - ist) * 8 + yy < GameConstants::SPELLS_QUANTITY)
					{
						if(c == (c | static_cast<ui8>(std::pow(2., yy))))
						{
							nhi->spells.insert((pos - ist) * 8 + yy);
						}
					}
				}
			}
		}
	}
	else if(map->version == EMapFormat::AB)
	{
		//we can read one spell
		ui8 buff = buffer[pos];
		++pos;
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
		if(readChar(buffer, pos))
		{
			for(int xx = 0; xx < GameConstants::PRIMARY_SKILLS; ++xx)
			{
				nhi->pushPrimSkill(xx, buffer[pos++]);
			}
		}
	}
	pos += 16;

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
		int artID = buffer[pos];
		++pos;
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
		ui8 rewardType = buffer[pos];
		++pos;
		hut->rewardType = rewardType;

		switch(rewardType)
		{
		case 1:
			{
				hut->rVal = read_le_u32(buffer + pos);
				pos += 4;
				break;
			}
		case 2:
			{
				hut->rVal = read_le_u32(buffer + pos);
				pos += 4;
				break;
			}
		case 3:
			{
				hut->rVal = buffer[pos];
				++pos;
				break;
			}
		case 4:
			{
				hut->rVal = buffer[pos];
				++pos;
				break;
			}
		case 5:
			{
				hut->rID = buffer[pos];
				++pos;
				// Only the first 3 bytes are used. Skip the 4th.
				hut->rVal = read_le_u32(buffer + pos) & 0x00ffffff;
				pos += 4;
				break;
			}
		case 6:
			{
				hut->rID = buffer[pos];
				++pos;
				hut->rVal = buffer[pos];
				++pos;
				break;
			}
		case 7:
			{
				hut->rID = buffer[pos];
				++pos;
				hut->rVal = buffer[pos];
				++pos;
				break;
			}
		case 8:
			{
				if (map->version == EMapFormat::ROE)
				{
					hut->rID = buffer[pos];
					++pos;
				}
				else
				{
					hut->rID = read_le_u16(buffer + pos);
					pos += 2;
				}
				break;
			}
		case 9:
			{
				hut->rID = buffer[pos];
				++pos;
				break;
			}
		case 10:
			{
				if(map->version > EMapFormat::ROE)
				{
					hut->rID = read_le_u16(buffer + pos);
					pos += 2;
					hut->rVal = read_le_u16(buffer + pos);
					pos += 2;
				}
				else
				{
					hut->rID = buffer[pos];
					++pos;
					hut->rVal = read_le_u16(buffer + pos);
					pos += 2;
				}
				break;
			}
		}
		pos += 2;
	}
	else
	{
		// missionType==255
		pos += 3;
	}

	return hut;
}

void CMapLoaderH3M::readQuest(IQuestObject * guard)
{
	guard->quest->missionType = buffer[pos];
	++pos;

	switch(guard->quest->missionType)
	{
	case 0:
		return;
	case 2:
		{
			guard->quest->m2stats.resize(4);
			for(int x = 0; x < 4; ++x)
			{
				guard->quest->m2stats[x] = buffer[pos++];
			}
		}
		break;
	case 1:
	case 3:
	case 4:
		{
			guard->quest->m13489val = read_le_u32(buffer + pos);
			pos += 4;
			break;
		}
	case 5:
		{
			int artNumber = buffer[pos];
			++pos;
			for(int yy = 0; yy < artNumber; ++yy)
			{
				int artid = read_le_u16(buffer + pos);
				pos += 2;
				guard->quest->m5arts.push_back(artid);
				map->allowedArtifact[artid] = false; //these are unavailable for random generation
			}
			break;
		}
	case 6:
		{
			int typeNumber = buffer[pos];
			++pos;
			guard->quest->m6creatures.resize(typeNumber);
			for(int hh = 0; hh < typeNumber; ++hh)
			{
				guard->quest->m6creatures[hh].type = VLC->creh->creatures[read_le_u16(buffer + pos)];
				pos += 2;
				guard->quest->m6creatures[hh].count = read_le_u16(buffer + pos);
				pos += 2;
			}
			break;
		}
	case 7:
		{
			guard->quest->m7resources.resize(7);
			for(int x = 0; x < 7; ++x)
			{
				guard->quest->m7resources[x] = read_le_u32(buffer + pos);
				pos += 4;
			}
			break;
		}
	case 8:
	case 9:
		{
			guard->quest->m13489val = buffer[pos];
			++pos;
			break;
		}
	}

	int limit = read_le_u32(buffer + pos);
	pos += 4;
	if(limit == (static_cast<int>(0xffffffff)))
	{
		guard->quest->lastDay = -1;
	}
	else
	{
		guard->quest->lastDay = limit;
	}
	guard->quest->firstVisitText = readString(buffer, pos);
	guard->quest->nextVisitText = readString(buffer, pos);
	guard->quest->completedText = readString(buffer, pos);
	guard->quest->isCustomFirst = guard->quest->firstVisitText.size() > 0;
	guard->quest->isCustomNext = guard->quest->nextVisitText.size() > 0;
	guard->quest->isCustomComplete = guard->quest->completedText.size() > 0;
}

void CMapLoaderH3M::addQuest(CGObjectInstance * quest)
{
	auto q = dynamic_cast<IQuestObject *>(quest);
	q->quest->qid = map->quests.size();
	map->quests.push_back(q->quest);
}

CGTownInstance * CMapLoaderH3M::readTown(int castleID)
{
	CGTownInstance * nt = new CGTownInstance();
	nt->identifier = 0;
	if(map->version > EMapFormat::ROE)
	{
		nt->identifier = read_le_u32(buffer + pos);
		pos += 4;
	}
	nt->tempOwner = buffer[pos];
	++pos;
	if(readChar(buffer, pos))
	{
		// Has name
		nt->name = readString(buffer, pos);
	}

	// True if garrison isn't empty
	if(readChar(buffer, pos))
	{
		readCreatureSet(nt, 7, map->version > EMapFormat::ROE);
	}
	nt->formation = buffer[pos];
	++pos;

	// Custom buildings info
	if(readChar(buffer, pos))
	{
		// Built buildings
		for(int byte = 0; byte < 6; ++byte)
		{
			for(int bit = 0; bit < 8; ++bit)
			{
				if(buffer[pos] & (1 << bit))
				{
					nt->builtBuildings.insert(byte * 8 + bit);
				}
			}
			++pos;
		}

		// Forbidden buildings
		for(int byte = 6; byte < 12; ++byte)
		{
			for(int bit = 0; bit < 8; ++bit)
			{
				if(buffer[pos] & (1 << bit))
				{
					nt->forbiddenBuildings.insert((byte - 6) * 8 + bit);
				}
			}
			++pos;
		}
		nt->builtBuildings = convertBuildings(nt->builtBuildings, castleID);
		nt->forbiddenBuildings = convertBuildings(nt->forbiddenBuildings, castleID);
	}
	// Standard buildings
	else
	{
		if(readChar(buffer, pos))
		{
			// Has fort
			nt->builtBuildings.insert(EBuilding::FORT);
		}

		//means that set of standard building should be included
		nt->builtBuildings.insert(-50);
	}

	int ist = pos;
	if(map->version > EMapFormat::ROE)
	{
		for(; pos < ist + 9; ++pos)
		{
			ui8 c = buffer[pos];
			for(int yy = 0; yy < 8; ++yy)
			{
				if((pos - ist) * 8 + yy < GameConstants::SPELLS_QUANTITY)
				{
					if(c != (c | static_cast<ui8>(std::pow(2., yy))))
					{
						nt->obligatorySpells.push_back((pos - ist) * 8 + yy);
					}
				}
			}
		}
	}

	ist = pos;
	for(; pos < ist + 9; ++pos)
	{
		ui8 c = buffer[pos];
		for(int yy = 0; yy < 8; ++yy)
		{
			if((pos - ist) * 8 + yy < GameConstants::SPELLS_QUANTITY)
			{
				if(c != (c | static_cast<ui8>(std::pow(2., yy))))
				{
					nt->possibleSpells.push_back((pos - ist) * 8 + yy);
				}
			}
		}
	}

	// Read castle events
	int numberOfEvent = read_le_u32(buffer + pos);
	pos += 4;

	for(int gh = 0; gh < numberOfEvent; ++gh)
	{
		CCastleEvent * nce = new CCastleEvent();
		nce->town = nt;
		nce->name = readString(buffer, pos);
		nce->message = readString(buffer, pos);
		for(int x = 0; x < 7; ++x)
		{
			nce->resources[x] = read_le_u32(buffer + pos);
			pos += 4;
		}

		nce->players = buffer[pos];
		++pos;
		if(map->version > EMapFormat::AB)
		{
			nce->humanAffected = buffer[pos];
			++pos;
		}
		else
		{
			nce->humanAffected = true;
		}

		nce->computerAffected = buffer[pos];
		++pos;
		nce->firstOccurence = read_le_u16(buffer + pos);
		pos += 2;
		nce->nextOccurence = buffer[pos];
		++pos;

		pos += 17;

		// New buildings
		for(int byte = 0; byte < 6; ++byte)
		{
			for(int bit = 0; bit < 8; ++bit)
			{
				if(buffer[pos] & (1 << bit))
				{
					nce->buildings.insert(byte * 8 + bit);
				}
			}
			++pos;
		}
		nce->buildings = convertBuildings(nce->buildings, castleID, false);

		nce->creatures.resize(7);
		for(int vv = 0; vv < 7; ++vv)
		{
			nce->creatures[vv] = read_le_u16(buffer + pos);
			pos += 2;
		}
		pos += 4;
		nt->events.push_back(nce);
	}

	if(map->version > EMapFormat::AB)
	{
		nt->alignment = buffer[pos];
		++pos;
	}
	else
	{
		nt->alignment = 0xff;
	}
	pos += 3;

	nt->builded = 0;
	nt->destroyed = 0;
	nt->garrisonHero = nullptr;

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
	int numberOfEvents = read_le_u32(buffer + pos);
	pos += 4;
	for(int yyoo = 0; yyoo < numberOfEvents; ++yyoo)
	{
		CMapEvent * ne = new CMapEvent();
		ne->name = std::string();
		ne->message = std::string();
		int nameLen = read_le_u32(buffer + pos);
		pos += 4;
		for(int qq = 0; qq < nameLen; ++qq)
		{
			ne->name += buffer[pos];
			++pos;
		}
		int messLen = read_le_u32(buffer + pos);
		pos += 4;
		for(int qq = 0; qq < messLen; ++qq)
		{
			ne->message +=buffer[pos];
			++pos;
		}
		for(int k = 0; k < 7; ++k)
		{
			ne->resources[k] = read_le_u32(buffer + pos);
			pos += 4;
		}
		ne->players = buffer[pos];
		++pos;
		if(map->version > EMapFormat::AB)
		{
			ne->humanAffected = buffer[pos];
			++pos;
		}
		else
		{
			ne->humanAffected = true;
		}
		ne->computerAffected = buffer[pos];
		++pos;
		ne->firstOccurence = read_le_u16(buffer + pos);
		pos += 2;
		ne->nextOccurence = buffer[pos];
		++pos;

		char unknown[17];
		memcpy(unknown, buffer + pos, 17);
		pos += 17;

		map->events.push_back(ne);
	}
}

void CMapLoaderH3M::addBlockVisibleTiles(CGObjectInstance * obj)
{
	for(int fx = 0; fx < 8; ++fx)
	{
		for(int fy = 0; fy < 6; ++fy)
		{
			int xVal = obj->pos.x + fx - 7;
			int yVal = obj->pos.y + fy - 5;
			int zVal = obj->pos.z;
			if(xVal >= 0 && xVal < map->width && yVal >= 0 && yVal < map->height)
			{
				TerrainTile & curt = map->terrain[xVal][yVal][zVal];
				if(((obj->defInfo->visitMap[fy] >> (7 - fx)) & 1))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if(!((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
				{
					curt.blockingObjects.push_back(obj);
					curt.blocked = true;
				}
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
