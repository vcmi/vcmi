#include "StdInc.h"
#include "CMapGenerator.h"

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"
#include "../filesystem/CResourceLoader.h"

CMapGenOptions::CMapGenOptions() : width(CMapHeader::MAP_SIZE_MIDDLE), height(CMapHeader::MAP_SIZE_MIDDLE), hasTwoLevels(true),
	playersCnt(RANDOM_SIZE), teamsCnt(RANDOM_SIZE), compOnlyPlayersCnt(0), compOnlyTeamsCnt(RANDOM_SIZE),
	waterContent(EWaterContent::RANDOM), monsterStrength(EMonsterStrength::RANDOM)
{
	resetPlayersMap();
}

si32 CMapGenOptions::getWidth() const
{
	return width;
}

void CMapGenOptions::setWidth(si32 value)
{
	assert(value >= 1);
	width = value;
}

si32 CMapGenOptions::getHeight() const
{
	return height;
}

void CMapGenOptions::setHeight(si32 value)
{
	assert(value >= 1);
	height = value;
}

bool CMapGenOptions::getHasTwoLevels() const
{
	return hasTwoLevels;
}

void CMapGenOptions::setHasTwoLevels(bool value)
{
	hasTwoLevels = value;
}

si8 CMapGenOptions::getPlayersCnt() const
{
	return playersCnt;
}

void CMapGenOptions::setPlayersCnt(si8 value)
{
	assert((value >= 1 && value <= PlayerColor::PLAYER_LIMIT_I) || value == RANDOM_SIZE);
	playersCnt = value;
	resetPlayersMap();
}

si8 CMapGenOptions::getTeamsCnt() const
{
	return teamsCnt;
}

void CMapGenOptions::setTeamsCnt(si8 value)
{
	assert(playersCnt == RANDOM_SIZE || (value >= 0 && value < playersCnt) || value == RANDOM_SIZE);
	teamsCnt = value;
}

si8 CMapGenOptions::getCompOnlyPlayersCnt() const
{
	return compOnlyPlayersCnt;
}

void CMapGenOptions::setCompOnlyPlayersCnt(si8 value)
{
	assert(value == RANDOM_SIZE || (value >= 0 && value <= PlayerColor::PLAYER_LIMIT_I - playersCnt));
	compOnlyPlayersCnt = value;
	resetPlayersMap();
}

si8 CMapGenOptions::getCompOnlyTeamsCnt() const
{
	return compOnlyTeamsCnt;
}

void CMapGenOptions::setCompOnlyTeamsCnt(si8 value)
{
	assert(value == RANDOM_SIZE || compOnlyPlayersCnt == RANDOM_SIZE || (value >= 0 && value <= std::max(compOnlyPlayersCnt - 1, 0)));
	compOnlyTeamsCnt = value;
}

EWaterContent::EWaterContent CMapGenOptions::getWaterContent() const
{
	return waterContent;
}

void CMapGenOptions::setWaterContent(EWaterContent::EWaterContent value)
{
	waterContent = value;
}

EMonsterStrength::EMonsterStrength CMapGenOptions::getMonsterStrength() const
{
	return monsterStrength;
}

void CMapGenOptions::setMonsterStrength(EMonsterStrength::EMonsterStrength value)
{
	monsterStrength = value;
}

void CMapGenOptions::resetPlayersMap()
{
	players.clear();
	int realPlayersCnt = playersCnt == RANDOM_SIZE ? static_cast<int>(PlayerColor::PLAYER_LIMIT_I) : playersCnt;
	int realCompOnlyPlayersCnt = compOnlyPlayersCnt == RANDOM_SIZE ? (PlayerColor::PLAYER_LIMIT_I - realPlayersCnt) : compOnlyPlayersCnt;
	for(int color = 0; color < (realPlayersCnt + realCompOnlyPlayersCnt); ++color)
	{
		CPlayerSettings player;
		player.setColor(PlayerColor(color));
		player.setPlayerType((color >= realPlayersCnt) ? EPlayerType::COMP_ONLY : EPlayerType::AI);
		players[PlayerColor(color)] = player;
	}
}

