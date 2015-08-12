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

static const std::string conditionNames[] = {
"haveArtifact", "haveCreatures",   "haveResources",   "haveBuilding",
"control",      "destroy",         "transport",       "daysPassed",
"isHuman",      "daysWithoutTown", "standardWin",     "constValue"
};

static const std::string typeNames[] = { "victory", "defeat" };

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
			event.position.x = data["position"].Vector()[0].Float();
			event.position.y = data["position"].Vector()[1].Float();
			event.position.z = data["position"].Vector()[2].Float();
		}
	}
	return event;
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
	event.onFulfill = source["message"].String();
	event.description = source["description"].String();
	event.effect.type = vstd::find_pos(typeNames, source["effect"]["type"].String());
	event.effect.toOtherMessage = source["effect"]["messageToSend"].String();
	event.trigger = EventExpression(source["condition"], JsonToCondition); // logical expression
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

/*
	//This code can be used to write map header to console or file in its Json representation

	JsonNode out;
	JsonNode data;
	data["victoryString"].String() = mapHeader->victoryMessage;
	data["defeatString"].String() = mapHeader->defeatMessage;

	data["victoryIconIndex"].Float() = mapHeader->victoryIconIndex;
	data["defeatIconIndex"].Float() = mapHeader->defeatIconIndex;

	for (const TriggeredEvent & entry : mapHeader->triggeredEvents)
	{
		JsonNode event;
		event["message"].String() = entry.onFulfill;
		event["effect"]["messageToSend"].String() = entry.effect.toOtherMessage;
		event["effect"]["type"].String() = typeNames[entry.effect.type];
		event["condition"] = entry.trigger.toJson(eventToJson);
		data["triggeredEvents"][entry.identifier] = event;
	}

	out[mapHeader->name] = data;
	logGlobal->errorStream() << out;

JsonNode eventToJson(const EventCondition & cond)
{
	JsonNode ret;
	ret.Vector().resize(2);
	ret.Vector()[0].String() = conditionNames[size_t(cond.condition)];
	JsonNode & data = ret.Vector()[1];
	data["type"].Float() = cond.objectType;
	data["value"].Float() = cond.value;
	data["position"].Vector().resize(3);
	data["position"].Vector()[0].Float() = cond.position.x;
	data["position"].Vector()[1].Float() = cond.position.y;
	data["position"].Vector()[2].Float() = cond.position.z;

	return ret;
}
*/

void CMapLoaderJson::readMap()
{
	readHeader();
	map->initTerrain();
	//TODO:readMap
}

void CMapLoaderJson::readHeader()
{
	//do not use map field here, use only mapHeader
	ResourceID headerID(HEADER_FILE_NAME, EResType::TEXT);

	if(!loader.existsResource(headerID))
		throw new std::runtime_error(HEADER_FILE_NAME+" not found");

	auto headerData = loader.load(headerID)->readAll();

	const JsonNode header(reinterpret_cast<char*>(headerData.first.get()), headerData.second);

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

///CMapSaverJson
CMapSaverJson::CMapSaverJson(CInputOutputStream * stream):
	CMapFormatZip(stream),
	saver(ioApi, "_")
{
	
}

CMapSaverJson::~CMapSaverJson()
{
	
}

void CMapSaverJson::saveMap(const std::unique_ptr<CMap>& map)
{
	//TODO: saveMap
	this->map = map.get();
	saveHeader();	
	
}

void CMapSaverJson::saveHeader()
{
	JsonNode header;
	header["versionMajor"].Float() = VERSION_MAJOR;
	header["versionMinor"].Float() = VERSION_MINOR;	
	
	//todo: multilevel map save support	
	JsonNode levels = header["mapLevels"];
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
	
	//todo:	allowedHeroes;
	//todo: placeholdedHeroes;	
	
	std::ostringstream out;
	out << header;
	out.flush();
	
	{
		auto s = out.str();
		std::unique_ptr<COutputStream> stream = saver.addFile(HEADER_FILE_NAME);
		
		stream->write((const ui8*)s.c_str(), s.size());
	}	
}
