/*
* MapFormatJson.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "MapFormatJson.h"

#include "../filesystem/CInputStream.h"
#include "../filesystem/COutputStream.h"
#include "CMap.h"
#include "../CModHandler.h"
#include "../CHeroHandler.h"
#include "../CTownHandler.h"
#include "../VCMI_Lib.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapObjects/CObjectHandler.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../spells/CSpellHandler.h"
#include "../StringConstants.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

namespace HeaderDetail
{
	static const ui8 difficultyDefault = 1;//normal

	static const std::vector<std::string> difficultyMap =
	{
		"EASY",
		"NORMAL",
		"HARD",
		"EXPERT",
		"IMPOSSIBLE"
	};
}

namespace TriggeredEventsDetail
{
	static const std::array<std::string, 12> conditionNames =
	{
		"haveArtifact", "haveCreatures",   "haveResources",   "haveBuilding",
		"control",      "destroy",         "transport",       "daysPassed",
		"isHuman",      "daysWithoutTown", "standardWin",     "constValue"
	};

	static const std::array<std::string, 2> typeNames = { "victory", "defeat" };

	static EventCondition JsonToCondition(const JsonNode & node)
	{
		//todo: support of new condition format
		EventCondition event;

		const auto & conditionName = node.Vector()[0].String();

		auto pos = vstd::find_pos(conditionNames, conditionName);

		event.condition = EventCondition::EWinLoseType(pos);
		if (node.Vector().size() > 1)
		{
			const JsonNode & data = node.Vector()[1];
			if (data["type"].getType() == JsonNode::DATA_STRING)
			{
				auto identifier = VLC->modh->identifiers.getIdentifier(data["type"]);
				if(identifier)
					event.objectType = identifier.get();
				else
					throw std::runtime_error("Identifier resolution failed in event condition");
			}

			if (data["type"].getType() == JsonNode::DATA_FLOAT)
				event.objectType = data["type"].Float();

			if (!data["value"].isNull())
				event.value = data["value"].Float();

			if (!data["position"].isNull())
			{
				auto & position = data["position"].Vector();
				event.position.x = position.at(0).Float();
				event.position.y = position.at(1).Float();
				event.position.z = position.at(2).Float();
			}
		}
		return event;
	}

	static JsonNode ConditionToJson(const EventCondition& event)
	{
		JsonNode json;

		JsonVector& asVector = json.Vector();

		JsonNode condition;
		condition.String() = conditionNames.at(event.condition);
		asVector.push_back(condition);

		JsonNode data;

		//todo: save identifier

		if(event.objectType != -1)
			data["type"].Float() = event.objectType;

		if(event.value != -1)
			data["value"].Float() = event.value;

		if(event.position != int3(-1,-1,-1))
		{
			auto & position = data["position"].Vector();
			position.resize(3);
			position[0].Float() = event.position.x;
			position[1].Float() = event.position.y;
			position[2].Float() = event.position.z;
		}

		if(!data.isNull())
			asVector.push_back(data);

		return std::move(json);
	}
}//namespace TriggeredEventsDetail

namespace TerrainDetail
{
	static const std::array<std::string, 10> terrainCodes =
	{
		"dt", "sa", "gr", "sn", "sw", "rg", "sb", "lv", "wt", "rc"
	};
	static const std::array<std::string, 4> roadCodes =
	{
		"", "pd", "pg", "pc"
	};

	static const std::array<std::string, 5> riverCodes =
	{
		"", "rw", "ri", "rm", "rl"
	};

	static const std::array<char, 10> flipCodes =
	{
		'_', '-', '|', '+'
	};
}

static std::string tailString(const std::string & input, char separator)
{
	std::string ret;
	size_t splitPos = input.find(separator);
	if (splitPos != std::string::npos)
		ret = input.substr(splitPos + 1);
	return ret;
}

static si32 extractNumber(const std::string & input, char separator)
{
	std::string tmp = tailString(input, separator);
	return atoi(tmp.c_str());
}

///CMapFormatJson
const int CMapFormatJson::VERSION_MAJOR = 1;
const int CMapFormatJson::VERSION_MINOR = 0;

const std::string CMapFormatJson::HEADER_FILE_NAME = "header.json";
const std::string CMapFormatJson::OBJECTS_FILE_NAME = "objects.json";

void CMapFormatJson::serializeAllowedFactions(JsonSerializeFormat & handler, std::set<TFaction> & value)
{
	//TODO: unify allowed factions with others - make them std::vector<bool>

	std::vector<bool> temp;
	temp.resize(VLC->townh->factions.size(), false);
	auto standard = VLC->townh->getDefaultAllowed();

    if(handler.saving)
	{
		for(auto faction : VLC->townh->factions)
			if(faction->town && vstd::contains(value, faction->index))
				temp[std::size_t(faction->index)] = true;
	}

	handler.serializeLIC("allowedFactions", &CTownHandler::decodeFaction, &CTownHandler::encodeFaction, standard, temp);

	if(!handler.saving)
	{
		value.clear();
		for (std::size_t i=0; i<temp.size(); i++)
			if(temp[i])
				value.insert(i);
	}
}

void CMapFormatJson::serializeHeader(JsonSerializeFormat & handler)
{
	handler.serializeString("name", mapHeader->name);
	handler.serializeString("description", mapHeader->description);
	handler.serializeNumeric("heroLevelLimit", mapHeader->levelLimit);

	//todo: support arbitrary percentage
	handler.serializeNumericEnum("difficulty", HeaderDetail::difficultyMap, HeaderDetail::difficultyDefault, mapHeader->difficulty);

	serializePlayerInfo(handler);

	handler.serializeLIC("allowedHeroes", &CHeroHandler::decodeHero, &CHeroHandler::encodeHero, VLC->heroh->getDefaultAllowed(), mapHeader->allowedHeroes);
}

void CMapFormatJson::serializePlayerInfo(JsonSerializeFormat & handler)
{
	auto playersData = handler.enterStruct("players");

	for(int player = 0; player < PlayerColor::PLAYER_LIMIT_I; player++)
	{
		PlayerInfo & info = mapHeader->players[player];

		if(handler.saving)
		{
			if(!info.canAnyonePlay())
				continue;
		}

		auto playerData = playersData.enterStruct(GameConstants::PLAYER_COLOR_NAMES[player]);

		if(!handler.saving)
		{
			if(playerData.get().isNull())
			{
				info.canComputerPlay = false;
				info.canHumanPlay = false;
				continue;
			}
			info.canComputerPlay = true;
		}

		serializeAllowedFactions(handler, info.allowedFactions);

		handler.serializeBoolEnum("canPlay", "PlayerOrAI", "AIOnly", info.canHumanPlay);

		//mainTown
		if(handler.saving)
		{

		}
		else
		{

		}

		handler.serializeBool("generateHeroAtMainTown", info.generateHeroAtMainTown);


		//mainHero

		//mainHeroPortrait

		//mainCustomHeroName

		//towns

		//heroes
		{
			//auto heroes = playerData.enterStruct("heroes");

		}

		if(!handler.saving)
		{
			//isFactionRandom indicates that player may select faction, depends on towns & heroes
			// true if main town is random and generateHeroAtMainTown==true
			// or main hero is random
			//TODO: recheck mechanics
			//	info.isFactionRandom =
		}


	}
}

void CMapFormatJson::readTeams(JsonDeserializer & handler)
{
	auto teams = handler.enterStruct("teams");
	const JsonNode & src = teams.get();

    if(src.getType() != JsonNode::DATA_VECTOR)
	{
		// No alliances
		if(src.getType() != JsonNode::DATA_NULL)
			logGlobal->errorStream() << "Invalid teams field type";

		mapHeader->howManyTeams = 0;
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
		{
			if(mapHeader->players[i].canComputerPlay || mapHeader->players[i].canHumanPlay)
			{
				mapHeader->players[i].team = TeamID(mapHeader->howManyTeams++);
			}
		}
	}
	else
	{
		const JsonVector & srcVector = src.Vector();
		mapHeader->howManyTeams = srcVector.size();

		for(int team = 0; team < mapHeader->howManyTeams; team++)
		{
			for(const JsonNode & playerData : srcVector[team].Vector())
			{
				PlayerColor player = PlayerColor(vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, playerData.String()));
				if(player.isValidPlayer())
				{
					if(mapHeader->players[player.getNum()].canAnyonePlay())
					{
						mapHeader->players[player.getNum()].team = TeamID(team);
					}
				}
			}
		}

		for(PlayerInfo & player : mapHeader->players)
		{
			if(player.canAnyonePlay() && player.team == TeamID::NO_TEAM)
				player.team = TeamID(mapHeader->howManyTeams++);
		}

	}
}


void CMapFormatJson::writeTeams(JsonSerializer & handler)
{
	auto teams = handler.enterStruct("teams");
	JsonNode & dest = teams.get();
	std::vector<std::set<PlayerColor>> teamsData;

	teamsData.resize(mapHeader->howManyTeams);

	//get raw data
	for(int idx = 0; idx < mapHeader->players.size(); idx++)
	{
		const PlayerInfo & player = mapHeader->players.at(idx);
		int team = player.team.getNum();
		if(vstd::iswithin(team, 0, mapHeader->howManyTeams-1) && player.canAnyonePlay())
			teamsData.at(team).insert(PlayerColor(idx));
	}

	//remove single-member teams
	vstd::erase_if(teamsData, [](std::set<PlayerColor> & elem) -> bool
	{
		return elem.size() <= 1;
	});

	//construct output
	dest.setType(JsonNode::DATA_VECTOR);

	for(const std::set<PlayerColor> & teamData : teamsData)
	{
		JsonNode team(JsonNode::DATA_VECTOR);
		for(const PlayerColor & player : teamData)
		{
			JsonNode member(JsonNode::DATA_STRING);
			member.String() = GameConstants::PLAYER_COLOR_NAMES[player.getNum()];
			team.Vector().push_back(std::move(member));
		}
		dest.Vector().push_back(std::move(team));
	}
}

void CMapFormatJson::serializeTriggeredEvents(JsonSerializeFormat & handler)
{
	handler.serializeString("victoryString", mapHeader->victoryMessage);
	handler.serializeNumeric("victoryIconIndex", mapHeader->victoryIconIndex);

	handler.serializeString("defeatString", mapHeader->defeatMessage);
	handler.serializeNumeric("defeatIconIndex", mapHeader->defeatIconIndex);
}


void CMapFormatJson::readTriggeredEvents(JsonDeserializer & handler)
{
	const JsonNode & input = handler.getCurrent();

	serializeTriggeredEvents(handler);

	mapHeader->triggeredEvents.clear();

	for (auto & entry : input["triggeredEvents"].Struct())
	{
		TriggeredEvent event;
		event.identifier = entry.first;
		readTriggeredEvent(event, entry.second);
		mapHeader->triggeredEvents.push_back(event);
	}
}

void CMapFormatJson::readTriggeredEvent(TriggeredEvent & event, const JsonNode & source)
{
	using namespace TriggeredEventsDetail;

	event.onFulfill = source["message"].String();
	event.description = source["description"].String();
	event.effect.type = vstd::find_pos(typeNames, source["effect"]["type"].String());
	event.effect.toOtherMessage = source["effect"]["messageToSend"].String();
	event.trigger = EventExpression(source["condition"], JsonToCondition); // logical expression
}

void CMapFormatJson::writeTriggeredEvents(JsonSerializer & handler)
{
	JsonNode & output = handler.getCurrent();

	serializeTriggeredEvents(handler);

	JsonMap & triggeredEvents = output["triggeredEvents"].Struct();

	for(auto event : mapHeader->triggeredEvents)
		writeTriggeredEvent(event, triggeredEvents[event.identifier]);
}

void CMapFormatJson::writeTriggeredEvent(const TriggeredEvent& event, JsonNode& dest)
{
	using namespace TriggeredEventsDetail;

	dest["message"].String() = event.onFulfill;
	dest["description"].String() = event.description;

	dest["effect"]["type"].String() = typeNames.at(size_t(event.effect.type));
	dest["effect"]["messageToSend"].String() = event.effect.toOtherMessage;

	dest["condition"] = event.trigger.toJson(ConditionToJson);
}

void CMapFormatJson::serializeOptions(JsonSerializeFormat & handler)
{
	//rumors

	//disposedHeroes

	//predefinedHeroes

	handler.serializeLIC("allowedAbilities", &CHeroHandler::decodeSkill, &CHeroHandler::encodeSkill, VLC->heroh->getDefaultAllowedAbilities(), map->allowedAbilities);

	handler.serializeLIC("allowedArtifacts", &CArtHandler::decodeArfifact, &CArtHandler::encodeArtifact, VLC->arth->getDefaultAllowed(), map->allowedArtifact);

	handler.serializeLIC("allowedSpells", &CSpellHandler::decodeSpell, &CSpellHandler::encodeSpell, VLC->spellh->getDefaultAllowed(), map->allowedSpell);

	//events
}

void CMapFormatJson::readOptions(JsonDeserializer & handler)
{
	serializeOptions(handler);
}

void CMapFormatJson::writeOptions(JsonSerializer & handler)
{
	serializeOptions(handler);
}


///CMapPatcher
CMapPatcher::CMapPatcher(JsonNode stream):
	input(stream)
{

}

void CMapPatcher::patchMapHeader(std::unique_ptr<CMapHeader> & header)
{
	map = nullptr;
	mapHeader = header.get();
	if (!input.isNull())
		readPatchData();
}

void CMapPatcher::readPatchData()
{
	JsonDeserializer handler(input);
	readTriggeredEvents(handler);
}


///CMapLoaderJson
CMapLoaderJson::CMapLoaderJson(CInputStream * stream):
	buffer(stream),
	ioApi(new CProxyROIOApi(buffer)),
	loader("", "_", ioApi)
{

}

si32 CMapLoaderJson::getIdentifier(const std::string& type, const std::string& name)
{
	boost::optional<si32> res = VLC->modh->identifiers.getIdentifier("core", type, name, false);

	if(!res)
	{
		throw new std::runtime_error("Map load failed. Identifier not resolved.");
	}
	return res.get();
}

std::unique_ptr<CMap> CMapLoaderJson::loadMap()
{
	LOG_TRACE(logGlobal);
	std::unique_ptr<CMap> result = std::unique_ptr<CMap>(new CMap());
	map = result.get();
	mapHeader = map;
	readMap();
	return std::move(result);
}

std::unique_ptr<CMapHeader> CMapLoaderJson::loadMapHeader()
{
	LOG_TRACE(logGlobal);
	map = nullptr;
	std::unique_ptr<CMapHeader> result = std::unique_ptr<CMapHeader>(new CMapHeader());
	mapHeader = result.get();
	readHeader(false);
	return std::move(result);
}

JsonNode CMapLoaderJson::getFromArchive(const std::string & archiveFilename)
{
	ResourceID resource(archiveFilename, EResType::TEXT);

	if(!loader.existsResource(resource))
		throw new std::runtime_error(archiveFilename+" not found");

	auto data = loader.load(resource)->readAll();

	JsonNode res(reinterpret_cast<char*>(data.first.get()), data.second);

	return std::move(res);
}


void CMapLoaderJson::readMap()
{
	LOG_TRACE(logGlobal);
	readHeader(true);
	map->initTerrain();
	readTerrain();
	readObjects();

	// Calculate blocked / visitable positions
	for(auto & elem : map->objects)
	{
		map->addBlockVisTiles(elem);
	}
	map->calculateGuardingGreaturePositions();
}

void CMapLoaderJson::readHeader(const bool complete)
{
	//do not use map field here, use only mapHeader
	JsonNode header = getFromArchive(HEADER_FILE_NAME);

	int versionMajor = header["versionMajor"].Float();

	if(versionMajor != VERSION_MAJOR)
	{
		logGlobal->errorStream() << "Unsupported map format version: " << versionMajor;
		throw std::runtime_error("Unsupported map format version");
	}

	int versionMinor = header["versionMinor"].Float();

	if(versionMinor > VERSION_MINOR)
	{
		logGlobal->traceStream() << "Too new map format revision: " << versionMinor << ". This map should work but some of map features may be ignored.";
	}

	JsonDeserializer handler(header);

	mapHeader->version = EMapFormat::VCMI;//todo: new version field

	//todo: multilevel map load support
	{
		auto levels = handler.enterStruct("mapLevels");

		{
			auto surface = levels.enterStruct("surface");
			mapHeader->height = surface.get()["height"].Float();
			mapHeader->width = surface.get()["width"].Float();
		}
		{
			auto underground = levels.enterStruct("underground");
			mapHeader->twoLevel = !underground.get().isNull();
		}
	}

	serializeHeader(handler);

	readTriggeredEvents(handler);


	readTeams(handler);
	//TODO: readHeader

	if(complete)
		readOptions(handler);
}


void CMapLoaderJson::readTerrainTile(const std::string& src, TerrainTile& tile)
{
	using namespace TerrainDetail;
	{//terrain type
		const std::string typeCode = src.substr(0,2);

		int rawType = vstd::find_pos(terrainCodes, typeCode);

		if(rawType < 0)
			throw new std::runtime_error("Invalid terrain type code in "+src);

		tile.terType = ETerrainType(rawType);
	}
	int startPos = 2; //0+typeCode fixed length
	{//terrain view
		int pos = startPos;
		while(isdigit(src.at(pos)))
			pos++;
		int len = pos - startPos;
		if(len<=0)
			throw new std::runtime_error("Invalid terrain view in "+src);
		const std::string rawCode = src.substr(startPos,len);
		tile.terView = atoi(rawCode.c_str());
		startPos+=len;
	}
	{//terrain flip
		int terrainFlip = vstd::find_pos(flipCodes, src.at(startPos++));
		if(terrainFlip < 0)
			throw new std::runtime_error("Invalid terrain flip in "+src);
		else
			tile.extTileFlags = terrainFlip;
	}
	if(startPos >= src.size())
		return;
	bool hasRoad = true;
	{//road type
		const std::string typeCode = src.substr(startPos,2);
		startPos+=2;
		int rawType = vstd::find_pos(roadCodes, typeCode);
		if(rawType < 0)
		{
			rawType = vstd::find_pos(riverCodes, typeCode);
			if(rawType < 0)
				throw new std::runtime_error("Invalid river type in "+src);
			else
			{
				tile.riverType = ERiverType::ERiverType(rawType);
				hasRoad = false;
			}
		}
		else
			tile.roadType = ERoadType::ERoadType(rawType);
	}
	if(hasRoad)
	{//road dir
		int pos = startPos;
		while(isdigit(src.at(pos)))
			pos++;
		int len = pos - startPos;
		if(len<=0)
			throw new std::runtime_error("Invalid road dir in "+src);
		const std::string rawCode = src.substr(startPos,len);
		tile.roadDir = atoi(rawCode.c_str());
		startPos+=len;
	}
	if(hasRoad)
	{//road flip
		int flip = vstd::find_pos(flipCodes, src.at(startPos++));
		if(flip < 0)
			throw new std::runtime_error("Invalid road flip in "+src);
		else
			tile.extTileFlags |= (flip<<4);
	}
	if(startPos >= src.size())
		return;
	if(hasRoad)
	{//river type
		const std::string typeCode = src.substr(startPos,2);
		startPos+=2;
		int rawType = vstd::find_pos(riverCodes, typeCode);
		if(rawType < 0)
			throw new std::runtime_error("Invalid river type in "+src);
		tile.riverType = ERiverType::ERiverType(rawType);
	}
	{//river dir
		int pos = startPos;
		while(isdigit(src.at(pos)))
			pos++;
		int len = pos - startPos;
		if(len<=0)
			throw new std::runtime_error("Invalid river dir in "+src);
		const std::string rawCode = src.substr(startPos,len);
		tile.riverDir = atoi(rawCode.c_str());
		startPos+=len;
	}
	{//river flip
		int flip = vstd::find_pos(flipCodes, src.at(startPos++));
		if(flip < 0)
			throw new std::runtime_error("Invalid road flip in "+src);
		else
			tile.extTileFlags |= (flip<<2);
	}
}

void CMapLoaderJson::readTerrainLevel(const JsonNode& src, const int index)
{
	int3 pos(0,0,index);

	const JsonVector & rows = src.Vector();

	if(rows.size() != map->height)
		throw new std::runtime_error("Invalid terrain data");

	for(pos.y = 0; pos.y < map->height; pos.y++)
	{
		const JsonVector & tiles = rows[pos.y].Vector();

		if(tiles.size() != map->width)
			throw new std::runtime_error("Invalid terrain data");

		for(pos.x = 0; pos.x < map->width; pos.x++)
			readTerrainTile(tiles[pos.x].String(), map->getTile(pos));
	}
}

void CMapLoaderJson::readTerrain()
{
	{
		const JsonNode surface = getFromArchive("surface_terrain.json");
		readTerrainLevel(surface, 0);
	}
	if(map->twoLevel)
	{
		const JsonNode underground = getFromArchive("underground_terrain.json");
		readTerrainLevel(underground, 1);
	}

}

CMapLoaderJson::MapObjectLoader::MapObjectLoader(CMapLoaderJson * _owner, JsonMap::value_type& json):
	owner(_owner), instance(nullptr),id(-1), jsonKey(json.first), configuration(json.second), internalId(extractNumber(jsonKey, '_'))
{

}

void CMapLoaderJson::MapObjectLoader::construct()
{
	logGlobal->debugStream() <<"Loading: " <<jsonKey;

	//TODO:consider move to ObjectTypeHandler
	//find type handler
	std::string typeName = configuration["type"].String(), subTypeName = configuration["subType"].String();
	if(typeName.empty())
	{
		logGlobal->errorStream() << "Object type missing";
		logGlobal->traceStream() << configuration;
		return;
	}


	//special case for grail
    if(typeName == "grail")
	{
		auto & pos = owner->map->grailPos;
		pos.x = configuration["x"].Float();
		pos.y = configuration["y"].Float();
		pos.z = configuration["l"].Float();
		owner->map->grailRadius = configuration["options"]["grailRadius"].Float();
		return;
	}
	else if(subTypeName.empty())
	{
		logGlobal->errorStream() << "Object subType missing";
		logGlobal->traceStream() << configuration;
		return;
	}

	auto handler = VLC->objtypeh->getHandlerFor(typeName, subTypeName);

	instance = handler->create(ObjectTemplate());
	instance->id = ObjectInstanceID(owner->map->objects.size());
	owner->map->objects.push_back(instance);
}

void CMapLoaderJson::MapObjectLoader::configure()
{
	if(nullptr == instance)
		return;

	JsonDeserializer handler(configuration);

	instance->serializeJson(handler);

	if(instance->ID == Obj::TOWN)
	{
		owner->map->towns.push_back(static_cast<CGTownInstance *>(instance));
	}
	else if(instance->ID == Obj::HERO)
	{
		logGlobal->debugStream() << "Hero: " << VLC->heroh->heroes[instance->subID]->name << " at " << instance->pos;
		owner->map->heroesOnMap.push_back(static_cast<CGHeroInstance *>(instance));
	}
	else if(auto art = dynamic_cast<CGArtifact *>(instance))
	{
		//todo: find better place for this code

		int artID = ArtifactID::NONE;
		int spellID = -1;

		if(art->ID == Obj::SPELL_SCROLL)
		{
			auto spellIdentifier = configuration["options"]["spell"].String();
			auto rawId = VLC->modh->identifiers.getIdentifier("core", "spell", spellIdentifier);
			if(rawId)
				spellID = rawId.get();
			else
				spellID = 0;
			artID = ArtifactID::SPELL_SCROLL;
		}
		else if(art->ID  == Obj::ARTIFACT)
		{
			//specific artifact
			artID = art->subID;
		}

		art->storedArtifact = CArtifactInstance::createArtifact(owner->map, artID, spellID);
	}
}

void CMapLoaderJson::readObjects()
{
	LOG_TRACE(logGlobal);

	std::vector<std::unique_ptr<MapObjectLoader>> loaders;//todo: optimize MapObjectLoader memory layout

	JsonNode data = getFromArchive(OBJECTS_FILE_NAME);

	//get raw data
	for(auto & p : data.Struct())
		loaders.push_back(vstd::make_unique<MapObjectLoader>(this, p));

	auto sortInfos = [](const std::unique_ptr<MapObjectLoader> & lhs, const std::unique_ptr<MapObjectLoader> & rhs) -> bool
	{
		return lhs->internalId < rhs->internalId;
	};
	boost::sort(loaders, sortInfos);//this makes instance ids consistent after save-load, needed for testing
	//todo: use internalId in CMap

	for(auto & ptr : loaders)
		ptr->construct();

	//configure objects after all objects are constructed so we may resolve internal IDs even to actual pointers OTF
	for(auto & ptr : loaders)
		ptr->configure();
}

///CMapSaverJson
CMapSaverJson::CMapSaverJson(CInputOutputStream * stream):
	buffer(stream),
	ioApi(new CProxyIOApi(buffer)),
	saver(ioApi, "_")
{

}

CMapSaverJson::~CMapSaverJson()
{

}

void CMapSaverJson::addToArchive(const JsonNode& data, const std::string& filename)
{
	std::ostringstream out;
	out << data;
	out.flush();

	{
		auto s = out.str();
		std::unique_ptr<COutputStream> stream = saver.addFile(filename);

		if (stream->write((const ui8*)s.c_str(), s.size()) != s.size())
			throw new std::runtime_error("CMapSaverJson::saveHeader() zip compression failed.");
	}
}

void CMapSaverJson::saveMap(const std::unique_ptr<CMap>& map)
{
	this->map = map.get();
	this->mapHeader = this->map;
	writeHeader();
	writeTerrain();
	writeObjects();
}

void CMapSaverJson::writeHeader()
{
	JsonNode header;
	JsonSerializer handler(header);

	header["versionMajor"].Float() = VERSION_MAJOR;
	header["versionMinor"].Float() = VERSION_MINOR;

	//todo: multilevel map save support
	JsonNode & levels = header["mapLevels"];
	levels["surface"]["height"].Float() = mapHeader->height;
	levels["surface"]["width"].Float() = mapHeader->width;
	levels["surface"]["index"].Float() = 0;

	if(mapHeader->twoLevel)
	{
		levels["underground"]["height"].Float() = mapHeader->height;
		levels["underground"]["width"].Float() = mapHeader->width;
		levels["underground"]["index"].Float() = 1;
	}

	serializeHeader(handler);

	writeTriggeredEvents(handler);

	writeTeams(handler);

	writeOptions(handler);

	addToArchive(header, HEADER_FILE_NAME);
}

const std::string CMapSaverJson::writeTerrainTile(const TerrainTile & tile)
{
	using namespace TerrainDetail;

	std::ostringstream out;
	out.setf(std::ios::dec, std::ios::basefield);
	out.unsetf(std::ios::showbase);

	out << terrainCodes.at(int(tile.terType)) << (int)tile.terView << flipCodes[tile.extTileFlags % 4];

	if(tile.roadType != ERoadType::NO_ROAD)
	{
		out << roadCodes.at(int(tile.roadType)) << (int)tile.roadDir << flipCodes[(tile.extTileFlags >> 4) % 4];
	}

	if(tile.riverType != ERiverType::NO_RIVER)
	{
		out << riverCodes.at(int(tile.riverType)) << (int)tile.riverDir << flipCodes[(tile.extTileFlags >> 2) % 4];
	}

	return out.str();
}

JsonNode CMapSaverJson::writeTerrainLevel(const int index)
{
	JsonNode data;
	int3 pos(0,0,index);

	data.Vector().resize(map->height);

	for(pos.y = 0; pos.y < map->height; pos.y++)
	{
		JsonNode & row = data.Vector()[pos.y];

		row.Vector().resize(map->width);

		for(pos.x = 0; pos.x < map->width; pos.x++)
			row.Vector()[pos.x].String() = writeTerrainTile(map->getTile(pos));
	}

	return std::move(data);
}

void CMapSaverJson::writeTerrain()
{
	//todo: multilevel map save support

	JsonNode surface = writeTerrainLevel(0);
	addToArchive(surface, "surface_terrain.json");

	if(map->twoLevel)
	{
		JsonNode underground = writeTerrainLevel(1);
		addToArchive(underground, "underground_terrain.json");
	}
}

void CMapSaverJson::writeObjects()
{
	JsonNode data(JsonNode::DATA_STRUCT);

	JsonSerializer handler(data);

	for(CGObjectInstance * obj : map->objects)
	{
		auto temp = handler.enterStruct(obj->getStringId());

		obj->serializeJson(handler);
	}

	if(map->grailPos.valid())
	{
		JsonNode grail(JsonNode::DATA_STRUCT);
		grail["type"].String() = "grail";

		grail["x"].Float() = map->grailPos.x;
		grail["y"].Float() = map->grailPos.y;
		grail["l"].Float() = map->grailPos.z;

		grail["options"]["radius"].Float() = map->grailRadius;

		std::string grailId = boost::str(boost::format("grail_%d") % map->objects.size());

		data[grailId] = grail;
	}

	addToArchive(data, OBJECTS_FILE_NAME);
}

