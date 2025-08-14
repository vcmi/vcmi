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
#include "../json/JsonWriter.h"
#include "CMap.h"
#include "MapFormat.h"
#include "../GameLibrary.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"
#include "../TerrainHandler.h"
#include "../entities/artifact/CArtHandler.h"
#include "../entities/faction/CTownHandler.h"
#include "../entities/hero/CHeroHandler.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../modding/ModScope.h"
#include "../modding/ModUtility.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "../constants/StringConstants.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"
#include "../texts/Languages.h"

VCMI_LIB_NAMESPACE_BEGIN

class MapObjectResolver: public IInstanceResolver
{
public:
	MapObjectResolver(const CMapFormatJson * owner_);

	si32 decode (const std::string & identifier) const override;
	std::string encode(si32 identifier) const override;

private:
	const CMapFormatJson * owner;
};

MapObjectResolver::MapObjectResolver(const CMapFormatJson * owner_):
	owner(owner_)
{

}

si32 MapObjectResolver::decode(const std::string & identifier) const
{
	//always decode as ObjectInstanceID

	auto it = owner->map->instanceNames.find(identifier);

	if(it != owner->map->instanceNames.end())
	{
		return (*it).second->id.getNum();
	}
	else
	{
		logGlobal->error("Object not found: %s", identifier);
		return -1;
	}
}

std::string MapObjectResolver::encode(si32 identifier) const
{
	ObjectInstanceID id(identifier);

	auto object = owner->map->getObject(id);

	if(!object)
	{
		logGlobal->error("Cannot get object with id %d", id.getNum());
		return "";
	}
	return object->instanceName;
}

namespace HeaderDetail
{
	static const std::vector<std::string> difficultyMap =
	{
		"EASY",
		"NORMAL",
		"HARD",
		"EXPERT",
		"IMPOSSIBLE"
	};

	enum class ECanPlay
	{
        NONE = 0,
        PLAYER_OR_AI = 1,
        PLAYER_ONLY = 2,
        AI_ONLY = 3
	};

	static const std::vector<std::string> canPlayMap =
	{
		"",
		"PlayerOrAI",
		"PlayerOnly",
		"AIOnly"
	};
}

namespace TriggeredEventsDetail
{
	static const std::array conditionNames =
	{
		"haveArtifact", "haveCreatures",   "haveResources",   "haveBuilding",
		"control",      "destroy",         "transport",       "daysPassed",
		"isHuman",      "daysWithoutTown", "standardWin",     "constValue"
	};

	static const std::array typeNames = { "victory", "defeat" };

	static EventCondition JsonToCondition(const JsonNode & node)
	{
		EventCondition event;

		const auto & conditionName = node.Vector()[0].String();

		auto pos = vstd::find_pos(conditionNames, conditionName);

		event.condition = static_cast<EventCondition::EWinLoseType>(pos);

		if (node.Vector().size() > 1)
		{
			const JsonNode & data = node.Vector()[1];

			event.objectInstanceName = data["object"].String();
			event.value = data["value"].Integer();

			switch (event.condition)
			{
				case EventCondition::HAVE_ARTIFACT:
				case EventCondition::TRANSPORT:
					if (data["type"].isNumber()) // compatibility
						event.objectType = ArtifactID(data["type"].Integer());
					else
						event.objectType = ArtifactID(ArtifactID::decode(data["type"].String()));
					break;
				case EventCondition::HAVE_CREATURES:
					if (data["type"].isNumber()) // compatibility
						event.objectType = CreatureID(data["type"].Integer());
					else
						event.objectType = CreatureID(CreatureID::decode(data["type"].String()));
					break;
				case EventCondition::HAVE_RESOURCES:
					if (data["type"].isNumber()) // compatibility
						event.objectType = GameResID(data["type"].Integer());
					else
						event.objectType = GameResID(GameResID::decode(data["type"].String()));
					break;
				case EventCondition::HAVE_BUILDING:
					if (data["type"].isNumber()) // compatibility
						event.objectType = BuildingID(data["type"].Integer());
					else
						event.objectType = BuildingID(BuildingID::decode(data["type"].String()));
					break;
				case EventCondition::CONTROL:
				case EventCondition::DESTROY:
					if (data["type"].isNumber()) // compatibility
						event.objectType = MapObjectID(data["type"].Integer());
					else
						event.objectType = MapObjectID(MapObjectID::decode(data["type"].String()));
					break;
			}

			if (!data["position"].isNull())
			{
				const auto & position = data["position"].Vector();
				event.position.x = static_cast<si32>(position.at(0).Float());
				event.position.y = static_cast<si32>(position.at(1).Float());
				event.position.z = static_cast<si32>(position.at(2).Float());
			}
		}
		return event;
	}

