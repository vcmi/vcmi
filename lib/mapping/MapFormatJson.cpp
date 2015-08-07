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

///CMapLoaderJson
CMapLoaderJson::CMapLoaderJson(CInputStream * stream):
	input(stream)
{
	
}

std::unique_ptr<CMap> CMapLoaderJson::loadMap()
{
	map = new CMap();
	mapHeader.reset(map);
	readMap();
	mapHeader.reset();
	return std::unique_ptr<CMap>(map);
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
	assert(0); // Not implemented, vcmi does not have its own map format right now
}

void CMapLoaderJson::readHeader()
{
	//TODO: read such data like map name & size
//	readTriggeredEvents();
	readPlayerInfo();
	assert(0); // Not implemented
}

void CMapLoaderJson::readPlayerInfo()
{
	assert(0); // Not implemented
}