const std::map<PlayerColor, CMapGenOptions::CPlayerSettings> & CMapGenOptions::getPlayersSettings() const
{
	return players;
}

void CMapGenOptions::setStartingTownForPlayer(PlayerColor color, si32 town)
{
	auto it = players.find(color);
	if(it == players.end()) assert(0);
	it->second.setStartingTown(town);
}

void CMapGenOptions::setPlayerTypeForStandardPlayer(PlayerColor color, EPlayerType::EPlayerType playerType)
{
	assert(playerType != EPlayerType::COMP_ONLY);
	auto it = players.find(color);
	if(it == players.end()) assert(0);
	it->second.setPlayerType(playerType);
}

void CMapGenOptions::finalize()
{
	CRandomGenerator gen;
	finalize(gen);
}

void CMapGenOptions::finalize(CRandomGenerator & gen)
{
	if(playersCnt == RANDOM_SIZE)
	{
		// 1 human is required at least
		auto humanPlayers = countHumanPlayers();
		if(humanPlayers == 0) humanPlayers = 1;
		playersCnt = gen.getInteger(humanPlayers, PlayerColor::PLAYER_LIMIT_I);

		// Remove AI players only from the end of the players map if necessary
		for(auto itrev = players.end(); itrev != players.begin();)
		{
			auto it = itrev;
			--it;
			if(players.size() == playersCnt) break;
			if(it->second.getPlayerType() == EPlayerType::AI)
			{
				players.erase(it);
			}
			else
			{
				--itrev;
			}
		}
	}
	if(teamsCnt == RANDOM_SIZE)
	{
		teamsCnt = gen.getInteger(0, playersCnt - 1);
	}
	if(compOnlyPlayersCnt == RANDOM_SIZE)
	{
		compOnlyPlayersCnt = gen.getInteger(0, 8 - playersCnt);
		auto totalPlayersCnt = playersCnt + compOnlyPlayersCnt;

		// Remove comp only players only from the end of the players map if necessary
		for(auto itrev = players.end(); itrev != players.begin();)
		{
			auto it = itrev;
			--it;
			if(players.size() <= totalPlayersCnt) break;
			if(it->second.getPlayerType() == EPlayerType::COMP_ONLY)
			{
				players.erase(it);
			}
			else
			{
				--itrev;
			}
		}

		// Add some comp only players if necessary
		auto compOnlyPlayersToAdd = totalPlayersCnt - players.size();
		for(int i = 0; i < compOnlyPlayersToAdd; ++i)
		{
			CPlayerSettings pSettings;
			pSettings.setPlayerType(EPlayerType::COMP_ONLY);
			pSettings.setColor(getNextPlayerColor());
			players[pSettings.getColor()] = pSettings;
		}
	}
	if(compOnlyTeamsCnt == RANDOM_SIZE)
	{
		compOnlyTeamsCnt = gen.getInteger(0, std::max(compOnlyPlayersCnt - 1, 0));
	}

	// There should be at least 2 players (1-player-maps aren't allowed)
	if(playersCnt + compOnlyPlayersCnt < 2)
	{
		CPlayerSettings pSettings;
		pSettings.setPlayerType(EPlayerType::AI);
		pSettings.setColor(getNextPlayerColor());
		players[pSettings.getColor()] = pSettings;
		playersCnt = 2;
	}

	// 1 team isn't allowed
	if(teamsCnt == 1 && compOnlyPlayersCnt == 0)
	{
		teamsCnt = 0;
	}

	if(waterContent == EWaterContent::RANDOM)
	{
		waterContent = static_cast<EWaterContent::EWaterContent>(gen.getInteger(0, 2));
	}
	if(monsterStrength == EMonsterStrength::RANDOM)
	{
		monsterStrength = static_cast<EMonsterStrength::EMonsterStrength>(gen.getInteger(0, 2));
	}
}