	static JsonNode ConditionToJson(const EventCondition & event)
	{
		JsonNode json;

		JsonVector & asVector = json.Vector();

		JsonNode condition;
		condition.String() = conditionNames.at(event.condition);
		asVector.push_back(condition);

		JsonNode data;

		if(!event.objectInstanceName.empty())
			data["object"].String() = event.objectInstanceName;

		data["type"].String() = event.objectType.toString();
		data["value"].Integer() = event.value;

		if(event.position != int3(-1, -1, -1))
		{
			auto & position = data["position"].Vector();
			position.resize(3);
			position[0].Float() = event.position.x;
			position[1].Float() = event.position.y;
			position[2].Float() = event.position.z;
		}

		if(!data.isNull())
			asVector.push_back(data);

		return json;
	}
}//namespace TriggeredEventsDetail

namespace TerrainDetail
{
	static const std::array<char, 4> flipCodes =
	{
		'_', '-', '|', '+'
	};
}

///CMapFormatJson
const int CMapFormatJson::VERSION_MAJOR = 2;
const int CMapFormatJson::VERSION_MINOR = 0;

const std::string CMapFormatJson::HEADER_FILE_NAME = "header.json";
const std::string CMapFormatJson::OBJECTS_FILE_NAME = "objects.json";

std::string getTerrainFilename(int i)
{
	if(i == 0)
		return "surface_terrain.json";
	else if(i == 1)
		return "underground_terrain.json";
	else
		return "level-" + std::to_string(i + 1) + "_terrain.json";
}

CMapFormatJson::CMapFormatJson():
	fileVersionMajor(0), fileVersionMinor(0),
	mapObjectResolver(std::make_unique<MapObjectResolver>(this)),
	map(nullptr), mapHeader(nullptr)
{

}

TerrainId CMapFormatJson::getTerrainByCode(const std::string & code)
{
	for(const auto & object : LIBRARY->terrainTypeHandler->objects)
	{
		if(object->shortIdentifier == code)
			return object->getId();
	}
	return TerrainId::NONE;
}

RiverId CMapFormatJson::getRiverByCode(const std::string & code)
{
	for(const auto & object : LIBRARY->riverTypeHandler->objects)
	{
		if (object->shortIdentifier == code)
			return object->getId();
	}
	return RiverId::NO_RIVER;
}

RoadId CMapFormatJson::getRoadByCode(const std::string & code)
{
	for(const auto & object : LIBRARY->roadTypeHandler->objects)
	{
		if (object->shortIdentifier == code)
			return object->getId();
	}
	return RoadId::NO_ROAD;
}

void CMapFormatJson::serializeAllowedFactions(JsonSerializeFormat & handler, std::set<FactionID> & value) const
{
	std::set<FactionID> temp;

	if(handler.saving)
	{
		for(auto const factionID : LIBRARY->townh->getDefaultAllowed())
			if(vstd::contains(value, factionID))
				temp.insert(factionID);
	}

	handler.serializeLIC("allowedFactions", &FactionID::decode, &FactionID::encode, LIBRARY->townh->getDefaultAllowed(), temp);

	if(!handler.saving)
		value = temp;
}

