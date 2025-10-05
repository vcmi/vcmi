/*
 * CTownHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTownHandler.h"

#include "CTown.h"
#include "CFaction.h"
#include "../building/CBuilding.h"
#include "../hero/CHeroClassHandler.h"

#include "../../CCreatureHandler.h"
#include "../../IGameSettings.h"
#include "../../TerrainHandler.h"
#include "../../GameLibrary.h"

#include "../../bonuses/Propagators.h"
#include "../../constants/StringConstants.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../modding/IdentifierStorage.h"
#include "../../modding/ModScope.h"
#include "../../spells/CSpellHandler.h"
#include "../../texts/CGeneralTextHandler.h"
#include "../../texts/CLegacyConfigParser.h"
#include "../../json/JsonBonus.h"
#include "../../json/JsonUtils.h"

VCMI_LIB_NAMESPACE_BEGIN

const int NAMES_PER_TOWN=16; // number of town names per faction in H3 files. Json can define any number

CTownHandler::CTownHandler()
	: buildingsLibrary(JsonPath::builtin("config/buildingsLibrary"))
	, randomFaction(std::make_unique<CFaction>())
{
	randomFaction->town = std::make_unique<CTown>();
	randomFaction->town->faction = randomFaction.get();
	randomFaction->identifier = "random";
	randomFaction->modScope = "core";
}

CTownHandler::~CTownHandler() = default;

JsonNode readBuilding(CLegacyConfigParser & parser)
{
	JsonNode ret;
	JsonNode & cost = ret["cost"];

	for(const std::string & resID : GameConstants::RESOURCE_NAMES)
		cost[resID].Float() = parser.readNumber();
	
	parser.endLine();

	return ret;
}

const TPropagatorPtr & CTownHandler::emptyPropagator()
{
	static const TPropagatorPtr emptyProp(nullptr);
	return emptyProp;
}

std::vector<JsonNode> CTownHandler::loadLegacyData()
{
	size_t dataSize = LIBRARY->engineSettings()->getInteger(EGameSettings::TEXTS_FACTION);

	std::vector<JsonNode> dest(dataSize);
	objects.resize(dataSize);

	auto getBuild = [&](size_t town, size_t building) -> JsonNode &
	{
		return dest[town]["town"]["buildings"][EBuildingType::names[building]];
	};

	CLegacyConfigParser parser(TextPath::builtin("DATA/BUILDING.TXT"));

	parser.endLine(); // header
	parser.endLine();

	//Unique buildings
	for (size_t town=0; town<dataSize; town++)
	{
		parser.endLine(); //header
		parser.endLine();

		int buildID = 17;
		do
		{
			getBuild(town, buildID) = readBuilding(parser);
			buildID++;
		}
		while (!parser.isNextEntryEmpty());
	}

	// Common buildings
	parser.endLine(); // header
	parser.endLine();
	parser.endLine();

	int buildID = 0;
	do
	{
		JsonNode building = readBuilding(parser);

		for (size_t town=0; town<dataSize; town++)
			getBuild(town, buildID) = building;

		buildID++;
	}
	while (!parser.isNextEntryEmpty());

	parser.endLine(); //header
	parser.endLine();

	//Dwellings
	for (size_t town=0; town<dataSize; town++)
	{
		parser.endLine(); //header
		parser.endLine();

		for (size_t i=0; i<14; i++)
		{
			getBuild(town, 30+i) = readBuilding(parser);
		}
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/BLDGNEUT.TXT"));

		for(int building=0; building<15; building++)
		{
			std::string name  = parser.readString();
			std::string descr = parser.readString();
			parser.endLine();

			for(int j=0; j<dataSize; j++)
			{
				getBuild(j, building)["name"].String() = name;
				getBuild(j, building)["description"].String() = descr;
			}
		}
		parser.endLine(); // silo
		parser.endLine(); // blacksmith  //unused entries
		parser.endLine(); // moat

		//shipyard with the ship
		std::string name  = parser.readString();
		std::string descr = parser.readString();
		parser.endLine();

		for(int town=0; town<dataSize; town++)
		{
			getBuild(town, 20)["name"].String() = name;
			getBuild(town, 20)["description"].String() = descr;
		}

		//blacksmith
		for(int town=0; town<dataSize; town++)
		{
			getBuild(town, 16)["name"].String() =  parser.readString();
			getBuild(town, 16)["description"].String() = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/BLDGSPEC.TXT"));

		for(int town=0; town<dataSize; town++)
		{
			for(int build=0; build<9; build++)
			{
				getBuild(town, 17 + build)["name"].String() =  parser.readString();
				getBuild(town, 17 + build)["description"].String() = parser.readString();
				parser.endLine();
			}
			getBuild(town, 26)["name"].String() =  parser.readString(); // Grail
			getBuild(town, 26)["description"].String() = parser.readString();
			parser.endLine();

			getBuild(town, 15)["name"].String() =  parser.readString(); // Resource silo
			getBuild(town, 15)["description"].String() = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser(TextPath::builtin("DATA/DWELLING.TXT"));

		for(int town=0; town<dataSize; town++)
		{
			for(int build=0; build<14; build++)
			{
				getBuild(town, 30 + build)["name"].String() =  parser.readString();
				getBuild(town, 30 + build)["description"].String() = parser.readString();
				parser.endLine();
			}
		}
	}
	{
		CLegacyConfigParser typeParser(TextPath::builtin("DATA/TOWNTYPE.TXT"));
		CLegacyConfigParser nameParser(TextPath::builtin("DATA/TOWNNAME.TXT"));
		size_t townID=0;
		do
		{
			dest[townID]["name"].String() = typeParser.readString();

			for (int i=0; i<NAMES_PER_TOWN; i++)
			{
				JsonNode name;
				name.String() = nameParser.readString();
				dest[townID]["town"]["names"].Vector().push_back(name);
				nameParser.endLine();
			}
			townID++;
		}
		while (typeParser.endLine());
	}
	return dest;
}

void CTownHandler::loadBuildingRequirements(CBuilding * building, const JsonNode & source, std::vector<BuildingRequirementsHelper> & bidsToLoad) const
{
	if (source.isNull())
		return;

	BuildingRequirementsHelper hlp;
	hlp.building = building;
	hlp.town = building->town;
	hlp.json = source;
	bidsToLoad.push_back(hlp);
}

void CTownHandler::loadBuildingBonuses(const JsonNode & source, BonusList & bonusList, CBuilding * building) const
{
	for(const auto & b : source.Vector())
	{
		auto bonus = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NONE, BonusSource::TOWN_STRUCTURE, 0, BonusSourceID(building->getUniqueTypeID()));

		if(!JsonUtils::parseBonus(b, bonus.get()))
			continue;

		if (bonus->description.empty() && (bonus->type == BonusType::MORALE || bonus->type == BonusType::LUCK))
			bonus->description.appendTextID(building->getNameTextID());

		//JsonUtils::parseBuildingBonus produces UNKNOWN type propagator instead of empty.
		assert(bonus->propagator == nullptr || bonus->propagator->getPropagatorType() != BonusNodeType::UNKNOWN);

		if(bonus->propagator != nullptr
			&& bonus->propagator->getPropagatorType() == BonusNodeType::UNKNOWN)
				bonus->addPropagator(emptyPropagator());
		building->addNewBonus(bonus, bonusList);
	}
}

void CTownHandler::loadBuilding(CTown * town, const std::string & stringID, const JsonNode & source)
{
	assert(stringID.find(':') == std::string::npos);
	assert(!source.getModScope().empty());

	auto * ret = new CBuilding();
	ret->bid = BuildingID(source["id"].Integer());
	ret->subId = BuildingSubID::NONE;

	if (ret->bid == BuildingID::NONE)
		logMod->error("Building '%s' isn't recognized and won't work properly. Correct the typo or update VCMI.", stringID);

	ret->mode = ret->bid == BuildingID::GRAIL
		? CBuilding::BUILD_GRAIL
		: vstd::find_or(CBuilding::MODES, source["mode"].String(), CBuilding::BUILD_NORMAL);

	ret->identifier = stringID;
	ret->modScope = source.getModScope();
	ret->town = town;

	LIBRARY->generaltexth->registerString(source.getModScope(), ret->getNameTextID(), source["name"]);
	LIBRARY->generaltexth->registerString(source.getModScope(), ret->getDescriptionTextID(), source["description"]);

	ret->subId = vstd::find_or(MappedKeys::SPECIAL_BUILDINGS, source["type"].String(), BuildingSubID::NONE);
	ret->resources.resolveFromJson(source["cost"]);

	//MODS COMPATIBILITY FOR pre-1.6
	bool produceEmpty = true;
	for(auto & res : source["produce"].Struct())
		if(res.second.Integer() != 0)
			produceEmpty = false;
	if(!produceEmpty)
		ret->produce.resolveFromJson(source["produce"]); // non legacy
	else if(ret->bid == BuildingID::RESOURCE_SILO)
	{
		logGlobal->warn("Resource silo in town '%s' does not produce any resources!", ret->town->faction->getJsonKey());
		switch (ret->town->primaryRes.toEnum())
		{
			case EGameResID::GOLD:
				ret->produce[ret->town->primaryRes] = 500;
				break;
			case EGameResID::WOOD_AND_ORE:
				ret->produce[EGameResID::WOOD] = 1;
				ret->produce[EGameResID::ORE] = 1;
				break;
			default:
				ret->produce[ret->town->primaryRes] = 1;
				break;
		}
	}

	ret->manualHeroVisit = source["manualHeroVisit"].Bool();
	ret->upgradeReplacesBonuses = source["upgradeReplacesBonuses"].Bool();

	const JsonNode & fortifications = source["fortifications"];
	if (!fortifications.isNull())
	{
		LIBRARY->identifiers()->requestIdentifierIfNotNull("creature", fortifications["citadelShooter"], [=](si32 identifier)
		{
			ret->fortifications.citadelShooter = CreatureID(identifier);
		});

		LIBRARY->identifiers()->requestIdentifierIfNotNull("creature", fortifications["upperTowerShooter"], [=](si32 identifier)
		{
			ret->fortifications.upperTowerShooter = CreatureID(identifier);
		});

		LIBRARY->identifiers()->requestIdentifierIfNotNull("creature", fortifications["lowerTowerShooter"], [=](si32 identifier)
		{
			ret->fortifications.lowerTowerShooter = CreatureID(identifier);
		});

		ret->fortifications.wallsHealth = fortifications["wallsHealth"].Integer();
		ret->fortifications.citadelHealth = fortifications["citadelHealth"].Integer();
		ret->fortifications.upperTowerHealth = fortifications["upperTowerHealth"].Integer();
		ret->fortifications.lowerTowerHealth = fortifications["lowerTowerHealth"].Integer();
		ret->fortifications.hasMoat = fortifications["hasMoat"].Bool();
	}

	loadBuildingBonuses(source["bonuses"], ret->buildingBonuses, ret);

	if(!source["mapObjectLikeBonuses"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifierIfNotNull("object", source["mapObjectLikeBonuses"], [ret](si32 identifier)
		{
			ret->mapObjectLikeBonuses = MapObjectID(identifier);
		});
	}

	if(!source["configuration"].isNull())
		ret->rewardableObjectInfo.init(source["configuration"], ret->getBaseTextID());

	loadBuildingRequirements(ret, source["requires"], requirementsToLoad);

	if (!source["warMachine"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifier("artifact", source["warMachine"], [=](si32 identifier)
		{
			ret->warMachine = ArtifactID(identifier);
		});
	}

	if (!source["upgrades"].isNull())
	{
		// building id and upgrades can't be the same
		if(stringID == source["upgrades"].String())
		{
			auto townName = ret->town->faction->getNameTranslated();
			logMod->error("Building with ID '%s' of town '%s' can't be an upgrade of the same building.", stringID, townName);
			throw std::runtime_error(boost::str(boost::format("Building with ID '%s' of town '%s' can't be an upgrade of the same building.")
												% stringID % townName));
		}
		else
		{
			LIBRARY->identifiers()->requestIdentifier(ret->town->getBuildingScope(), source["upgrades"], [=](si32 identifier)
			{
				ret->upgrade = BuildingID(identifier);
			});
		}
	}
	else
		ret->upgrade = BuildingID::NONE;

	if (ret->town->buildings[ret->bid] != nullptr)
		logMod->error("Mod %s, faction %s: detected multiple town buildings with ID %d", source.getModScope(), stringID, ret->bid.getNum());

	ret->town->buildings[ret->bid].reset(ret);
	for(const auto & element : source["marketModes"].Vector())
	{
		if(MappedKeys::MARKET_NAMES_TO_TYPES.count(element.String()))
		{
			auto mode = MappedKeys::MARKET_NAMES_TO_TYPES.at(element.String());
			ret->marketModes.insert(mode);

			if (mode == EMarketMode::RESOURCE_SKILL)
			{
				const auto & items = source["marketOffer"].Vector();
				ret->marketOffer.resize(items.size());
				for (int i = 0; i < items.size(); ++i)
				{
					LIBRARY->identifiers()->requestIdentifier("secondarySkill", items[i], [ret, i](si32 identifier)
					{
						ret->marketOffer[i] = SecondarySkill(identifier);
					});
				}
			}
		}
	}

	registerObject(source.getModScope(), ret->town->getBuildingScope(), ret->identifier, ret->bid.getNum());
}

void CTownHandler::loadBuildings(CTown * town, const JsonNode & source)
{
	for(const auto & node : source.Struct())
	{
		if (!node.second.isNull())
			loadBuilding(town, node.first, node.second);
	}
}

void CTownHandler::loadStructure(CTown &town, const std::string & stringID, const JsonNode & source) const
{
	auto * ret = new CStructure();

	ret->building = nullptr;
	ret->buildable = nullptr;

	LIBRARY->identifiers()->tryRequestIdentifier( source.getModScope(), "building." + town.faction->getJsonKey(), stringID, [=, &town](si32 identifier) mutable
	{
		ret->building = town.buildings[BuildingID(identifier)].get();
	});

	if (source["builds"].isNull())
	{
		LIBRARY->identifiers()->tryRequestIdentifier( source.getModScope(), "building." + town.faction->getJsonKey(), stringID, [=, &town](si32 identifier) mutable
		{
			ret->building = town.buildings[BuildingID(identifier)].get();
		});
	}
	else
	{
		LIBRARY->identifiers()->requestIdentifier("building." + town.faction->getJsonKey(), source["builds"], [=, &town](si32 identifier) mutable
		{
			ret->buildable = town.buildings[BuildingID(identifier)].get();
		});
	}

	ret->identifier = stringID;
	ret->pos.x = static_cast<si32>(source["x"].Float());
	ret->pos.y = static_cast<si32>(source["y"].Float());
	ret->pos.z = static_cast<si32>(source["z"].Float());

	ret->hiddenUpgrade = source["hidden"].Bool();
	ret->defName = AnimationPath::fromJson(source["animation"]);
	ret->borderName = ImagePath::fromJson(source["border"]);
	ret->campaignBonus = ImagePath::fromJson(source["campaignBonus"]);
	ret->areaName = ImagePath::fromJson(source["area"]);

	town.clientInfo.structures.emplace_back(ret);
}

void CTownHandler::loadStructures(CTown &town, const JsonNode & source) const
{
	for(const auto & node : source.Struct())
	{
		if (!node.second.isNull())
			loadStructure(town, node.first, node.second);
	}
}

void CTownHandler::loadTownHall(CTown &town, const JsonNode & source) const
{
	auto & dstSlots = town.clientInfo.hallSlots;
	const auto & srcSlots = source.Vector();
	dstSlots.resize(srcSlots.size());

	for(size_t i=0; i<dstSlots.size(); i++)
	{
		auto & dstRow = dstSlots[i];
		const auto & srcRow = srcSlots[i].Vector();
		dstRow.resize(srcRow.size());

		for(size_t j=0; j < dstRow.size(); j++)
		{
			auto & dstBox = dstRow[j];
			const auto & srcBox = srcRow[j].Vector();
			dstBox.resize(srcBox.size());

			for(size_t k=0; k<dstBox.size(); k++)
			{
				auto & dst = dstBox[k];
				const auto & src = srcBox[k];

				LIBRARY->identifiers()->requestIdentifier("building." + town.faction->getJsonKey(), src, [&](si32 identifier)
				{
					dst = BuildingID(identifier);
				});
			}
		}
	}
}

Point JsonToPoint(const JsonNode & node)
{
	if(!node.isStruct())
		return Point::makeInvalid();

	Point ret;
	ret.x = static_cast<si32>(node["x"].Float());
	ret.y = static_cast<si32>(node["y"].Float());
	return ret;
}

void CTownHandler::loadSiegeScreen(CTown &town, const JsonNode & source) const
{
	town.clientInfo.siegePrefix = source["imagePrefix"].String();
	town.clientInfo.towerIconSmall = source["towerIconSmall"].String();
	town.clientInfo.towerIconLarge = source["towerIconLarge"].String();

	LIBRARY->identifiers()->requestIdentifier("creature", source["shooter"], [&town](si32 creature)
	{
		auto crId = CreatureID(creature);
		if((*LIBRARY->creh)[crId]->animation.missileFrameAngles.empty())
			logMod->error("Mod '%s' error: Creature '%s' on the Archer's tower is not a shooter. Mod should be fixed. Siege will not work properly!"
				, town.faction->getNameTranslated()
				, (*LIBRARY->creh)[crId]->getNameSingularTranslated());

		town.fortifications.citadelShooter = crId;
		town.fortifications.upperTowerShooter = crId;
		town.fortifications.lowerTowerShooter = crId;
	});

	auto & pos = town.clientInfo.siegePositions;
	pos.resize(21);

	pos[8]  = JsonToPoint(source["towers"]["top"]["tower"]);
	pos[17] = JsonToPoint(source["towers"]["top"]["battlement"]);
	pos[20] = JsonToPoint(source["towers"]["top"]["creature"]);

	pos[2]  = JsonToPoint(source["towers"]["keep"]["tower"]);
	pos[15] = JsonToPoint(source["towers"]["keep"]["battlement"]);
	pos[18] = JsonToPoint(source["towers"]["keep"]["creature"]);

	pos[3]  = JsonToPoint(source["towers"]["bottom"]["tower"]);
	pos[16] = JsonToPoint(source["towers"]["bottom"]["battlement"]);
	pos[19] = JsonToPoint(source["towers"]["bottom"]["creature"]);

	pos[9]  = JsonToPoint(source["gate"]["gate"]);
	pos[10]  = JsonToPoint(source["gate"]["arch"]);

	pos[7]  = JsonToPoint(source["walls"]["upper"]);
	pos[6]  = JsonToPoint(source["walls"]["upperMid"]);
	pos[5]  = JsonToPoint(source["walls"]["bottomMid"]);
	pos[4]  = JsonToPoint(source["walls"]["bottom"]);

	pos[13] = JsonToPoint(source["moat"]["moat"]);
	pos[14] = JsonToPoint(source["moat"]["bank"]);

	pos[11] = JsonToPoint(source["static"]["bottom"]);
	pos[12] = JsonToPoint(source["static"]["top"]);
	pos[1]  = JsonToPoint(source["static"]["background"]);
}

static void readIcon(JsonNode source, std::string & small, std::string & large)
{
	if (source.getType() == JsonNode::JsonType::DATA_STRUCT) // don't crash on old format
	{
		small = source["small"].String();
		large = source["large"].String();
	}
}

void CTownHandler::loadClientData(CTown &town, const JsonNode & source) const
{
	CTown::ClientInfo & info = town.clientInfo;

	readIcon(source["icons"]["village"]["normal"], info.iconSmall[0][0], info.iconLarge[0][0]);
	readIcon(source["icons"]["village"]["built"], info.iconSmall[0][1], info.iconLarge[0][1]);
	readIcon(source["icons"]["fort"]["normal"], info.iconSmall[1][0], info.iconLarge[1][0]);
	readIcon(source["icons"]["fort"]["built"], info.iconSmall[1][1], info.iconLarge[1][1]);

	info.hallBackground = ImagePath::fromJson(source["hallBackground"]);
	info.townBackground = ImagePath::fromJson(source["townBackground"]);
	info.buildingsIcons = AnimationPath::fromJson(source["buildingsIcons"]);
	info.tavernVideo = VideoPath::fromJson(source["tavernVideo"]);
	info.guildWindowPosition  = Point(source["guildWindowPosition"]["x"].Integer(), source["guildWindowPosition"]["y"].Integer());

	info.guildSpellPositions.clear();
	for(auto & level : source["guildSpellPositions"].Vector())
	{
		std::vector<Point> points;
		for(auto & item : level.Vector())
			points.push_back(Point(item["x"].Integer(), item["y"].Integer()));
		info.guildSpellPositions.push_back(points);
	}

	auto loadStringOrVector = [](auto & target, auto & node, auto fromJsonFunc){
		if(node.isVector())
		{
			target.clear();
			for(auto & background : node.Vector())
				target.push_back(fromJsonFunc(background));
		}
		else
			target = {fromJsonFunc(node)};
	};
	loadStringOrVector(info.musicTheme, source["musicTheme"], AudioPath::fromJson);
	loadStringOrVector(info.guildBackground, source["guildBackground"], ImagePath::fromJson);
	loadStringOrVector(info.guildWindow, source["guildWindow"], ImagePath::fromJson);

	loadTownHall(town,   source["hallSlots"]);
	loadStructures(town, source["structures"]);
	loadSiegeScreen(town, source["siege"]);
}

void CTownHandler::loadTown(CTown * town, const JsonNode & source)
{
	const auto * resIter = boost::find(GameConstants::RESOURCE_NAMES, source["primaryResource"].String());
	if(resIter == std::end(GameConstants::RESOURCE_NAMES))
		town->primaryRes = GameResID(EGameResID::WOOD_AND_ORE); //Wood + Ore
	else
		town->primaryRes = GameResID(resIter - std::begin(GameConstants::RESOURCE_NAMES));

	if (!source["warMachine"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifier( "creature", source["warMachine"], [=](si32 creatureID)
		{
			town->warMachineDeprecated = creatureID;
		});
	}

	town->mageLevel = static_cast<ui32>(source["mageGuild"].Float());

	town->namesCount = 0;
	for(const auto & name : source["names"].Vector())
	{
		LIBRARY->generaltexth->registerString(town->faction->modScope, town->getRandomNameTextID(town->namesCount), name);
		town->namesCount += 1;
	}

	if (!source["moatAbility"].isNull()) // VCMI 1.2 compatibility code
	{
		LIBRARY->identifiers()->requestIdentifier( "spell", source["moatAbility"], [=](si32 ability)
		{
			town->fortifications.moatSpell = SpellID(ability);
		});
	}
	else
	{
		LIBRARY->identifiers()->requestIdentifier( source.getModScope(), "spell", "castleMoat", [=](si32 ability)
		{
			town->fortifications.moatSpell = SpellID(ability);
		});
	}

	//  Horde building creature level
	for(const JsonNode &node : source["horde"].Vector())
		town->hordeLvl[static_cast<int>(town->hordeLvl.size())] = static_cast<int>(node.Float());

	// town needs to have exactly 2 horde entries. Validation will take care of 2+ entries
	// but anything below 2 must be handled here
	for (size_t i=source["horde"].Vector().size(); i<2; i++)
		town->hordeLvl[static_cast<int>(i)] = -1;

	const JsonVector & creatures = source["creatures"].Vector();

	town->creatures.resize(creatures.size());

	for (size_t i=0; i< creatures.size(); i++)
	{
		const JsonVector & level = creatures[i].Vector();

		town->creatures[i].resize(level.size());

		for (size_t j=0; j<level.size(); j++)
		{
			LIBRARY->identifiers()->requestIdentifier("creature", level[j], [=](si32 creature)
			{
				town->creatures[i][j] = CreatureID(creature);
			});
		}
	}

	town->defaultTavernChance = static_cast<ui32>(source["defaultTavern"].Float());
	/// set chance of specific hero class to appear in this town
	for(const auto & node : source["tavern"].Struct())
	{
		int chance = static_cast<int>(node.second.Float());

		LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), "heroClass", node.first, [=](si32 classID)
		{
			LIBRARY->heroclassesh->objects[classID]->selectionProbability[town->faction->getId()] = chance;
		});
	}

	for(const auto & node : source["guildSpells"].Struct())
	{
		int chance = static_cast<int>(node.second.Float());

		LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), "spell", node.first, [=](si32 spellID)
		{
			LIBRARY->spellh->objects.at(spellID)->probabilities[town->faction->getId()] = chance;
		});
	}

	for(const JsonNode & d : source["adventureMap"]["dwellings"].Vector())
	{
		town->dwellings.push_back(d["graphics"].String());
		town->dwellingNames.push_back(d["name"].String());
	}

	loadBuildings(town, source["buildings"]);
	loadClientData(*town, source);
}

void CTownHandler::loadPuzzle(CFaction &faction, const JsonNode &source) const
{
	faction.puzzleMap.reserve(GameConstants::PUZZLE_MAP_PIECES);

	std::string prefix = source["prefix"].String();
	for(const JsonNode &piece : source["pieces"].Vector())
	{
		size_t index = faction.puzzleMap.size();
		SPuzzleInfo spi;

		spi.position.x = static_cast<si16>(piece["x"].Float());
		spi.position.y = static_cast<si16>(piece["y"].Float());
		spi.whenUncovered = static_cast<ui16>(piece["index"].Float());
		spi.number = static_cast<ui16>(index);

		// filename calculation
		std::ostringstream suffix;
		suffix << std::setfill('0') << std::setw(2) << index;

		spi.filename = ImagePath::builtinTODO(prefix + suffix.str());

		faction.puzzleMap.push_back(spi);
	}
	assert(faction.puzzleMap.size() == GameConstants::PUZZLE_MAP_PIECES);
}

std::shared_ptr<CFaction> CTownHandler::loadFromJson(const std::string & scope, const JsonNode & source, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto faction = std::make_shared<CFaction>();

	faction->index = static_cast<FactionID>(index);
	faction->modScope = scope;
	faction->identifier = identifier;

	LIBRARY->generaltexth->registerString(scope, faction->getNameTextID(), source["name"]);
	LIBRARY->generaltexth->registerString(scope, faction->getDescriptionTextID(), source["description"]);

	faction->creatureBg120 = ImagePath::fromJson(source["creatureBackground"]["120px"]);
	faction->creatureBg130 = ImagePath::fromJson(source["creatureBackground"]["130px"]);

	faction->boatType = BoatId::CASTLE; //Do not crash
	if (!source["boat"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifier("core:boat", source["boat"], [=](int32_t boatTypeID)
		{
			faction->boatType = BoatId(boatTypeID);
		});
	}

	int alignment = vstd::find_pos(GameConstants::ALIGNMENT_NAMES, source["alignment"].String());
	if (alignment == -1)
		faction->alignment = EAlignment::NEUTRAL;
	else
		faction->alignment = static_cast<EAlignment>(alignment);
	
	auto preferUndergound = source["preferUndergroundPlacement"];
	faction->preferUndergroundPlacement = preferUndergound.isNull() ? false : preferUndergound.Bool();
	faction->special = source["special"].Bool();

	// NOTE: semi-workaround - normally, towns are supposed to have native terrains.
	// Towns without one are exceptions. So, vcmi requires nativeTerrain to be defined
	// But allows it to be defined with explicit value of "none" if town should not have native terrain
	// This is better than allowing such terrain-less towns silently, leading to issues with RMG
	faction->nativeTerrain = ETerrainId::NONE;
	if ( !source["nativeTerrain"].isNull() && source["nativeTerrain"].String() != "none")
	{
		LIBRARY->identifiers()->requestIdentifier("terrain", source["nativeTerrain"], [=](int32_t index){
			faction->nativeTerrain = TerrainId(index);

			auto const & terrain = LIBRARY->terrainTypeHandler->getById(faction->nativeTerrain);

			if (!terrain->isSurface() && !terrain->isUnderground())
				logMod->warn("Faction %s has terrain %s as native, but terrain is not suitable for either surface or subterranean layers!", faction->getJsonKey(), terrain->getJsonKey());
		});
	}

	if (!source["town"].isNull())
	{
		faction->town = std::make_unique<CTown>();
		faction->town->faction = faction.get();
		loadTown(faction->town.get(), source["town"]);
	}
	else
		faction->town = nullptr;

	if (!source["puzzleMap"].isNull())
		loadPuzzle(*faction, source["puzzleMap"]);

	return faction;
}

void CTownHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(scope, data, name, objects.size());

	objects.emplace_back(object);

	if (object->town)
	{
		auto & info = object->town->clientInfo;
		info.icons[0][0] = 8 + object->index.getNum() * 4 + 0;
		info.icons[0][1] = 8 + object->index.getNum() * 4 + 1;
		info.icons[1][0] = 8 + object->index.getNum() * 4 + 2;
		info.icons[1][1] = 8 + object->index.getNum() * 4 + 3;

		LIBRARY->identifiers()->requestIdentifier(scope, "object", "town", [=](si32 index)
		{
			// register town once objects are loaded
			JsonNode config = data["town"]["mapObject"];
			config["faction"].String() = name;
			config["faction"].setModScope(scope, false);
			if (config.getModScope().empty())// MODS COMPATIBILITY FOR 0.96
				config.setModScope(scope, false);
			LIBRARY->objtypeh->loadSubObject(object->identifier, config, index, object->index);

			// MODS COMPATIBILITY FOR 0.96
			const auto & advMap = data["town"]["adventureMap"];
			if (!advMap.isNull())
			{
				logMod->warn("Outdated town mod. Will try to generate valid templates out of fort");
				JsonNode config;
				config["animation"] = advMap["castle"];
				LIBRARY->objtypeh->getHandlerFor(index, object->index)->addTemplate(config);
			}
		});
	}

	registerObject(scope, "faction", name, object->index.getNum());
}

void CTownHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(scope, data, name, index);

	if (objects.size() > index)
		assert(objects[index] == nullptr); // ensure that this id was not loaded before
	else
		objects.resize(index + 1);
	objects[index] = object;

	if (object->town)
	{
		auto & info = object->town->clientInfo;
		info.icons[0][0] = (GameConstants::F_NUMBER + object->index.getNum()) * 2 + 0;
		info.icons[0][1] = (GameConstants::F_NUMBER + object->index.getNum()) * 2 + 1;
		info.icons[1][0] = object->index.getNum() * 2 + 0;
		info.icons[1][1] = object->index.getNum() * 2 + 1;

		LIBRARY->identifiers()->requestIdentifier(scope, "object", "town", [=](si32 index)
		{
			// register town once objects are loaded
			JsonNode config = data["town"]["mapObject"];
			config["faction"].String() = name;
			config["faction"].setModScope(scope, false);
			LIBRARY->objtypeh->loadSubObject(object->identifier, config, index, object->index);
		});
	}

	registerObject(scope, "faction", name, object->index.getNum());
}

void CTownHandler::loadRandomFaction()
{
	JsonNode randomFactionJson(JsonPath::builtin("config/factions/random.json"));
	randomFactionJson.setModScope(ModScope::scopeBuiltin(), true);
	loadBuildings(randomFaction->town.get(), randomFactionJson["random"]["town"]["buildings"]);
}

void CTownHandler::loadCustom()
{
	loadRandomFaction();
}

void CTownHandler::beforeValidate(JsonNode & object)
{
	if (object.Struct().count("town") == 0)
		return;

	const auto & inheritBuilding = [this](const std::string & name, JsonNode & target)
	{
		if(buildingsLibrary.Struct().count(name) == 0)
		{
			if(!target.Struct().count("id"))
				logMod->warn("Mod '%s': Town building '%s' lack ID.", target.getModScope(), name);
			return;
		}

		JsonNode baseCopy(buildingsLibrary[name]);

		if (target.Struct().count("id") && baseCopy.Struct().count("id"))
		{
			logMod->warn("Mod '%s': Town building '%s' has specified 'id' field for a predefined building! Ignoring this field.", target["id"].getModScope(), name);
			target.Struct().erase("id");
		}

		baseCopy.setModScope(target.getModScope());
		JsonUtils::inherit(target, baseCopy);
	};

	for (auto & building : object["town"]["buildings"].Struct())
	{
		if (building.second.isNull())
			continue;

		inheritBuilding(building.first, building.second);
		if (building.second.Struct().count("type"))
			inheritBuilding(building.second["type"].String(), building.second);

		// MODS COMPATIBILITY FOR pre-1.6
		// convert old buildigns with onVisitBonuses into configurable building
		if (building.second.Struct().count("onVisitBonuses"))
		{
			building.second["configuration"]["visitMode"] = JsonNode("bonus");
			building.second["configuration"]["rewards"][0]["message"] = building.second["description"];
			building.second["configuration"]["rewards"][0]["bonuses"] = building.second["onVisitBonuses"];
		}
	}
}

void CTownHandler::afterLoadFinalization()
{
	initializeRequirements();
}

void CTownHandler::initializeRequirements()
{
	// must be done separately after all ID's are known
	for (auto & requirement : requirementsToLoad)
	{
		requirement.building->requirements = CBuilding::TRequired(requirement.json, [&](const JsonNode & node) -> BuildingID
		{
			if (node.Vector().size() > 1)
			{
				logMod->error("Unexpected length of town buildings requirements: %d", node.Vector().size());
				logMod->error("Entry contains: ");
				logMod->error(node.toString());
			}

			auto index = LIBRARY->identifiers()->getIdentifier(requirement.town->getBuildingScope(), node[0]);

			if (!index.has_value())
			{
				logMod->error("Unknown building in town buildings: %s", node[0].String());
				return BuildingID::NONE;
			}
			return BuildingID(index.value());
		});
	}
	requirementsToLoad.clear();
}

std::set<FactionID> CTownHandler::getDefaultAllowed() const
{
	std::set<FactionID> allowedFactions;

	for(const auto & town : objects)
		if (town->town != nullptr && !town->special)
			allowedFactions.insert(town->getId());

	return allowedFactions;
}

std::set<FactionID> CTownHandler::getAllowedFactions(bool withTown) const
{
	if (withTown)
		return getDefaultAllowed();

	std::set<FactionID> result;
	for(const auto & town : objects)
		result.insert(town->getId());

	return result;
}

const std::vector<std::string> & CTownHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "faction", "town" };
	return typeNames;
}

VCMI_LIB_NAMESPACE_END