int CMapGenOptions::countHumanPlayers() const
{
	return static_cast<int>(boost::count_if(players, [](const std::pair<PlayerColor, CPlayerSettings> & pair)
	{
		return pair.second.getPlayerType() == EPlayerType::HUMAN;
	}));
}

PlayerColor CMapGenOptions::getNextPlayerColor() const
{
	for(PlayerColor i = PlayerColor(0); i < PlayerColor::PLAYER_LIMIT; i.advance(1))
	{
		if(!players.count(i))
		{
			return i;
		}
	}
	assert(0);
	return PlayerColor(0);
}

CMapGenOptions::CPlayerSettings::CPlayerSettings() : color(0), startingTown(RANDOM_TOWN), playerType(EPlayerType::AI)
{

}

PlayerColor CMapGenOptions::CPlayerSettings::getColor() const
{
	return color;
}

void CMapGenOptions::CPlayerSettings::setColor(PlayerColor value)
{
	assert(value >= PlayerColor(0) && value < PlayerColor::PLAYER_LIMIT);
	color = value;
}

si32 CMapGenOptions::CPlayerSettings::getStartingTown() const
{
	return startingTown;
}

void CMapGenOptions::CPlayerSettings::setStartingTown(si32 value)
{
	assert(value >= -1 && value < static_cast<int>(VLC->townh->factions.size()));
	startingTown = value;
}

EPlayerType::EPlayerType CMapGenOptions::CPlayerSettings::getPlayerType() const
{
	return playerType;
}

void CMapGenOptions::CPlayerSettings::setPlayerType(EPlayerType::EPlayerType value)
{
	playerType = value;
}

CMapGenerator::CMapGenerator(const CMapGenOptions & mapGenOptions, int randomSeed /*= std::time(nullptr)*/) :
	mapGenOptions(mapGenOptions), randomSeed(randomSeed)
{
	gen.seed(randomSeed);
}

CMapGenerator::~CMapGenerator()
{

}

std::unique_ptr<CMap> CMapGenerator::generate()
{
	mapGenOptions.finalize(gen);

	//TODO select a template based on the map gen options or adapt it if necessary
	CRandomMapTemplateStorage::get();

	map = make_unique<CMap>();
	editManager = map->getEditManager();
	editManager->getUndoManager().setUndoRedoLimit(0);
	addHeaderInfo();

	genTerrain();
	genTowns();

	return std::move(map);
}

std::string CMapGenerator::getMapDescription() const
{
	const std::string waterContentStr[3] = { "none", "normal", "islands" };
	const std::string monsterStrengthStr[3] = { "weak", "normal", "strong" };

	std::stringstream ss;
	ss << "Map created by the Random Map Generator.\nTemplate was <MOCK>, ";
	ss << "Random seed was " << randomSeed << ", size " << map->width << "x";
	ss << map->height << ", levels " << (map->twoLevel ? "2" : "1") << ", ";
	ss << "humans " << static_cast<int>(mapGenOptions.getPlayersCnt()) << ", computers ";
	ss << static_cast<int>(mapGenOptions.getCompOnlyPlayersCnt()) << ", water " << waterContentStr[mapGenOptions.getWaterContent()];
	ss << ", monster " << monsterStrengthStr[mapGenOptions.getMonsterStrength()] << ", second expansion map";

	BOOST_FOREACH(const auto & pair, mapGenOptions.getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		if(pSettings.getPlayerType() == EPlayerType::HUMAN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()] << " is human";
		}
		if(pSettings.getStartingTown() != CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()]
			   << " town choice is " << ETownType::names[pSettings.getStartingTown()];
		}
	}

	return ss.str();
}