void CMapFormatJson::serializeHeader(JsonSerializeFormat & handler)
{
	handler.serializeStruct("name", mapHeader->name);
	handler.serializeStruct("description", mapHeader->description);
	handler.serializeStruct("author", mapHeader->author);
	handler.serializeStruct("authorContact", mapHeader->authorContact);
	handler.serializeStruct("mapVersion", mapHeader->mapVersion);
	handler.serializeInt("creationDateTime", mapHeader->creationDateTime, 0);
	handler.serializeInt("heroLevelLimit", mapHeader->levelLimit, 0);

	//todo: support arbitrary percentage
	handler.serializeEnum("difficulty", mapHeader->difficulty, HeaderDetail::difficultyMap);

	serializePlayerInfo(handler);

	handler.serializeLIC("allowedHeroes", &HeroTypeID::decode, &HeroTypeID::encode, LIBRARY->heroh->getDefaultAllowed(), mapHeader->allowedHeroes);

	handler.serializeStruct("victoryMessage", mapHeader->victoryMessage);
	handler.serializeInt("victoryIconIndex", mapHeader->victoryIconIndex);

	handler.serializeStruct("defeatMessage", mapHeader->defeatMessage);
	handler.serializeInt("defeatIconIndex", mapHeader->defeatIconIndex);

	handler.serializeIdArray("reservedCampaignHeroes", mapHeader->reservedCampaignHeroes);
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

		auto playerData = handler.enterStruct(GameConstants::PLAYER_COLOR_NAMES[player]);

		if(!handler.saving)
		{
			if(handler.getCurrent().isNull())
			{
				info.canComputerPlay = false;
				info.canHumanPlay = false;
				continue;
			}
		}

		serializeAllowedFactions(handler, info.allowedFactions);

		HeaderDetail::ECanPlay canPlay = HeaderDetail::ECanPlay::NONE;

		if(handler.saving)
		{
			if(info.canComputerPlay)
			{
				canPlay = info.canHumanPlay ? HeaderDetail::ECanPlay::PLAYER_OR_AI : HeaderDetail::ECanPlay::AI_ONLY;
			}
			else
			{
				canPlay = info.canHumanPlay ? HeaderDetail::ECanPlay::PLAYER_ONLY : HeaderDetail::ECanPlay::NONE;
			}
		}

		handler.serializeEnum("canPlay", canPlay, HeaderDetail::canPlayMap);

		if(!handler.saving)
		{
			switch(canPlay)
			{
			case HeaderDetail::ECanPlay::PLAYER_OR_AI:
				info.canComputerPlay = true;
				info.canHumanPlay = true;
				break;
			case HeaderDetail::ECanPlay::PLAYER_ONLY:
				info.canComputerPlay = false;
				info.canHumanPlay = true;
				break;
			case HeaderDetail::ECanPlay::AI_ONLY:
				info.canComputerPlay = true;
				info.canHumanPlay = false;
				break;
            default:
				info.canComputerPlay = false;
				info.canHumanPlay = false;
				break;
			}
		}

		//saving whole structure only if position is valid
		if(!handler.saving || info.posOfMainTown.isValid())
		{
			auto mainTown = handler.enterStruct("mainTown");
			handler.serializeBool("generateHero", info.generateHeroAtMainTown);
			handler.serializeInt("x", info.posOfMainTown.x, -1);
			handler.serializeInt("y", info.posOfMainTown.y, -1);
			handler.serializeInt("l", info.posOfMainTown.z, -1);
		}
		if(!handler.saving)
		{
			info.hasMainTown = info.posOfMainTown.isValid();
		}

		handler.serializeString("mainHero", info.mainHeroInstance);//must be before "heroes"

		//heroes
		if(handler.saving)
		{
			//ignoring heroesNames and saving from actual map objects
			//TODO: optimize
			for(auto & hero : map->getObjects<CGHeroInstance>())
			{
				if((hero->getOwner()) == PlayerColor(player))
				{
					auto heroes = handler.enterStruct("heroes");
					if(hero)
					{
						auto heroData = handler.enterStruct(hero->instanceName);
						heroData->serializeString("name", hero->nameCustomTextId);

						if(hero->ID == Obj::HERO)
						{
							std::string temp;
							if(hero->getHeroTypeID().hasValue())
								temp = hero->getHeroType()->getJsonKey();

							handler.serializeString("type", temp);
						}
					}
				}
			}
		}
		else
		{
			info.heroesNames.clear();

			auto heroes = handler.enterStruct("heroes");

			for(const auto & hero : handler.getCurrent().Struct())
			{
				const JsonNode & data = hero.second;
				const std::string instanceName = hero.first;

				SHeroName hname;
				hname.heroId = HeroTypeID::NONE;
				std::string rawId = data["type"].String();

				if(!rawId.empty())
					hname.heroId = HeroTypeID(HeroTypeID::decode(rawId));

				hname.heroName = data["name"].String();

				if(instanceName == info.mainHeroInstance)
				{
					//this is main hero
					info.mainCustomHeroNameTextId = hname.heroName;
					info.hasRandomHero = (hname.heroId == HeroTypeID::NONE);
					info.mainCustomHeroId = hname.heroId;
					info.mainCustomHeroPortrait = HeroTypeID::NONE;
					//todo:mainHeroPortrait
				}

                info.heroesNames.push_back(hname);
			}
		}

		handler.serializeBool("randomFaction", info.isFactionRandom);
	}
}

void CMapFormatJson::readTeams(JsonDeserializer & handler)
{
	auto teams = handler.enterArray("teams");
	const JsonNode & src = teams->getCurrent();

	if(src.getType() != JsonNode::JsonType::DATA_VECTOR)
	{
		// No alliances
		if(src.getType() != JsonNode::JsonType::DATA_NULL)
			logGlobal->error("Invalid teams field type");

		mapHeader->howManyTeams = 0;
		for(auto & player : mapHeader->players)
			if(player.canAnyonePlay())
				player.team = TeamID(mapHeader->howManyTeams++);
	}
	else
	{
		const JsonVector & srcVector = src.Vector();
		mapHeader->howManyTeams = static_cast<ui8>(srcVector.size());

		for(int team = 0; team < mapHeader->howManyTeams; team++)
			for(const JsonNode & playerData : srcVector[team].Vector())
			{
				PlayerColor player = PlayerColor(vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, playerData.String()));
				if(player.isValidPlayer())
					if(mapHeader->players[player.getNum()].canAnyonePlay())
						mapHeader->players[player.getNum()].team = TeamID(team);
			}

		for(PlayerInfo & player : mapHeader->players)
			if(player.canAnyonePlay() && player.team == TeamID::NO_TEAM)
				player.team = TeamID(mapHeader->howManyTeams++);
	}
}

