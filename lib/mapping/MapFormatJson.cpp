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
#include "../VCMI_Lib.h"

namespace TriggeredEventsDetail{

static const std::array<std::string, 12> conditionNames = {
"haveArtifact", "haveCreatures",   "haveResources",   "haveBuilding",
"control",      "destroy",         "transport",       "daysPassed",
"isHuman",      "daysWithoutTown", "standardWin",     "constValue"
};

static const std::array<std::string, 2> typeNames = { "victory", "defeat" };

static EventCondition JsonToCondition(const JsonNode & node)
{
	EventCondition event;
	event.condition = EventCondition::EWinLoseType(vstd::find_pos(conditionNames, node.Vector()[0].String()));
	if (node.Vector().size() > 1)
	{
		const JsonNode & data = node.Vector()[1];
		if (data["type"].getType() == JsonNode::DATA_STRING)
			event.objectType = VLC->modh->identifiers.getIdentifier(data["type"]).get();
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
	
	asVector.push_back(data);
			
	return std::move(json);
}

}//namespace TriggeredEventsDetail

namespace TerrainDetail{
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

///CMapFormatJson
const int CMapFormatJson::VERSION_MAJOR = 1;
const int CMapFormatJson::VERSION_MINOR = 0;

const std::string CMapFormatJson::HEADER_FILE_NAME = "header.json";

void CMapFormatJson::readTriggeredEvents(const JsonNode & input)
{
	mapHeader->victoryMessage = input["victoryString"].String();
	mapHeader->victoryIconIndex = input["victoryIconIndex"].Float();

	mapHeader->defeatMessage = input["defeatString"].String();
	mapHeader->defeatIconIndex = input["defeatIconIndex"].Float();	
	
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

void CMapFormatJson::writeTriggeredEvents(JsonNode& output)
{
	output["victoryString"].String() = map->victoryMessage;
	output["victoryIconIndex"].Float() = map->victoryIconIndex;

	output["defeatString"].String() = map->defeatMessage;
	output["defeatIconIndex"].Float() = map->defeatIconIndex;
	
	JsonMap & triggeredEvents = output["triggeredEvents"].Struct();
	
	for(auto event : map->triggeredEvents)
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


///CMapPatcher
CMapPatcher::CMapPatcher(JsonNode stream):
	input(stream)
{

}

void CMapPatcher::patchMapHeader(std::unique_ptr<CMapHeader> & header)
{
	header.swap(mapHeader);
	if (!input.isNull())
		readPatchData();
	header.swap(mapHeader);
}

void CMapPatcher::readPatchData()
{
	readTriggeredEvents(input);
}

///CMapFormatZip
CMapFormatZip::CMapFormatZip(CInputOutputStream * stream):
	buffer(stream),
	ioApi(new CProxyIOApi(buffer))
{
	
}


///CMapLoaderJson
CMapLoaderJson::CMapLoaderJson(CInputOutputStream * stream):
	CMapFormatZip(stream),
	loader("", "_", ioApi)	
{
	
}

std::unique_ptr<CMap> CMapLoaderJson::loadMap()
{
	map = new CMap();
	mapHeader = std::unique_ptr<CMapHeader>(dynamic_cast<CMapHeader *>(map));
	readMap();
	return std::unique_ptr<CMap>(dynamic_cast<CMap *>(mapHeader.release()));
}

std::unique_ptr<CMapHeader> CMapLoaderJson::loadMapHeader()
{
	mapHeader.reset(new CMapHeader);
	readHeader();
	return std::move(mapHeader);
}

const JsonNode CMapLoaderJson::readJson(const std::string & archiveFilename)
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
	readHeader();
	map->initTerrain();
	readTerrain();
	//TODO:readMap
}

void CMapLoaderJson::readHeader()
{
	//do not use map field here, use only mapHeader
	const JsonNode header = readJson(HEADER_FILE_NAME);

	//TODO: read such data like map name & size
	//mapHeader->version = ??? //todo: new version field

	//todo: multilevel map load support	
	const JsonNode levels = header["mapLevels"];	
	mapHeader->height = levels["surface"]["height"].Float();
	mapHeader->width = levels["surface"]["width"].Float();	
	mapHeader->twoLevel = !levels["underground"].isNull();

	mapHeader->name = header["name"].String();
	mapHeader->description = header["description"].String();
	
	//todo: support arbitrary percentage
	
	static const std::map<std::string, ui8> difficultyMap =
	{
		{"", 1},
		{"EASY", 0},
		{"NORMAL", 1}, 
		{"HARD", 2}, 
		{"EXPERT", 3},
		{"IMPOSSIBLE", 4}		
	};
	
	mapHeader->difficulty = difficultyMap.at(header["difficulty"].String()); 
	mapHeader->levelLimit = header["levelLimit"].Float();
	

//	std::vector<bool> allowedHeroes;
//	std::vector<ui16> placeholdedHeroes;	
	
	readTriggeredEvents(header);
	readPlayerInfo();
	//TODO: readHeader
}

void CMapLoaderJson::readPlayerInfo()
{
	//ui8 howManyTeams;
	//TODO: readPlayerInfo
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
		const JsonNode surface = readJson("surface.json");
		readTerrainLevel(surface, 0);		
	}
	if(map->twoLevel)
	{
		const JsonNode underground = readJson("underground.json");
		readTerrainLevel(underground, 1);			
	}
		
}

///CMapSaverJson
CMapSaverJson::CMapSaverJson(CInputOutputStream * stream):
	CMapFormatZip(stream),
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
	//TODO: saveMap
	this->map = map.get();
	writeHeader();	
	writeTerrain();
}

void CMapSaverJson::writeHeader()
{
	JsonNode header;
	header["versionMajor"].Float() = VERSION_MAJOR;
	header["versionMinor"].Float() = VERSION_MINOR;	
	
	//todo: multilevel map save support	
	JsonNode & levels = header["mapLevels"];
	levels["surface"]["height"].Float() = map->height;	
	levels["surface"]["width"].Float() = map->width;
	levels["surface"]["index"].Float() = 0;
	
	if(map->twoLevel)
	{
		levels["underground"]["height"].Float() = map->height;	
		levels["underground"]["width"].Float() = map->width;	
		levels["underground"]["index"].Float() = 1;
	}
	
	header["name"].String() = map->name;
	header["description"].String() = map->description;
	
	
	//todo: support arbitrary percentage	
	static const std::map<ui8, std::string> difficultyMap =
	{
		{0, "EASY"},
		{1, "NORMAL"},
		{2, "HARD"},
		{3, "EXPERT"},
		{4, "IMPOSSIBLE"}
	};
	
	header["difficulty"].String() = difficultyMap.at(map->difficulty);	
	header["levelLimit"].Float() = map->levelLimit;
	
	writeTriggeredEvents(header);
	
	//todo: player info
	
	//todo:	allowedHeroes;
	//todo: placeholdedHeroes;	
	
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
	addToArchive(surface, "surface.json");
	
	if(map->twoLevel)
	{
		JsonNode underground = writeTerrainLevel(1);
		addToArchive(underground, "underground.json");		
	}
}