void CMapGenerator::addPlayerInfo()
{
	// Calculate which team numbers exist
	std::array<std::list<int>, 2> teamNumbers; // 0= cpu/human, 1= cpu only
	int teamOffset = 0;
	for(int i = 0; i < 2; ++i)
	{
		int playersCnt = i == 0 ? mapGenOptions.getPlayersCnt() : mapGenOptions.getCompOnlyPlayersCnt();
		int teamsCnt = i == 0 ? mapGenOptions.getTeamsCnt() : mapGenOptions.getCompOnlyTeamsCnt();

		if(playersCnt == 0)
		{
			continue;
		}
		int playersPerTeam = playersCnt /
				(teamsCnt == 0 ? playersCnt : teamsCnt);
		int teamsCntNorm = teamsCnt;
		if(teamsCntNorm == 0)
		{
			teamsCntNorm = playersCnt;
		}
		for(int j = 0; j < teamsCntNorm; ++j)
		{
			for(int k = 0; k < playersPerTeam; ++k)
			{
				teamNumbers[i].push_back(j + teamOffset);
			}
		}
		for(int j = 0; j < playersCnt - teamsCntNorm * playersPerTeam; ++j)
		{
			teamNumbers[i].push_back(j + teamOffset);
		}
		teamOffset += teamsCntNorm;
	}

	// Team numbers are assigned randomly to every player
	BOOST_FOREACH(const auto & pair, mapGenOptions.getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		PlayerInfo player;
		player.canComputerPlay = true;
		int j = pSettings.getPlayerType() == EPlayerType::COMP_ONLY ? 1 : 0;
		if(j == 0)
		{
			player.canHumanPlay = true;
		}
		auto itTeam = std::next(teamNumbers[j].begin(), gen.getInteger(0, teamNumbers[j].size() - 1));
		player.team = TeamID(*itTeam);
		teamNumbers[j].erase(itTeam);
		map->players[pSettings.getColor().getNum()] = player;
	}

	map->howManyTeams = (mapGenOptions.getTeamsCnt() == 0 ? mapGenOptions.getPlayersCnt() : mapGenOptions.getTeamsCnt())
			+ (mapGenOptions.getCompOnlyTeamsCnt() == 0 ? mapGenOptions.getCompOnlyPlayersCnt() : mapGenOptions.getCompOnlyTeamsCnt());
}

void CMapGenerator::genTerrain()
{
	map->initTerrain();
	editManager->clearTerrain(&gen);
	editManager->getTerrainSelection().selectRange(MapRect(int3(10, 10, 0), 20, 30));
	editManager->drawTerrain(ETerrainType::GRASS, &gen);
}

void CMapGenerator::genTowns()
{
	//FIXME mock gen
	const int3 townPos[2] = { int3(17, 13, 0), int3(25,13, 0) };
	const int townTypes[2] = { ETownType::CASTLE, ETownType::DUNGEON };

	for(size_t i = 0; i < map->players.size(); ++i)
	{
		auto & playerInfo = map->players[i];
		if(!playerInfo.canAnyonePlay()) break;

		PlayerColor owner(i);
		int side = i % 2;
		CGTownInstance * town = new CGTownInstance();
		town->ID = Obj::TOWN;
		town->subID = townTypes[side];
		town->tempOwner = owner;
		town->defInfo = VLC->dobjinfo->gobjs[town->ID][town->subID];
		town->builtBuildings.insert(BuildingID::FORT);
		town->builtBuildings.insert(BuildingID::DEFAULT);
		editManager->insertObject(town, int3(townPos[side].x, townPos[side].y + (i / 2) * 5, 0));

		// Update player info
		playerInfo.allowedFactions.clear();
		playerInfo.allowedFactions.insert(townTypes[side]);
		playerInfo.hasMainTown = true;
		playerInfo.posOfMainTown = town->pos - int3(2, 0, 0);
		playerInfo.generateHeroAtMainTown = true;
	}
}

void CMapGenerator::addHeaderInfo()
{
	map->version = EMapFormat::SOD;
	map->width = mapGenOptions.getWidth();
	map->height = mapGenOptions.getHeight();
	map->twoLevel = mapGenOptions.getHasTwoLevels();
	map->name = VLC->generaltexth->allTexts[740];
	map->description = getMapDescription();
	map->difficulty = 1;
	addPlayerInfo();
}