void CMapFormatJson::writeTeams(JsonSerializer & handler)
{
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

	if(!teamsData.empty())
	{
		JsonNode dest;

		//construct output
		dest.setType(JsonNode::JsonType::DATA_VECTOR);

		for(const std::set<PlayerColor> & teamData : teamsData)
		{
			JsonNode team;
			for(const PlayerColor & player : teamData)
				team.Vector().emplace_back(GameConstants::PLAYER_COLOR_NAMES[player.getNum()]);

			dest.Vector().push_back(std::move(team));
		}
		handler.serializeRaw("teams", dest, std::nullopt);
	}
}

void CMapFormatJson::readTriggeredEvents(JsonDeserializer & handler)
{
	const JsonNode & input = handler.getCurrent();

	mapHeader->triggeredEvents.clear();

	for(const auto & entry : input["triggeredEvents"].Struct())
	{
		TriggeredEvent event;
		event.identifier = entry.first;
		readTriggeredEvent(event, entry.second);
		mapHeader->triggeredEvents.push_back(event);
	}
}

void CMapFormatJson::readTriggeredEvent(TriggeredEvent & event, const JsonNode & source) const
{
	using namespace TriggeredEventsDetail;

	event.onFulfill.jsonDeserialize(source["message"]);
	event.description.jsonDeserialize(source["description"]);
	event.effect.type = vstd::find_pos(typeNames, source["effect"]["type"].String());
	event.effect.toOtherMessage.jsonDeserialize(source["effect"]["messageToSend"]);
	event.trigger = EventExpression(source["condition"], JsonToCondition); // logical expression
}

void CMapFormatJson::writeTriggeredEvents(JsonSerializer & handler)
{
	JsonNode triggeredEvents;

	for(const auto & event : mapHeader->triggeredEvents)
		writeTriggeredEvent(event, triggeredEvents[event.identifier]);

	handler.serializeRaw("triggeredEvents", triggeredEvents, std::nullopt);
}

void CMapFormatJson::writeTriggeredEvent(const TriggeredEvent & event, JsonNode & dest) const
{
	using namespace TriggeredEventsDetail;

	if(!event.onFulfill.empty())
		event.onFulfill.jsonSerialize(dest["message"]);

	if(!event.description.empty())
		event.description.jsonSerialize(dest["description"]);

	dest["effect"]["type"].String() = typeNames.at(static_cast<size_t>(event.effect.type));

	if(!event.effect.toOtherMessage.empty())
		event.description.jsonSerialize(dest["effect"]["messageToSend"]);

	dest["condition"] = event.trigger.toJson(ConditionToJson);
}

void CMapFormatJson::readDisposedHeroes(JsonSerializeFormat & handler)
{
	auto definitions = handler.enterStruct("predefinedHeroes");//DisposedHeroes are part of predefinedHeroes in VCMI map format

	const JsonNode & data = handler.getCurrent();

	for(const auto & entry : data.Struct())
	{
		HeroTypeID type(HeroTypeID::decode(entry.first));

		std::set<PlayerColor> mask;

		for(const JsonNode & playerData : entry.second["availableFor"].Vector())
		{
			PlayerColor player = PlayerColor(vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, playerData.String()));
			if(player.isValidPlayer())
				mask.insert(player);
		}

		if(!mask.empty() && mask.size() != PlayerColor::PLAYER_LIMIT_I && type.getNum() >= 0)
		{
			DisposedHero hero;

			hero.heroId = type;
			hero.players = mask;
			//name and portrait are not used

			mapHeader->disposedHeroes.push_back(hero);
		}
	}
}

void CMapFormatJson::writeDisposedHeroes(JsonSerializeFormat & handler)
{
	if(mapHeader->disposedHeroes.empty())
		return;

	auto definitions = handler.enterStruct("predefinedHeroes");//DisposedHeroes are part of predefinedHeroes in VCMI map format

	for(DisposedHero & hero : mapHeader->disposedHeroes)
	{
		std::string type = HeroTypeID::encode(hero.heroId.getNum());

		auto definition = definitions->enterStruct(type);

		JsonNode players;
		definition->serializeIdArray("availableFor", hero.players);
	}
}

void CMapFormatJson::serializeRumors(JsonSerializeFormat & handler)
{
	auto rumors = handler.enterArray("rumors");
	rumors.serializeStruct(map->rumors);
}

void CMapFormatJson::serializeTimedEvents(JsonSerializeFormat & handler)
{
	auto events = handler.enterArray("events");
	std::vector<CMapEvent> temp(map->events.begin(), map->events.end());
	events.serializeStruct(temp);
	map->events.assign(temp.begin(), temp.end());
}

