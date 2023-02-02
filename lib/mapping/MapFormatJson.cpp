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
#include "../JsonDetail.h"
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
#include "../CSkillHandler.h"
#include "../StringConstants.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

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
	ObjectInstanceID id;

	//use h3m questIdentifiers if they are present
	if(owner->map->questIdentifierToId.empty())
	{
		id = ObjectInstanceID(identifier);
	}
	else
	{
		id = owner->map->questIdentifierToId[identifier];
	}

	si32 oid = id.getNum();
	if(oid < 0  ||  oid >= owner->map->objects.size())
	{
        logGlobal->error("Cannot get object with id %d", oid);
		return "";
	}

	return owner->map->objects[oid]->instanceName;
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
	static const std::array<std::string, 15> conditionNames =
	{
		"haveArtifact", "haveCreatures",   "haveResources",   "haveBuilding",
		"control",      "destroy",         "transport",       "daysPassed",
		"isHuman",      "daysWithoutTown", "standardWin",     "constValue",

		"have_0", "haveBuilding_0", "destroy_0"
	};

	static const std::array<std::string, 2> typeNames = { "victory", "defeat" };

	static EMetaclass decodeMetaclass(const std::string & source)
	{
		if(source.empty())
			return EMetaclass::INVALID;
		auto rawId = vstd::find_pos(NMetaclass::names, source);

		if(rawId >= 0)
			return (EMetaclass)rawId;
		else
			return EMetaclass::INVALID;
	}

	static std::string encodeIdentifier(EMetaclass metaType, si32 type)
	{
		std::string metaclassName = NMetaclass::names[(int)metaType];
		std::string identifier = "";

		switch(metaType)
		{
		case EMetaclass::ARTIFACT:
			{
				identifier = ArtifactID::encode(type);
			}
			break;
		case EMetaclass::CREATURE:
			{
				identifier = CreatureID::encode(type);
			}
			break;
		case EMetaclass::OBJECT:
			{
				//TODO
				std::set<si32> subtypes = VLC->objtypeh->knownSubObjects(type);
				if(!subtypes.empty())
				{
					si32 subtype = *subtypes.begin();
					auto handler = VLC->objtypeh->getHandlerFor(type, subtype);
					identifier = handler->typeName;
				}
			}
			break;
		case EMetaclass::RESOURCE:
			{
				identifier = GameConstants::RESOURCE_NAMES[type];
			}
			break;
		default:
			{
				logGlobal->error("Unsupported metaclass %s for event condition", metaclassName);
				return "";
			}
			break;
		}

		return VLC->modh->makeFullIdentifier("", metaclassName, identifier);
	}

	static EventCondition JsonToCondition(const JsonNode & node)
	{
		EventCondition event;

		const auto & conditionName = node.Vector()[0].String();

		auto pos = vstd::find_pos(conditionNames, conditionName);

		event.condition = EventCondition::EWinLoseType(pos);

		if (node.Vector().size() > 1)
		{
			const JsonNode & data = node.Vector()[1];

			switch (event.condition)
			{
			case EventCondition::HAVE_0:
			case EventCondition::DESTROY_0:
				{
					//todo: support subtypes

					std::string fullIdentifier = data["type"].String(), metaTypeName = "", scope = "" , identifier = "";
					CModHandler::parseIdentifier(fullIdentifier, scope, metaTypeName, identifier);

					event.metaType = decodeMetaclass(metaTypeName);

					auto type = VLC->modh->identifiers.getIdentifier(CModHandler::scopeBuiltin(), fullIdentifier, false);

					if(type)
						event.objectType = type.get();
					event.objectInstanceName = data["object"].String();
					if(data["value"].isNumber())
						event.value = static_cast<si32>(data["value"].Integer());
				}
				break;
			case EventCondition::HAVE_BUILDING_0:
				{
					//todo: support of new condition format HAVE_BUILDING_0
				}
				break;
			default:
				{
					//old format
					if (data["type"].getType() == JsonNode::JsonType::DATA_STRING)
					{
						auto identifier = VLC->modh->identifiers.getIdentifier(data["type"]);
						if(identifier)
							event.objectType = identifier.get();
						else
							throw std::runtime_error("Identifier resolution failed in event condition");
					}

					if (data["type"].isNumber())
						event.objectType = static_cast<si32>(data["type"].Float());

					if (!data["value"].isNull())
						event.value = static_cast<si32>(data["value"].Float());
				}
				break;
			}

			if (!data["position"].isNull())
			{
				auto & position = data["position"].Vector();
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

		switch (event.condition)
		{
		case EventCondition::HAVE_0:
		case EventCondition::DESTROY_0:
			{
				//todo: support subtypes

				if(event.metaType != EMetaclass::INVALID)
					data["type"].String() = encodeIdentifier(event.metaType, event.objectType);

				if(event.value > 0)
					data["value"].Integer() = event.value;

				if(!event.objectInstanceName.empty())
					data["object"].String() = event.objectInstanceName;
			}
			break;
		case EventCondition::HAVE_BUILDING_0:
			{
			//todo: support of new condition format HAVE_BUILDING_0
			}
			break;
		default:
			{
				//old format
				if(event.objectType != -1)
					data["type"].Integer() = event.objectType;

				if(event.value != -1)
					data["value"].Integer() = event.value;
			}
			break;
		}

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
const int CMapFormatJson::VERSION_MAJOR = 1;
const int CMapFormatJson::VERSION_MINOR = 0;

const std::string CMapFormatJson::HEADER_FILE_NAME = "header.json";
const std::string CMapFormatJson::OBJECTS_FILE_NAME = "objects.json";

CMapFormatJson::CMapFormatJson():
	fileVersionMajor(0), fileVersionMinor(0),
	mapObjectResolver(make_unique<MapObjectResolver>(this)),
	map(nullptr), mapHeader(nullptr)
{

}

void CMapFormatJson::serializeAllowedFactions(JsonSerializeFormat & handler, std::set<TFaction> & value)
{
	//TODO: unify allowed factions with others - make them std::vector<bool>

	std::vector<bool> temp;
	temp.resize(VLC->townh->size(), false);
	auto standard = VLC->townh->getDefaultAllowed();

    if(handler.saving)
	{
		for(auto faction : VLC->townh->objects)
			if(faction->town && vstd::contains(value, faction->index))
				temp[std::size_t(faction->index)] = true;
	}

	handler.serializeLIC("allowedFactions", &FactionID::decode, &FactionID::encode, standard, temp);

	if(!handler.saving)
	{
		value.clear();
		for (std::size_t i=0; i<temp.size(); i++)
			if(temp[i])
				value.insert((TFaction)i);
	}
}

void CMapFormatJson::serializeHeader(JsonSerializeFormat & handler)
{
	handler.serializeString("name", mapHeader->name);
	handler.serializeString("description", mapHeader->description);
	handler.serializeInt("heroLevelLimit", mapHeader->levelLimit, 0);

	//todo: support arbitrary percentage
	handler.serializeEnum("difficulty", mapHeader->difficulty, HeaderDetail::difficultyMap);

	serializePlayerInfo(handler);

	handler.serializeLIC("allowedHeroes", &HeroTypeID::decode, &HeroTypeID::encode, VLC->heroh->getDefaultAllowed(), mapHeader->allowedHeroes);

	handler.serializeString("victoryString", mapHeader->victoryMessage);
	handler.serializeInt("victoryIconIndex", mapHeader->victoryIconIndex);

	handler.serializeString("defeatString", mapHeader->defeatMessage);
	handler.serializeInt("defeatIconIndex", mapHeader->defeatIconIndex);
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
		if(!handler.saving || info.posOfMainTown.valid())
		{
			auto mainTown = handler.enterStruct("mainTown");
			handler.serializeBool("generateHero", info.generateHeroAtMainTown);
			handler.serializeInt("x", info.posOfMainTown.x, -1);
			handler.serializeInt("y", info.posOfMainTown.y, -1);
			handler.serializeInt("l", info.posOfMainTown.z, -1);
		}
		if(!handler.saving)
		{
			info.hasMainTown = info.posOfMainTown.valid();
		}

		handler.serializeString("mainHero", info.mainHeroInstance);//must be before "heroes"

		//heroes
		if(handler.saving)
		{
			//ignoring heroesNames and saving from actual map objects
			//TODO: optimize
			for(auto & obj : map->objects)
			{
				if((obj->ID == Obj::HERO || obj->ID == Obj::RANDOM_HERO) && obj->tempOwner == PlayerColor(player))
				{
					CGHeroInstance * hero = dynamic_cast<CGHeroInstance *>(obj.get());

					auto heroes = handler.enterStruct("heroes");
					if(hero)
					{
						auto heroData = handler.enterStruct(hero->instanceName);
						heroData->serializeString("name", hero->name);

						if(hero->ID == Obj::HERO)
						{
							std::string temp;
							if(hero->type)
							{
								temp = hero->type->identifier;
							}
							else
							{
								temp = VLC->heroh->objects[hero->subID]->identifier;
							}
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
				hname.heroId = -1;
				std::string rawId = data["type"].String();

				if(rawId != "")
					hname.heroId = HeroTypeID::decode(rawId);

				hname.heroName = data["name"].String();

				if(instanceName == info.mainHeroInstance)
				{
					//this is main hero
					info.mainCustomHeroName = hname.heroName;
					info.hasRandomHero = (hname.heroId == -1);
					info.mainCustomHeroId = hname.heroId;
					info.mainCustomHeroPortrait = -1;
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
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
			if(mapHeader->players[i].canComputerPlay || mapHeader->players[i].canHumanPlay)
				mapHeader->players[i].team = TeamID(mapHeader->howManyTeams++);
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
			JsonNode team(JsonNode::JsonType::DATA_VECTOR);
			for(const PlayerColor & player : teamData)
			{
				JsonNode member(JsonNode::JsonType::DATA_STRING);
				member.String() = GameConstants::PLAYER_COLOR_NAMES[player.getNum()];
				team.Vector().push_back(std::move(member));
			}
			dest.Vector().push_back(std::move(team));
		}
		handler.serializeRaw("teams", dest, boost::none);
	}
}

void CMapFormatJson::readTriggeredEvents(JsonDeserializer & handler)
{
	const JsonNode & input = handler.getCurrent();

	mapHeader->triggeredEvents.clear();

	for(auto & entry : input["triggeredEvents"].Struct())
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
	JsonNode triggeredEvents(JsonNode::JsonType::DATA_STRUCT);

	for(auto event : mapHeader->triggeredEvents)
		writeTriggeredEvent(event, triggeredEvents[event.identifier]);

	handler.serializeRaw("triggeredEvents", triggeredEvents, boost::none);
}

void CMapFormatJson::writeTriggeredEvent(const TriggeredEvent & event, JsonNode & dest)
{
	using namespace TriggeredEventsDetail;

	if(!event.onFulfill.empty())
		dest["message"].String() = event.onFulfill;

	if(!event.description.empty())
		dest["description"].String() = event.description;

	dest["effect"]["type"].String() = typeNames.at(size_t(event.effect.type));

	if(!event.effect.toOtherMessage.empty())
		dest["effect"]["messageToSend"].String() = event.effect.toOtherMessage;

	dest["condition"] = event.trigger.toJson(ConditionToJson);
}

void CMapFormatJson::readDisposedHeroes(JsonSerializeFormat & handler)
{
	auto definitions = handler.enterStruct("predefinedHeroes");//DisposedHeroes are part of predefinedHeroes in VCMI map format

	const JsonNode & data = handler.getCurrent();

	for(const auto & entry : data.Struct())
	{
		HeroTypeID type(HeroTypeID::decode(entry.first));

		ui8 mask = 0;

		for(const JsonNode & playerData : entry.second["availableFor"].Vector())
		{
			PlayerColor player = PlayerColor(vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, playerData.String()));
			if(player.isValidPlayer())
			{
				mask |= 1 << player.getNum();
			}
		}

		if(mask != 0 && mask != GameConstants::ALL_PLAYERS && type.getNum() >= 0)
		{
			DisposedHero hero;

			hero.heroId = type.getNum();
			hero.players = mask;
			//name and portrait are not used

			map->disposedHeroes.push_back(hero);
		}
	}
}

void CMapFormatJson::writeDisposedHeroes(JsonSerializeFormat & handler)
{
	if(map->disposedHeroes.empty())
		return;

	auto definitions = handler.enterStruct("predefinedHeroes");//DisposedHeroes are part of predefinedHeroes in VCMI map format

	for(const DisposedHero & hero : map->disposedHeroes)
	{
		std::string type = HeroTypeID::encode(hero.heroId);

		auto definition = definitions->enterStruct(type);

		JsonNode players(JsonNode::JsonType::DATA_VECTOR);

		for(int playerNum = 0; playerNum < PlayerColor::PLAYER_LIMIT_I; playerNum++)
		{
			if((1 << playerNum) & hero.players)
			{
				JsonNode player(JsonNode::JsonType::DATA_STRING);
				player.String() = GameConstants::PLAYER_COLOR_NAMES[playerNum];
				players.Vector().push_back(player);
			}
		}
		definition->serializeRaw("availableFor", players, boost::none);
	}
}

void CMapFormatJson::serializeRumors(JsonSerializeFormat & handler)
{
	auto rumors = handler.enterArray("rumors");
	rumors.serializeStruct(map->rumors);
}

void CMapFormatJson::serializePredefinedHeroes(JsonSerializeFormat & handler)
{
    //todo:serializePredefinedHeroes

    if(handler.saving)
	{
		if(!map->predefinedHeroes.empty())
		{
			auto predefinedHeroes = handler.enterStruct("predefinedHeroes");

            for(auto & hero : map->predefinedHeroes)
			{
                auto predefinedHero = handler.enterStruct(hero->getHeroTypeName());

                hero->serializeJsonDefinition(handler);
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

			CGHeroInstance * hero = new CGHeroInstance();
			hero->ID = Obj::HERO;
            hero->setHeroTypeName(p.first);
            hero->serializeJsonDefinition(handler);

            map->predefinedHeroes.push_back(hero);
		}
	}
}

void CMapFormatJson::serializeOptions(JsonSerializeFormat & handler)
{
	serializeRumors(handler);

	serializePredefinedHeroes(handler);

	handler.serializeLIC("allowedAbilities", &CSkillHandler::decodeSkill, &CSkillHandler::encodeSkill, VLC->skillh->getDefaultAllowed(), map->allowedAbilities);

	handler.serializeLIC("allowedArtifacts",  &ArtifactID::decode, &ArtifactID::encode, VLC->arth->getDefaultAllowed(), map->allowedArtifact);

	handler.serializeLIC("allowedSpells", &SpellID::decode, &SpellID::encode, VLC->spellh->getDefaultAllowed(), map->allowedSpell);

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
CMapPatcher::CMapPatcher(JsonNode stream):
	CMapFormatJson(),
	input(stream)
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
}

///CMapLoaderJson
CMapLoaderJson::CMapLoaderJson(CInputStream * stream):
	CMapFormatJson(),
	buffer(stream),
	ioApi(new CProxyROIOApi(buffer)),
	loader("", "_", ioApi)
{

}

std::unique_ptr<CMap> CMapLoaderJson::loadMap()
{
	LOG_TRACE(logGlobal);
	std::unique_ptr<CMap> result = std::unique_ptr<CMap>(new CMap());
	map = result.get();
	mapHeader = map;
	readMap();
	return result;
}

std::unique_ptr<CMapHeader> CMapLoaderJson::loadMapHeader()
{
	LOG_TRACE(logGlobal);
	map = nullptr;
	std::unique_ptr<CMapHeader> result = std::unique_ptr<CMapHeader>(new CMapHeader());
	mapHeader = result.get();
	readHeader(false);
	return result;
}

JsonNode CMapLoaderJson::getFromArchive(const std::string & archiveFilename)
{
	ResourceID resource(archiveFilename, EResType::TEXT);

	if(!loader.existsResource(resource))
		throw std::runtime_error(archiveFilename+" not found");

	auto data = loader.load(resource)->readAll();

	JsonNode res(reinterpret_cast<char*>(data.first.get()), data.second);

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

	if(fileVersionMajor != VERSION_MAJOR)
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

	//todo: multilevel map load support
	{
		auto levels = handler.enterStruct("mapLevels");

		{
			auto surface = handler.enterStruct("surface");
			handler.serializeInt("height", mapHeader->height);
			handler.serializeInt("width", mapHeader->width);
		}
		{
			auto underground = handler.enterStruct("underground");
			mapHeader->twoLevel = !underground->getCurrent().isNull();
		}
	}

	serializeHeader(handler);

	readTriggeredEvents(handler);

	readTeams(handler);
	//TODO: check mods

	if(complete)
		readOptions(handler);
}

void CMapLoaderJson::readTerrainTile(const std::string & src, TerrainTile & tile)
{
	try
	{
		using namespace TerrainDetail;
		{//terrain type
			const std::string typeCode = src.substr(0, 2);
			tile.terType = const_cast<TerrainType*>(VLC->terrainTypeHandler->getInfoByCode(typeCode));
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
		//FIXME: check without exceptions?
		{//road type
			const std::string typeCode = src.substr(startPos, 2);
			startPos += 2;
			try
			{
				tile.roadType = const_cast<RoadType*>(VLC->terrainTypeHandler->getRoadByCode(typeCode));
			}
			catch (const std::exception& e) //it's not a road, it's a river
			{
				try
				{
					tile.riverType = const_cast<RiverType*>(VLC->terrainTypeHandler->getRiverByCode(typeCode));
					hasRoad = false;
				}
				catch (const std::exception& e)
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
			tile.riverType = const_cast<RiverType*>(VLC->terrainTypeHandler->getRiverByCode(typeCode));
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
	catch (const std::exception & e)
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

CMapLoaderJson::MapObjectLoader::MapObjectLoader(CMapLoaderJson * _owner, JsonMap::value_type & json):
	owner(_owner), instance(nullptr), id(-1), jsonKey(json.first), configuration(json.second)
{

}

void CMapLoaderJson::MapObjectLoader::construct()
{
	//TODO:consider move to ObjectTypeHandler
	//find type handler
	std::string typeName = configuration["type"].String(), subtypeName = configuration["subtype"].String();
	if(typeName.empty())
	{
		logGlobal->error("Object type missing");
		logGlobal->debug(configuration.toJson());
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
		logGlobal->debug(configuration.toJson());
		return;
	}

	auto handler = VLC->objtypeh->getHandlerFor( CModHandler::scopeMap(), typeName, subtypeName);

	auto appearance = new ObjectTemplate;

	appearance->id = Obj(handler->type);
	appearance->subid = handler->subtype;
	appearance->readJson(configuration["template"], false);

	// Will be destroyed soon and replaced with shared template
	instance = handler->create(std::shared_ptr<const ObjectTemplate>(appearance));

	instance->id = ObjectInstanceID((si32)owner->map->objects.size());
	instance->instanceName = jsonKey;
	instance->pos = pos;
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

	if(auto art = dynamic_cast<CGArtifact *>(instance))
	{
		auto artID = ArtifactID::NONE;
		int spellID = -1;

		if(art->ID == Obj::SPELL_SCROLL)
		{
			auto spellIdentifier = configuration["options"]["spell"].String();
			auto rawId = VLC->modh->identifiers.getIdentifier(CModHandler::scopeBuiltin(), "spell", spellIdentifier);
			if(rawId)
				spellID = rawId.get();
			else
				spellID = 0;
			artID = ArtifactID::SPELL_SCROLL;
		}
		else if(art->ID  == Obj::ARTIFACT)
		{
			//specific artifact
			artID = ArtifactID(art->subID);
		}

		art->storedArtifact = CArtifactInstance::createArtifact(owner->map, artID, spellID);
	}

	if(auto hero = dynamic_cast<CGHeroInstance *>(instance))
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
		loaders.push_back(vstd::make_unique<MapObjectLoader>(this, p));

	for(auto & ptr : loaders)
		ptr->construct();

	//configure objects after all objects are constructed so we may resolve internal IDs even to actual pointers OTF
	for(auto & ptr : loaders)
		ptr->configure();

	std::sort(map->heroesOnMap.begin(), map->heroesOnMap.end(), [](const ConstTransitivePtr<CGHeroInstance> & a, const ConstTransitivePtr<CGHeroInstance> & b)
	{
		return a->subID < b->subID;
	});
}

///CMapSaverJson
CMapSaverJson::CMapSaverJson(CInputOutputStream * stream):
	CMapFormatJson(),
	buffer(stream),
	ioApi(new CProxyIOApi(buffer)),
	saver(ioApi, "_")
{
	fileVersionMajor = VERSION_MAJOR;
	fileVersionMinor = VERSION_MINOR;
}

CMapSaverJson::~CMapSaverJson()
{

}

void CMapSaverJson::addToArchive(const JsonNode & data, const std::string & filename)
{
	std::ostringstream out;
	JsonWriter writer(out);
	writer.writeNode(data);
	out.flush();

	{
		auto s = out.str();
		std::unique_ptr<COutputStream> stream = saver.addFile(filename);

		if (stream->write((const ui8*)s.c_str(), s.size()) != s.size())
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

std::string CMapSaverJson::writeTerrainTile(const TerrainTile & tile)
{
	using namespace TerrainDetail;

	std::ostringstream out;
	out.setf(std::ios::dec, std::ios::basefield);
	out.unsetf(std::ios::showbase);

	out << tile.terType->typeCode << (int)tile.terView << flipCodes[tile.extTileFlags % 4];

	if(tile.roadType->id != Road::NO_ROAD)
		out << tile.roadType->code << (int)tile.roadDir << flipCodes[(tile.extTileFlags >> 4) % 4];

	if(tile.riverType->id != River::NO_RIVER)
		out << tile.riverType->code << (int)tile.riverDir << flipCodes[(tile.extTileFlags >> 2) % 4];

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
	logGlobal->trace("Saving objects");
	JsonNode data(JsonNode::JsonType::DATA_STRUCT);

	JsonSerializer handler(mapObjectResolver.get(), data);

	for(CGObjectInstance * obj : map->objects)
	{
		//logGlobal->trace("\t%s", obj->instanceName);
		auto temp = handler.enterStruct(obj->instanceName);

		obj->serializeJson(handler);
	}

	if(map->grailPos.valid())
	{
		JsonNode grail(JsonNode::JsonType::DATA_STRUCT);
		grail["type"].String() = "grail";

		grail["x"].Float() = map->grailPos.x;
		grail["y"].Float() = map->grailPos.y;
		grail["l"].Float() = map->grailPos.z;

		grail["options"]["radius"].Float() = map->grailRadius;

		std::string grailId = boost::str(boost::format("grail_%d") % map->objects.size());

		data[grailId] = grail;
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


VCMI_LIB_NAMESPACE_END