CTemplateZoneTowns::CTemplateZoneTowns() : minTowns(0), minCastles(0), townDensity(0), castleDensity(0)
{

}

int CTemplateZoneTowns::getMinTowns() const
{
	return minTowns;
}

void CTemplateZoneTowns::setMinTowns(int value)
{
	assert(value >= 0);
	minTowns = value;
}

int CTemplateZoneTowns::getMinCastles() const
{
	return minCastles;
}

void CTemplateZoneTowns::setMinCastles(int value)
{
	assert(value >= 0);
	minCastles = value;
}

int CTemplateZoneTowns::getTownDensity() const
{
	return townDensity;
}

void CTemplateZoneTowns::setTownDensity(int value)
{
	assert(value >= 0);
	townDensity = value;
}

int CTemplateZoneTowns::getCastleDensity() const
{
	return castleDensity;
}

void CTemplateZoneTowns::setCastleDensity(int value)
{
	assert(value >= 0);
	castleDensity = value;
}

CTemplateZone::CTemplateZone() : id(0), type(ETemplateZoneType::HUMAN_START), baseSize(0), owner(0),
	neutralTownsAreSameType(false), matchTerrainToTown(true)
{

}

TTemplateZoneId CTemplateZone::getId() const
{
	return id;
}

void CTemplateZone::setId(TTemplateZoneId value)
{
	id = value;
}

ETemplateZoneType::ETemplateZoneType CTemplateZone::getType() const
{
	return type;
}
void CTemplateZone::setType(ETemplateZoneType::ETemplateZoneType value)
{
	type = value;
}

int CTemplateZone::getBaseSize() const
{
	return baseSize;
}

void CTemplateZone::setBaseSize(int value)
{
	assert(value >= 0);
	baseSize = value;
}

int CTemplateZone::getOwner() const
{
	return owner;
}

void CTemplateZone::setOwner(int value)
{
	owner = value;
}

const CTemplateZoneTowns & CTemplateZone::getPlayerTowns() const
{
	return playerTowns;
}

void CTemplateZone::setPlayerTowns(const CTemplateZoneTowns & value)
{
	playerTowns = value;
}

const CTemplateZoneTowns & CTemplateZone::getNeutralTowns() const
{
	return neutralTowns;
}

void CTemplateZone::setNeutralTowns(const CTemplateZoneTowns & value)
{
	neutralTowns = value;
}

bool CTemplateZone::getNeutralTownsAreSameType() const
{
	return neutralTownsAreSameType;
}

void CTemplateZone::setNeutralTownsAreSameType(bool value)
{
	neutralTownsAreSameType = value;
}

const std::set<TFaction> & CTemplateZone::getAllowedTownTypes() const
{
	return allowedTownTypes;
}

void CTemplateZone::setAllowedTownTypes(const std::set<TFaction> & value)
{
	allowedTownTypes = value;
}

bool CTemplateZone::getMatchTerrainToTown() const
{
	return matchTerrainToTown;
}

void CTemplateZone::setMatchTerrainToTown(bool value)
{
	matchTerrainToTown = value;
}

const std::set<ETerrainType> & CTemplateZone::getTerrainTypes() const
{
	return terrainTypes;
}

void CTemplateZone::setTerrainTypes(const std::set<ETerrainType> & value)
{
	assert(value.find(ETerrainType::WRONG) == value.end() && value.find(ETerrainType::BORDER) == value.end() &&
		   value.find(ETerrainType::WATER) == value.end());
	terrainTypes = value;
}

CTemplateZoneConnection::CTemplateZoneConnection() : zoneA(0), zoneB(0), guardStrength(0)
{

}

TTemplateZoneId CTemplateZoneConnection::getZoneA() const
{
	return zoneA;
}