void CMapFormatJson::serializePredefinedHeroes(JsonSerializeFormat & handler)
{
	if(handler.saving)
	{
		auto heroPool = map->getHeroesInPool();
		if(!heroPool.empty())
		{
			auto predefinedHeroes = handler.enterStruct("predefinedHeroes");

			for(auto & heroID : heroPool)
			{
				auto heroPtr = map->tryGetFromHeroPool(heroID);
				auto predefinedHero = handler.enterStruct(heroPtr->getHeroTypeName());

				heroPtr->serializeJsonDefinition(handler);
			}
		}
	}
	else
	{
		auto predefinedHeroes = handler.enterStruct("predefinedHeroes");

		const JsonNode & data = handler.getCurrent();

		for(const auto & p : data.Struct())
		{
			auto predefinedHero = handler.enterStruct(p.first);

			auto hero = std::make_shared<CGHeroInstance>(map->cb);
			hero->ID = Obj::HERO;
			hero->setHeroTypeName(p.first);
			hero->serializeJsonDefinition(handler);

			map->addToHeroPool(hero);
		}
	}
}

void CMapFormatJson::serializeOptions(JsonSerializeFormat & handler)
{
	serializeRumors(handler);
	
	serializeTimedEvents(handler);

	serializePredefinedHeroes(handler);

	handler.serializeLIC("allowedAbilities", &SecondarySkill::decode, &SecondarySkill::encode, LIBRARY->skillh->getDefaultAllowed(), map->allowedAbilities);

	handler.serializeLIC("allowedArtifacts",  &ArtifactID::decode, &ArtifactID::encode, LIBRARY->arth->getDefaultAllowed(), map->allowedArtifact);

	handler.serializeLIC("allowedSpells", &SpellID::decode, &SpellID::encode, LIBRARY->spellh->getDefaultAllowed(), map->allowedSpells);

	//todo:events
}

void CMapFormatJson::readOptions(JsonDeserializer & handler)
{
	readDisposedHeroes(handler);
	serializeOptions(handler);
}

void CMapFormatJson::writeOptions(JsonSerializer & handler)
{
	writeDisposedHeroes(handler);
	serializeOptions(handler);
}

///CMapPatcher
CMapPatcher::CMapPatcher(const JsonNode & stream): input(stream)
{
	//todo: update map patches and change this
	fileVersionMajor = 0;
	fileVersionMinor = 0;
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
	JsonDeserializer handler(mapObjectResolver.get(), input);
	readTriggeredEvents(handler);

	handler.serializeInt("defeatIconIndex", mapHeader->defeatIconIndex);
	handler.serializeInt("victoryIconIndex", mapHeader->victoryIconIndex);
	handler.serializeStruct("victoryString", mapHeader->victoryMessage);
	handler.serializeStruct("defeatString", mapHeader->defeatMessage);
}

///CMapLoaderJson
CMapLoaderJson::CMapLoaderJson(CInputStream * stream)
	: buffer(stream)
	, ioApi(new CProxyROIOApi(buffer))
	, loader("", "_", ioApi)
{
}

std::unique_ptr<CMap> CMapLoaderJson::loadMap(IGameInfoCallback * cb)
{
	LOG_TRACE(logGlobal);
	auto result = std::make_unique<CMap>(cb);
	map = result.get();
	mapHeader = map;
	readMap();
	return result;
}

std::unique_ptr<CMapHeader> CMapLoaderJson::loadMapHeader()
{
	LOG_TRACE(logGlobal);
	map = nullptr;
	auto result = std::make_unique<CMapHeader>();
	mapHeader = result.get();
	readHeader(false);
	return result;
}

bool CMapLoaderJson::isExistArchive(const std::string & archiveFilename)
{
	return loader.existsResource(JsonPath::builtin(archiveFilename));
}

JsonNode CMapLoaderJson::getFromArchive(const std::string & archiveFilename)
{
	JsonPath resource = JsonPath::builtin(archiveFilename);

	if(!loader.existsResource(resource))
		throw std::runtime_error(archiveFilename+" not found");

	auto data = loader.load(resource)->readAll();

	JsonNode res(reinterpret_cast<const std::byte*>(data.first.get()), data.second, archiveFilename);

	return res;
}

void CMapLoaderJson::readMap()
{
	LOG_TRACE(logGlobal);
	readHeader(true);
	map->initTerrain();
	readTerrain();
	readObjects();

	map->calculateGuardingGreaturePositions();
}

void CMapLoaderJson::readHeader(const bool complete)
{
	//do not use map field here, use only mapHeader
	JsonNode header = getFromArchive(HEADER_FILE_NAME);

	fileVersionMajor = static_cast<int>(header["versionMajor"].Integer());

	if(fileVersionMajor > VERSION_MAJOR)
	{
		logGlobal->error("Unsupported map format version: %d", fileVersionMajor);
		throw std::runtime_error("Unsupported map format version");
	}

	fileVersionMinor = static_cast<int>(header["versionMinor"].Integer());

	if(fileVersionMinor > VERSION_MINOR)
	{
		logGlobal->warn("Too new map format revision: %d. This map should work but some of map features may be ignored.", fileVersionMinor);
	}

	JsonDeserializer handler(mapObjectResolver.get(), header);

	mapHeader->version = EMapFormat::VCMI;//todo: new version field
	
	//loading mods
	mapHeader->mods = ModVerificationInfo::jsonDeserializeList(header["mods"]);

	{
		auto levels = handler.enterStruct("mapLevels");
		{
			auto surface = handler.enterStruct("surface");
			handler.serializeInt("height", mapHeader->height);
			handler.serializeInt("width", mapHeader->width);
		}
		mapHeader->mapLevels = levels->getCurrent().Struct().size();
	}

	serializeHeader(handler);

	readTriggeredEvents(handler);

	readTeams(handler);
	//TODO: check mods

	if(complete)
		readOptions(handler);
	
	readTranslations();
}

void CMapLoaderJson::readTerrainTile(const std::string & src, TerrainTile & tile)
{
	try
	{
		using namespace TerrainDetail;
		{//terrain type
			const std::string typeCode = src.substr(0, 2);
			tile.terrainType = getTerrainByCode(typeCode);
		}
		int startPos = 2; //0+typeCode fixed length
		{//terrain view
			int pos = startPos;
			while (isdigit(src.at(pos)))
				pos++;
			int len = pos - startPos;
			if (len <= 0)
				throw std::runtime_error("Invalid terrain view in " + src);
			const std::string rawCode = src.substr(startPos, len);
			tile.terView = atoi(rawCode.c_str());
			startPos += len;
		}
		{//terrain flip
			int terrainFlip = vstd::find_pos(flipCodes, src.at(startPos++));
			if (terrainFlip < 0)
				throw std::runtime_error("Invalid terrain flip in " + src);
			else
				tile.extTileFlags = terrainFlip;
		}
		if (startPos >= src.size())
			return;
		bool hasRoad = true;
		{//road type
			const std::string typeCode = src.substr(startPos, 2);
			startPos += 2;
			tile.roadType = getRoadByCode(typeCode);
			if(!tile.roadType) //it's not a road, it's a river
			{
				tile.roadType = Road::NO_ROAD;
				tile.riverType = getRiverByCode(typeCode);
				hasRoad = false;
				if(!tile.riverType)
				{
					throw std::runtime_error("Invalid river type in " + src);
				}
			}
		}
		if (hasRoad)
		{//road dir
			int pos = startPos;
			while (isdigit(src.at(pos)))
				pos++;
			int len = pos - startPos;
			if (len <= 0)
				throw std::runtime_error("Invalid road dir in " + src);
			const std::string rawCode = src.substr(startPos, len);
			tile.roadDir = atoi(rawCode.c_str());
			startPos += len;
		}
		if (hasRoad)
		{//road flip
			int flip = vstd::find_pos(flipCodes, src.at(startPos++));
			if (flip < 0)
				throw std::runtime_error("Invalid road flip in " + src);
			else
				tile.extTileFlags |= (flip << 4);
		}
		if (startPos >= src.size())
			return;
		if (hasRoad)
		{//river type
			const std::string typeCode = src.substr(startPos, 2);
			startPos += 2;
			tile.riverType = getRiverByCode(typeCode);
		}
		{//river dir
			int pos = startPos;
			while (isdigit(src.at(pos)))
				pos++;
			int len = pos - startPos;
			if (len <= 0)
				throw std::runtime_error("Invalid river dir in " + src);
			const std::string rawCode = src.substr(startPos, len);
			tile.riverDir = atoi(rawCode.c_str());
			startPos += len;
		}
		{//river flip
			int flip = vstd::find_pos(flipCodes, src.at(startPos++));
			if (flip < 0)
				throw std::runtime_error("Invalid road flip in " + src);
			else
				tile.extTileFlags |= (flip << 2);
		}
	}
	catch (const std::exception &)
	{
		logGlobal->error("Failed to read terrain tile: %s");
	}
}

void CMapLoaderJson::readTerrainLevel(const JsonNode & src, const int index)
{
	int3 pos(0, 0, index);

	const JsonVector & rows = src.Vector();

	if(rows.size() != map->height)
		throw std::runtime_error("Invalid terrain data");

	for(pos.y = 0; pos.y < map->height; pos.y++)
	{
		const JsonVector & tiles = rows[pos.y].Vector();

		if(tiles.size() != map->width)
			throw std::runtime_error("Invalid terrain data");

		for(pos.x = 0; pos.x < map->width; pos.x++)
			readTerrainTile(tiles[pos.x].String(), map->getTile(pos));
	}
}