void CTemplateZoneConnection::setZoneA(TTemplateZoneId value)
{
	zoneA = value;
}

TTemplateZoneId CTemplateZoneConnection::getZoneB() const
{
	return zoneB;
}

void CTemplateZoneConnection::setZoneB(TTemplateZoneId value)
{
	zoneB = value;
}

int CTemplateZoneConnection::getGuardStrength() const
{
	return guardStrength;
}

void CTemplateZoneConnection::setGuardStrength(int value)
{
	assert(value >= 0);
	guardStrength = value;
}

CRandomMapTemplateSize::CRandomMapTemplateSize() : width(CMapHeader::MAP_SIZE_MIDDLE), height(CMapHeader::MAP_SIZE_MIDDLE), under(true)
{

}

CRandomMapTemplateSize::CRandomMapTemplateSize(int width, int height, bool under) : width(width), height(height), under(under)
{

}

int CRandomMapTemplateSize::getWidth() const
{
	return width;
}

void CRandomMapTemplateSize::setWidth(int value)
{
	assert(value >= 1);
	width = value;
}

int CRandomMapTemplateSize::getHeight() const
{
	return height;
}

void CRandomMapTemplateSize::setHeight(int value)
{
	assert(value >= 1);
	height = value;
}

bool CRandomMapTemplateSize::getUnder() const
{
	return under;
}

void CRandomMapTemplateSize::setUnder(bool value)
{
	under = value;
}

CRandomMapTemplate::CRandomMapTemplate() : minHumanCnt(1), maxHumanCnt(PlayerColor::PLAYER_LIMIT_I), minTotalCnt(2),
	maxTotalCnt(PlayerColor::PLAYER_LIMIT_I)
{
	minSize = CRandomMapTemplateSize(CMapHeader::MAP_SIZE_SMALL, CMapHeader::MAP_SIZE_SMALL, false);
	maxSize = CRandomMapTemplateSize(CMapHeader::MAP_SIZE_XLARGE, CMapHeader::MAP_SIZE_XLARGE, true);
}

const std::string & CRandomMapTemplate::getName() const
{
	return name;
}

void CRandomMapTemplate::setName(const std::string & value)
{
	name = value;
}

const CRandomMapTemplateSize & CRandomMapTemplate::getMinSize() const
{
	return minSize;
}

void CRandomMapTemplate::setMinSize(const CRandomMapTemplateSize & value)
{
	minSize = value;
}

const CRandomMapTemplateSize & CRandomMapTemplate::getMaxSize() const
{
	return maxSize;
}

void CRandomMapTemplate::setMaxSize(const CRandomMapTemplateSize & value)
{
	maxSize = value;
}

int CRandomMapTemplate::getMinHumanCnt() const
{
	return minHumanCnt;
}

void CRandomMapTemplate::setMinHumanCnt(int value)
{
	assert(value >= 1 && value <= PlayerColor::PLAYER_LIMIT_I);
	minHumanCnt = value;
}

int CRandomMapTemplate::getMaxHumanCnt() const
{
	return maxHumanCnt;
}

void CRandomMapTemplate::setMaxHumanCnt(int value)
{
	assert(value >= 1 && value <= PlayerColor::PLAYER_LIMIT_I);
	maxHumanCnt = value;
}

int CRandomMapTemplate::getMinTotalCnt() const
{
	return minTotalCnt;
}

void CRandomMapTemplate::setMinTotalCnt(int value)
{
	assert(value >= 2 && value <= PlayerColor::PLAYER_LIMIT_I);
	minTotalCnt = value;
}

int CRandomMapTemplate::getMaxTotalCnt() const
{
	return maxTotalCnt;
}

void CRandomMapTemplate::setMaxTotalCnt(int value)
{
	assert(value >= 2 && value <= PlayerColor::PLAYER_LIMIT_I);
	maxTotalCnt = value;
}

const std::map<TTemplateZoneId, CTemplateZone> & CRandomMapTemplate::getZones() const
{
	return zones;
}