void CMapLoaderJson::readTerrain()
{
	for(int i = 0; i < map->mapLevels; i++)
	{
		const JsonNode node = getFromArchive(getTerrainFilename(i));
		readTerrainLevel(node, i);
	}
}

CMapLoaderJson::MapObjectLoader::MapObjectLoader(CMapLoaderJson * _owner, JsonMap::value_type & json):
	owner(_owner), instance(nullptr), id(-1), jsonKey(json.first), configuration(json.second)
{

}

void CMapLoaderJson::MapObjectLoader::construct()
{
	//TODO:consider move to ObjectTypeHandler
	//find type handler
	std::string typeName = configuration["type"].String();
	std::string subtypeName = configuration["subtype"].String();
	if(typeName.empty())
	{
		logGlobal->error("Object type missing");
		logGlobal->debug(configuration.toString());
		return;
	}

	int3 pos;
	pos.x = static_cast<si32>(configuration["x"].Float());
	pos.y = static_cast<si32>(configuration["y"].Float());
	pos.z = static_cast<si32>(configuration["l"].Float());

	//special case for grail
	if(typeName == "grail")
	{
		owner->map->grailPos = pos;

		owner->map->grailRadius = static_cast<int>(configuration["options"]["grailRadius"].Float());
		return;
	}
	else if(subtypeName.empty())
	{
		logGlobal->error("Object subtype missing");
		logGlobal->debug(configuration.toString());
		return;
	}

	auto handler = LIBRARY->objtypeh->getHandlerFor( ModScope::scopeMap(), typeName, subtypeName);

	auto appearance = std::make_shared<ObjectTemplate>();

	appearance->id = Obj(handler->getIndex());
	appearance->subid = handler->getSubIndex();
	appearance->readJson(configuration["template"], false);

	// Will be destroyed soon and replaced with shared template
	instance = handler->create(owner->map->cb, appearance);

	instance->instanceName = jsonKey;
	instance->setAnchorPos(pos);
	owner->map->addNewObject(instance);
}

void CMapLoaderJson::MapObjectLoader::configure()
{
	if(nullptr == instance)
		return;

	JsonDeserializer handler(owner->mapObjectResolver.get(), configuration);

	instance->serializeJson(handler);

	//artifact instance serialization requires access to Map object, handle it here for now
	//todo: find better solution for artifact instance serialization

	if(auto art = std::dynamic_pointer_cast<CGArtifact>(instance))
	{
		ArtifactID artID = ArtifactID::NONE;
		SpellID spellID = SpellID::NONE;

		if(art->ID == Obj::SPELL_SCROLL)
		{
			auto spellIdentifier = configuration["options"]["spell"].String();
			auto rawId = LIBRARY->identifiers()->getIdentifier(ModScope::scopeBuiltin(), "spell", spellIdentifier);
			if(rawId)
				spellID = rawId.value();
			else
				spellID = 0;
			artID = ArtifactID::SPELL_SCROLL;
		}
		else if (art->ID == Obj::ARTIFACT || (art->ID >= Obj::RANDOM_ART && art->ID <= Obj::RANDOM_RELIC_ART))
		{
			//specific artifact
			artID = art->getArtifactType();
		}

		art->setArtifactInstance(owner->map->createArtifact(artID, spellID.getNum()));
 	}

	if(auto hero = std::dynamic_pointer_cast<CGHeroInstance>(instance))
	{
		auto o = handler.enterStruct("options");
		hero->serializeJsonArtifacts(handler, "artifacts", owner->map);
	}
}

void CMapLoaderJson::readObjects()
{
	LOG_TRACE(logGlobal);

	std::vector<std::unique_ptr<MapObjectLoader>> loaders;//todo: optimize MapObjectLoader memory layout

	JsonNode data = getFromArchive(OBJECTS_FILE_NAME);

	//get raw data
	for(auto & p : data.Struct())
		loaders.push_back(std::make_unique<MapObjectLoader>(this, p));

	for(auto & ptr : loaders)
		ptr->construct();

	//configure objects after all objects are constructed so we may resolve internal IDs even to actual pointers OTF
	for(auto & ptr : loaders)
		ptr->configure();

	std::set<HeroTypeID> debugHeroesOnMap;
	for (auto const & hero : map->getObjects<CGHeroInstance>())
	{
		if(hero->ID != Obj::HERO && hero->ID != Obj::PRISON)
			continue;

		if (debugHeroesOnMap.count(hero->getHeroTypeID()))
			logGlobal->error("Hero is already on the map at %s", hero->visitablePos().toString());

		debugHeroesOnMap.insert(hero->getHeroTypeID());
	}
	map->parseUidCounter();
}

void CMapLoaderJson::readTranslations()
{
	std::list<Languages::Options> languages{Languages::getLanguageList().begin(), Languages::getLanguageList().end()};
	for(auto & language : Languages::getLanguageList())
	{
		if(isExistArchive(language.identifier + ".json"))
			mapHeader->translations.Struct()[language.identifier] = getFromArchive(language.identifier + ".json");
	}
	mapHeader->registerMapStrings();
}