void CRandomMapTemplate::setZones(const std::map<TTemplateZoneId, CTemplateZone> & value)
{
	zones = value;
}

const std::list<CTemplateZoneConnection> & CRandomMapTemplate::getConnections() const
{
	return connections;
}

void CRandomMapTemplate::setConnections(const std::list<CTemplateZoneConnection> & value)
{
	connections = value;
}

const std::map<std::string, CRandomMapTemplate> & CRmTemplateLoader::getTemplates() const
{
	return templates;
}

void CJsonRmTemplateLoader::loadTemplates()
{
	const JsonNode rootNode(ResourceID("config/rmg.json"));
	BOOST_FOREACH(const auto & templatePair, rootNode.Struct())
	{
		CRandomMapTemplate tpl;
		tpl.setName(templatePair.first);
		const auto & templateNode = templatePair.second;

		// Parse main template data
		tpl.setMinSize(parseMapTemplateSize(templateNode["minSize"].String()));
		tpl.setMaxSize(parseMapTemplateSize(templateNode["maxSize"].String()));
		tpl.setMinHumanCnt(templateNode["minHumanCnt"].Float());
		tpl.setMaxHumanCnt(templateNode["maxHumanCnt"].Float());
		tpl.setMinTotalCnt(templateNode["minTotalCnt"].Float());
		tpl.setMaxTotalCnt(templateNode["maxTotalCnt"].Float());

		// Parse zones
		std::map<TTemplateZoneId, CTemplateZone> zones;
		BOOST_FOREACH(const auto & zonePair, templateNode["zones"].Struct())
		{
			CTemplateZone zone;
			auto zoneId = boost::lexical_cast<TTemplateZoneId>(zonePair.first);
			zone.setId(zoneId);
			const auto & zoneNode = zonePair.second;
			zone.setType(getZoneType(zoneNode["type"].String()));
			zone.setBaseSize(zoneNode["baseSize"].Float());
			zone.setOwner(zoneNode["owner"].Float());
			zone.setPlayerTowns(parseTemplateZoneTowns(zoneNode["playerTowns"]));
			zone.setNeutralTowns(parseTemplateZoneTowns(zoneNode["neutralTowns"]));
			zone.setAllowedTownTypes(getFactions(zoneNode["allowedTownTypes"].Vector()));
			zone.setMatchTerrainToTown(zoneNode["matchTerrainToTown"].Bool());
			zone.setTerrainTypes(parseTerrainTypes(zoneNode["terrainTypes"].Vector()));
			zone.setNeutralTownsAreSameType((zoneNode["neutralTownsAreSameType"].Bool()));
			zones[zone.getId()] = zone;
		}
		tpl.setZones(zones);

		// Parse connections
		std::list<CTemplateZoneConnection> connections;
		BOOST_FOREACH(const auto & connPair, templateNode["connections"].Vector())
		{
			CTemplateZoneConnection conn;
			conn.setZoneA(boost::lexical_cast<TTemplateZoneId>(connPair["a"].String()));
			conn.setZoneB(boost::lexical_cast<TTemplateZoneId>(connPair["b"].String()));
			conn.setGuardStrength(connPair["guardStrength"].Float());
			connections.push_back(conn);
		}
		tpl.setConnections(connections);
		templates[tpl.getName()] = tpl;
	}
}

CRandomMapTemplateSize CJsonRmTemplateLoader::parseMapTemplateSize(const std::string & text) const
{
	CRandomMapTemplateSize size;
	if(text.empty()) return size;

	std::vector<std::string> parts;
	boost::split(parts, text, boost::is_any_of("+"));
	static const std::map<std::string, int> mapSizeMapping = boost::assign::map_list_of("s", CMapHeader::MAP_SIZE_SMALL)
			("m", CMapHeader::MAP_SIZE_MIDDLE)("l", CMapHeader::MAP_SIZE_LARGE)("xl", CMapHeader::MAP_SIZE_XLARGE);
	auto it = mapSizeMapping.find(parts[0]);
	if(it == mapSizeMapping.end())
	{
		// Map size is given as a number representation
		const auto & numericalRep = parts[0];
		parts.clear();
		boost::split(parts, numericalRep, boost::is_any_of("x"));
		assert(parts.size() == 3);
		size.setWidth(boost::lexical_cast<int>(parts[0]));
		size.setHeight(boost::lexical_cast<int>(parts[1]));
		size.setUnder(boost::lexical_cast<int>(parts[2]) == 1);
	}
	else
	{
		size.setWidth(it->second);
		size.setHeight(it->second);
		size.setUnder(parts.size() > 1 ? parts[1] == std::string("u") : false);
	}
	return size;
}

ETemplateZoneType::ETemplateZoneType CJsonRmTemplateLoader::getZoneType(const std::string & type) const
{
	static const std::map<std::string, ETemplateZoneType::ETemplateZoneType> zoneTypeMapping = boost::assign::map_list_of
			("humanStart", ETemplateZoneType::HUMAN_START)("computerStart", ETemplateZoneType::COMPUTER_START)
			("treasure", ETemplateZoneType::TREASURE)("junction", ETemplateZoneType::JUNCTION);
	auto it = zoneTypeMapping.find(type);
	assert(it != zoneTypeMapping.end());
	return it->second;
}

CTemplateZoneTowns CJsonRmTemplateLoader::parseTemplateZoneTowns(const JsonNode & node) const
{
	CTemplateZoneTowns towns;
	towns.setMinTowns(node["minTowns"].Float());
	towns.setMinCastles(node["minCastles"].Float());
	towns.setTownDensity(node["townDensity"].Float());
	towns.setCastleDensity(node["castleDensity"].Float());
	return towns;
}

std::set<TFaction> CJsonRmTemplateLoader::getFactions(const std::vector<JsonNode> factionStrings) const
{
	std::set<TFaction> factions;
	BOOST_FOREACH(const auto & factionNode, factionStrings)
	{
		auto factionStr = factionNode.String();
		if(factionStr == "all")
		{
			factions.clear();
			BOOST_FOREACH(auto factionPtr, VLC->townh->factions)
			{
				factions.insert(factionPtr->index);
			}
			return factions;
		}
		BOOST_FOREACH(auto factionPtr, VLC->townh->factions)
		{
			if(factionStr == factionPtr->name)
			{
				factions.insert(factionPtr->index);
				break;
			}
		}
	}
	return factions;
}

std::set<ETerrainType> CJsonRmTemplateLoader::parseTerrainTypes(const std::vector<JsonNode> terTypeStrings) const
{
	std::set<ETerrainType> terTypes;
	BOOST_FOREACH(const auto & node, terTypeStrings)
	{
		const auto & terTypeStr = node.String();
		if(terTypeStr == "all")
		{
			for(int i = 0; i < GameConstants::TERRAIN_TYPES; ++i) terTypes.insert(ETerrainType(i));
			break;
		}
		terTypes.insert(ETerrainType(vstd::find_pos(GameConstants::TERRAIN_NAMES, terTypeStr)));
	}
	return terTypes;
}

boost::mutex CRandomMapTemplateStorage::smx;

CRandomMapTemplateStorage & CRandomMapTemplateStorage::get()
{
	TLockGuard _(smx);
	static CRandomMapTemplateStorage storage;
	return storage;
}

const std::map<std::string, CRandomMapTemplate> & CRandomMapTemplateStorage::getTemplates() const
{
	return templates;
}

CRandomMapTemplateStorage::CRandomMapTemplateStorage()
{
	auto jsonLoader = make_unique<CJsonRmTemplateLoader>();
	jsonLoader->loadTemplates();
	const auto & tpls = jsonLoader->getTemplates();
	templates.insert(tpls.begin(), tpls.end());
}

CRandomMapTemplateStorage::~CRandomMapTemplateStorage()
{

}