///CMapSaverJson
CMapSaverJson::CMapSaverJson(CInputOutputStream * stream)
	: buffer(stream)
	, ioApi(new CProxyIOApi(buffer))
	, saver(ioApi, "_")
{
	fileVersionMajor = VERSION_MAJOR;
	fileVersionMinor = VERSION_MINOR;
}

//must be instantiated in .cpp file for access to complete types of all member fields
CMapSaverJson::~CMapSaverJson() = default;

void CMapSaverJson::addToArchive(const JsonNode & data, const std::string & filename)
{
	std::ostringstream out;
	JsonWriter writer(out, false);
	writer.writeNode(data);
	out.flush();

	{
		auto s = out.str();
		std::unique_ptr<COutputStream> stream = saver.addFile(filename);

		if(stream->write(reinterpret_cast<const ui8 *>(s.c_str()), s.size()) != s.size())
			throw std::runtime_error("CMapSaverJson::saveHeader() zip compression failed.");
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
	logGlobal->trace("Saving header");

	JsonNode header;
	JsonSerializer handler(mapObjectResolver.get(), header);

	header["versionMajor"].Float() = VERSION_MAJOR;
	header["versionMinor"].Float() = VERSION_MINOR;
	
	//write mods
	header["mods"] = ModVerificationInfo::jsonSerializeList(mapHeader->mods);

	auto getName = [](int level){
		if(level == 0)
			return std::string("surface");
		else if(level == 1)
			return std::string("underground");
		else
			return "level-" + std::to_string(level + 1);
	};

	JsonNode & levels = header["mapLevels"];
	for(int i = 0; i < map->mapLevels; i++)
	{
		auto name = getName(i);
		levels[name]["height"].Float() = mapHeader->height;
		levels[name]["width"].Float() = mapHeader->width;
		levels[name]["index"].Float() = i;
	}

	serializeHeader(handler);

	writeTriggeredEvents(handler);

	writeTeams(handler);

	writeOptions(handler);

	writeTranslations();

	addToArchive(header, HEADER_FILE_NAME);
}

std::string CMapSaverJson::writeTerrainTile(const TerrainTile & tile)
{
	using namespace TerrainDetail;

	std::ostringstream out;
	out.setf(std::ios::dec, std::ios::basefield);
	out.unsetf(std::ios::showbase);

	out << tile.getTerrain()->shortIdentifier << static_cast<int>(tile.terView) << flipCodes[tile.extTileFlags % 4];

	if(tile.hasRoad())
		out << tile.getRoad()->shortIdentifier << static_cast<int>(tile.roadDir) << flipCodes[(tile.extTileFlags >> 4) % 4];

	if(tile.hasRiver())
		out << tile.getRiver()->shortIdentifier << static_cast<int>(tile.riverDir) << flipCodes[(tile.extTileFlags >> 2) % 4];

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

	return data;
}

void CMapSaverJson::writeTerrain()
{
	logGlobal->trace("Saving terrain");

	for(int i = 0; i < map->mapLevels; i++)
	{
		JsonNode node = writeTerrainLevel(i);
		addToArchive(node, getTerrainFilename(i));
	}
}

void CMapSaverJson::writeObjects()
{
	logGlobal->trace("Saving objects");
	JsonNode data;

	JsonSerializer handler(mapObjectResolver.get(), data);

	for(const auto & obj : map->getObjects())
	{
		//logGlobal->trace("\t%s", obj->instanceName);
		auto temp = handler.enterStruct(obj->instanceName);

		obj->serializeJson(handler);
	}

	if(map->grailPos.isValid())
	{
		JsonNode grail;
		grail["type"].String() = "grail";

		grail["x"].Float() = map->grailPos.x;
		grail["y"].Float() = map->grailPos.y;
		grail["l"].Float() = map->grailPos.z;

		grail["options"]["radius"].Float() = map->grailRadius;

		data["grail"] = grail;
	}

	//cleanup empty options
	for(auto & p : data.Struct())
	{
		JsonNode & obj = p.second;
		if(obj["options"].Struct().empty())
			obj.Struct().erase("options");
	}

	addToArchive(data, OBJECTS_FILE_NAME);
}

void CMapSaverJson::writeTranslations()
{
	for(auto & s : mapHeader->translations.Struct())
	{
		auto & language = s.first;
		if(Languages::getLanguageOptions(language).identifier.empty())
		{
			logGlobal->error("Serializing of unsupported language %s is not permitted", language);
			continue;
		}
		logGlobal->trace("Saving translations, language: %s", language);
		addToArchive(s.second, language + ".json");
	}
}

VCMI_LIB_NAMESPACE_END
