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

#include "VCMI_Lib.h"
#include "CGeneralTextHandler.h"
#include "JsonNode.h"
#include "StringConstants.h"
#include "CCreatureHandler.h"
#include "CModHandler.h"
#include "CHeroHandler.h"
#include "CArtHandler.h"
#include "spells/CSpellHandler.h"
#include "filesystem/Filesystem.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

const int NAMES_PER_TOWN=16; // number of town names per faction in H3 files. Json can define any number

const std::map<std::string, CBuilding::EBuildMode> CBuilding::MODES =
{
	{ "normal", CBuilding::BUILD_NORMAL },
	{ "auto", CBuilding::BUILD_AUTO },
	{ "special", CBuilding::BUILD_SPECIAL },
	{ "grail", CBuilding::BUILD_GRAIL }
};

const std::map<std::string, CBuilding::ETowerHeight> CBuilding::TOWER_TYPES =
{
	{ "low", CBuilding::HEIGHT_LOW },
	{ "average", CBuilding::HEIGHT_AVERAGE },
	{ "high", CBuilding::HEIGHT_HIGH },
	{ "skyship", CBuilding::HEIGHT_SKYSHIP }
};

std::string CBuilding::getJsonKey() const
{
	return modScope + ':' + identifier;;
}

std::string CBuilding::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

std::string CBuilding::getDescriptionTranslated() const
{
	return VLC->generaltexth->translate(getDescriptionTextID());
}

std::string CBuilding::getNameTextID() const
{
	return TextIdentifier("building", modScope, town->faction->identifier, identifier, "name").get();
}

std::string CBuilding::getDescriptionTextID() const
{
	return TextIdentifier("building", modScope, town->faction->identifier, identifier, "description").get();
}

BuildingID CBuilding::getBase() const
{
	const CBuilding * build = this;
	while (build->upgrade >= 0)
	{
		build = build->town->buildings.at(build->upgrade);
	}

	return build->bid;
}

si32 CBuilding::getDistance(BuildingID buildID) const
{
	const CBuilding * build = town->buildings.at(buildID);
	int distance = 0;
	while (build->upgrade >= 0 && build != this)
	{
		build = build->town->buildings.at(build->upgrade);
		distance++;
	}
	if (build == this)
		return distance;
	return -1;
}

void CBuilding::addNewBonus(std::shared_ptr<Bonus> b, BonusList & bonusList)
{
	bonusList.push_back(b);
}

CFaction::CFaction()
{
	town = nullptr;
	index = 0;
	alignment = EAlignment::NEUTRAL;
	preferUndergroundPlacement = false;
}

CFaction::~CFaction()
{
	delete town;
}

int32_t CFaction::getIndex() const
{
	return index;
}

int32_t CFaction::getIconIndex() const
{
	return index; //???
}

std::string CFaction::getJsonKey() const
{
	return modScope + ':' + identifier;;
}

void CFaction::registerIcons(const IconRegistar & cb) const
{
	if(town)
	{
		auto & info = town->clientInfo;
		cb(info.icons[0][0], 0, "ITPT", info.iconLarge[0][0]);
		cb(info.icons[0][1], 0, "ITPT", info.iconLarge[0][1]);
		cb(info.icons[1][0], 0, "ITPT", info.iconLarge[1][0]);
		cb(info.icons[1][1], 0, "ITPT", info.iconLarge[1][1]);

		cb(info.icons[0][0] + 2, 0, "ITPA", info.iconSmall[0][0]);
		cb(info.icons[0][1] + 2, 0, "ITPA", info.iconSmall[0][1]);
		cb(info.icons[1][0] + 2, 0, "ITPA", info.iconSmall[1][0]);
		cb(info.icons[1][1] + 2, 0, "ITPA", info.iconSmall[1][1]);

		cb(index, 1, "CPRSMALL", info.towerIconSmall);
		cb(index, 1, "TWCRPORT", info.towerIconLarge);

	}
}

std::string CFaction::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
}

std::string CFaction::getNameTextID() const
{
	return TextIdentifier("faction", modScope, identifier, "name").get();
}

FactionID CFaction::getId() const
{
	return FactionID(index);
}

bool CFaction::hasTown() const
{
	return town != nullptr;
}

void CFaction::updateFrom(const JsonNode & data)
{

}

void CFaction::serializeJson(JsonSerializeFormat & handler)
{

}


CTown::CTown()
	: faction(nullptr), mageLevel(0), primaryRes(0), moatDamage(0), defaultTavernChance(0)
{
}

CTown::~CTown()
{
	for(auto & build : buildings)
		build.second.dellNull();

	for(auto & str : clientInfo.structures)
		str.dellNull();
}

std::string CTown::getRandomNameTranslated(size_t index) const
{
	return VLC->generaltexth->translate(getRandomNameTextID(index));
}

std::string CTown::getRandomNameTextID(size_t index) const
{
	return TextIdentifier("faction", faction->modScope, faction->identifier, "randomName", index).get();
}

size_t CTown::getRandomNamesCount() const
{
	return namesCount;
}

std::string CTown::getBuildingScope() const
{
	if(faction == nullptr)
		//no faction == random faction
		return "building";
	else
		return "building." + faction->getJsonKey();
}

std::set<si32> CTown::getAllBuildings() const
{
	std::set<si32> res;

	for(const auto & b : buildings)
	{
		res.insert(b.first.num);
	}

	return res;
}

const CBuilding * CTown::getSpecialBuilding(BuildingSubID::EBuildingSubID subID) const
{
	for(const auto & kvp : buildings)
	{
		if(kvp.second->subId == subID)
			return buildings.at(kvp.first);
	}
	return nullptr;
}

BuildingID::EBuildingID CTown::getBuildingType(BuildingSubID::EBuildingSubID subID) const
{
	auto building = getSpecialBuilding(subID);
	return building == nullptr ? BuildingID::NONE : building->bid.num;
}

const std::string CTown::getGreeting(BuildingSubID::EBuildingSubID subID) const
{
	return VLC->townh->getMappedValue<const std::string, BuildingSubID::EBuildingSubID>(subID, std::string(), specialMessages, false);
}

void CTown::setGreeting(BuildingSubID::EBuildingSubID subID, const std::string message) const
{
	specialMessages.insert(std::pair<BuildingSubID::EBuildingSubID, const std::string>(subID, message));
}

CTownHandler::CTownHandler()
{
	randomTown = new CTown();
	randomFaction = new CFaction();
	randomFaction->town = randomTown;
	randomTown->faction = randomFaction;
	randomFaction->identifier = "random";
	randomFaction->modScope = "core";
}

CTownHandler::~CTownHandler()
{
	delete randomTown;
}

JsonNode readBuilding(CLegacyConfigParser & parser)
{
	JsonNode ret;
	JsonNode & cost = ret["cost"];

	//note: this code will try to parse mithril as well but wil always return 0 for it
	for(const std::string & resID : GameConstants::RESOURCE_NAMES)
		cost[resID].Float() = parser.readNumber();

	cost.Struct().erase("mithril"); // erase mithril to avoid confusing validator

	parser.endLine();

	return ret;
}

TPropagatorPtr & CTownHandler::emptyPropagator()
{
	static TPropagatorPtr emptyProp(nullptr);
	return emptyProp;
}

std::vector<JsonNode> CTownHandler::loadLegacyData(size_t dataSize)
{
	std::vector<JsonNode> dest(dataSize);
	objects.resize(dataSize);

	auto getBuild = [&](size_t town, size_t building) -> JsonNode &
	{
		return dest[town]["town"]["buildings"][EBuildingType::names[building]];
	};

	CLegacyConfigParser parser("DATA/BUILDING.TXT");

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
		CLegacyConfigParser parser("DATA/BLDGNEUT.TXT");

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
		CLegacyConfigParser parser("DATA/BLDGSPEC.TXT");

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
		CLegacyConfigParser parser("DATA/DWELLING.TXT");

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
		CLegacyConfigParser typeParser("DATA/TOWNTYPE.TXT");
		CLegacyConfigParser nameParser("DATA/TOWNNAME.TXT");
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

void CTownHandler::loadBuildingRequirements(CBuilding * building, const JsonNode & source, std::vector<BuildingRequirementsHelper> & bidsToLoad)
{
	if (source.isNull())
		return;

	BuildingRequirementsHelper hlp;
	hlp.building = building;
	hlp.town = building->town;
	hlp.json = source;
	bidsToLoad.push_back(hlp);
}

template<typename R, typename K>
R CTownHandler::getMappedValue(const K key, const R defval, const std::map<K, R> & map, bool required)
{
	auto it = map.find(key);

	if(it != map.end())
		return it->second;

	if(required)
		logMod->warn("Warning: Property: '%s' is unknown. Correct the typo or update VCMI.", key);
	return defval;
}

template<typename R>
R CTownHandler::getMappedValue(const JsonNode & node, const R defval, const std::map<std::string, R> & map, bool required)
{
	if(!node.isNull() && node.getType() == JsonNode::JsonType::DATA_STRING)
		return getMappedValue<R, std::string>(node.String(), defval, map, required);
	return defval;
}

void CTownHandler::addBonusesForVanilaBuilding(CBuilding * building)
{
	std::shared_ptr<Bonus> b;
	static TPropagatorPtr playerPropagator = std::make_shared<CPropagatorNodeType>(CBonusSystemNode::ENodeTypes::PLAYER);

	if(building->subId == BuildingSubID::NONE)
	{
		if(building->bid == BuildingID::TAVERN)
		{
			b = createBonus(building, Bonus::MORALE, +1);
		}
		else if(building->bid == BuildingID::GRAIL
			&& building->town->faction != nullptr
			&& boost::algorithm::ends_with(building->town->faction->getJsonKey(), ":cove"))
		{
			static TPropagatorPtr allCreaturesPropagator(new CPropagatorNodeType(CBonusSystemNode::ENodeTypes::ALL_CREATURES));
			static auto factionLimiter = std::make_shared<CreatureFactionLimiter>(building->town->faction->getIndex());
			b = createBonus(building, Bonus::NO_TERRAIN_PENALTY, 0, allCreaturesPropagator);
			b->addLimiter(factionLimiter);
		}
	}
	else
	{
		switch(building->subId)
		{
		case BuildingSubID::BROTHERHOOD_OF_SWORD:
			b = createBonus(building, Bonus::MORALE, +2);
			building->overrideBids.insert(BuildingID::TAVERN);
			break;
		case BuildingSubID::FOUNTAIN_OF_FORTUNE:
			b = createBonus(building, Bonus::LUCK, +2);
			break;
		case BuildingSubID::SPELL_POWER_GARRISON_BONUS:
			b = createBonus(building, Bonus::PRIMARY_SKILL, +2, PrimarySkill::SPELL_POWER);
			break;
		case BuildingSubID::ATTACK_GARRISON_BONUS:
			b = createBonus(building, Bonus::PRIMARY_SKILL, +2, PrimarySkill::ATTACK);
			break;
		case BuildingSubID::DEFENSE_GARRISON_BONUS:
			b = createBonus(building, Bonus::PRIMARY_SKILL, +2, PrimarySkill::DEFENSE);
			break;
		case BuildingSubID::LIGHTHOUSE:
			b = createBonus(building, Bonus::SEA_MOVEMENT, +500, playerPropagator);
			break;
		}
	}
	if(b)
		building->addNewBonus(b, building->buildingBonuses);
}

std::shared_ptr<Bonus> CTownHandler::createBonus(CBuilding * build, Bonus::BonusType type, int val, int subtype)
{
	return createBonus(build, type, val, emptyPropagator(), subtype);
}

std::shared_ptr<Bonus> CTownHandler::createBonus(CBuilding * build, Bonus::BonusType type, int val, TPropagatorPtr & prop, int subtype)
{
	std::ostringstream descr;
	descr << build->getNameTranslated();
	return createBonusImpl(build->bid, type, val, prop, descr.str(), subtype);
}

std::shared_ptr<Bonus> CTownHandler::createBonusImpl(BuildingID building, Bonus::BonusType type, int val, TPropagatorPtr & prop, const std::string & description, int subtype)
{
	auto b = std::make_shared<Bonus>(Bonus::PERMANENT, type, Bonus::TOWN_STRUCTURE, val, building, description, subtype);

	if(prop)
		b->addPropagator(prop);

	return b;
}

void CTownHandler::loadSpecialBuildingBonuses(const JsonNode & source, BonusList & bonusList, CBuilding * building)
{
	for(auto b : source.Vector())
	{
		auto bonus = JsonUtils::parseBuildingBonus(b, building->bid, building->getNameTranslated());

		if(bonus == nullptr)
			continue;

		if(bonus->limiter != nullptr)
		{
			auto limPtr = dynamic_cast<CreatureFactionLimiter*>(bonus->limiter.get());

			if(limPtr != nullptr && limPtr->faction == (TFaction)-1)
				limPtr->faction = building->town->faction->getIndex();
		}
		//JsonUtils::parseBuildingBonus produces UNKNOWN type propagator instead of empty.
		if(bonus->propagator != nullptr
			&& bonus->propagator->getPropagatorType() == CBonusSystemNode::ENodeTypes::UNKNOWN)
				bonus->addPropagator(emptyPropagator());
		building->addNewBonus(bonus, bonusList);
	}
}

void CTownHandler::loadBuilding(CTown * town, const std::string & stringID, const JsonNode & source)
{
	assert(stringID.find(':') == std::string::npos);
	assert(!source.meta.empty());

	auto ret = new CBuilding();
	ret->bid = getMappedValue<BuildingID, std::string>(stringID, BuildingID::NONE, MappedKeys::BUILDING_NAMES_TO_TYPES, false);

	if(ret->bid == BuildingID::NONE)
		ret->bid = source["id"].isNull() ? BuildingID(BuildingID::NONE) : BuildingID(source["id"].Float());

	if (ret->bid == BuildingID::NONE)
		logMod->error("Error: Building '%s' has not internal ID and won't work properly. Correct the typo or update VCMI.", stringID);

	ret->mode = ret->bid == BuildingID::GRAIL
		? CBuilding::BUILD_GRAIL
		: getMappedValue<CBuilding::EBuildMode>(source["mode"], CBuilding::BUILD_NORMAL, CBuilding::MODES);

	ret->subId = getMappedValue<BuildingSubID::EBuildingSubID>(source["type"], BuildingSubID::NONE, MappedKeys::SPECIAL_BUILDINGS);
	ret->height = CBuilding::HEIGHT_NO_TOWER;

	if(ret->subId == BuildingSubID::LOOKOUT_TOWER
		|| ret->bid == BuildingID::GRAIL)
		ret->height = getMappedValue<CBuilding::ETowerHeight>(source["height"], CBuilding::HEIGHT_NO_TOWER, CBuilding::TOWER_TYPES);

	ret->identifier = stringID;
	ret->modScope = source.meta;
	ret->town = town;

	VLC->generaltexth->registerString(ret->getNameTextID(), source["name"].String());
	VLC->generaltexth->registerString(ret->getDescriptionTextID(), source["description"].String());

	ret->resources = TResources(source["cost"]);
	ret->produce =   TResources(source["produce"]);

	if(ret->bid == BuildingID::TAVERN)
		addBonusesForVanilaBuilding(ret);
	else if(ret->bid.IsSpecialOrGrail())
	{
		loadSpecialBuildingBonuses(source["bonuses"], ret->buildingBonuses, ret);

		if(ret->buildingBonuses.empty())
			addBonusesForVanilaBuilding(ret);

		loadSpecialBuildingBonuses(source["onVisitBonuses"], ret->onVisitBonuses, ret);

		if(!ret->onVisitBonuses.empty())
		{
			if(ret->subId == BuildingSubID::NONE)
				ret->subId = BuildingSubID::CUSTOM_VISITING_BONUS;

			for(auto & bonus : ret->onVisitBonuses)
				bonus->sid = Bonus::getSid32(ret->town->faction->getIndex(), ret->bid);
		}
	}
	//MODS COMPATIBILITY FOR 0.96
	if(!ret->produce.nonZero())
	{
		switch (ret->bid) {
			break; case BuildingID::VILLAGE_HALL: ret->produce[Res::GOLD] = 500;
			break; case BuildingID::TOWN_HALL :   ret->produce[Res::GOLD] = 1000;
			break; case BuildingID::CITY_HALL :   ret->produce[Res::GOLD] = 2000;
			break; case BuildingID::CAPITOL :     ret->produce[Res::GOLD] = 4000;
			break; case BuildingID::GRAIL :       ret->produce[Res::GOLD] = 5000;
			break; case BuildingID::RESOURCE_SILO :
			{
				switch (ret->town->primaryRes)
				{
					case Res::GOLD:
						ret->produce[ret->town->primaryRes] = 500;
						break;
					case Res::WOOD_AND_ORE:
						ret->produce[Res::WOOD] = 1;
						ret->produce[Res::ORE] = 1;
						break;
					default:
						ret->produce[ret->town->primaryRes] = 1;
						break;
				}
			}
		}
	}
	loadBuildingRequirements(ret, source["requires"], requirementsToLoad);

	if(ret->bid.IsSpecialOrGrail())
		loadBuildingRequirements(ret, source["overrides"], overriddenBidsToLoad);

	if (!source["upgrades"].isNull())
	{
		// building id and upgrades can't be the same
		if(stringID == source["upgrades"].String())
		{
			throw std::runtime_error(boost::str(boost::format("Building with ID '%s' of town '%s' can't be an upgrade of the same building.") %
												stringID % ret->town->faction->getNameTranslated()));
		}

		VLC->modh->identifiers.requestIdentifier(ret->town->getBuildingScope(), source["upgrades"], [=](si32 identifier)
		{
			ret->upgrade = BuildingID(identifier);
		});
	}
	else
		ret->upgrade = BuildingID::NONE;

	ret->town->buildings[ret->bid] = ret;

	registerObject(source.meta, ret->town->getBuildingScope(), ret->identifier, ret->bid);
}

void CTownHandler::loadBuildings(CTown * town, const JsonNode & source)
{
	for(auto & node : source.Struct())
	{
		if (!node.second.isNull())
		{
			loadBuilding(town, node.first, node.second);
		}
	}
}

void CTownHandler::loadStructure(CTown &town, const std::string & stringID, const JsonNode & source)
{
	auto ret = new CStructure();

	ret->building = nullptr;
	ret->buildable = nullptr;

	VLC->modh->identifiers.tryRequestIdentifier( source.meta, "building." + town.faction->getJsonKey(), stringID, [=, &town](si32 identifier) mutable
	{
		ret->building = town.buildings[BuildingID(identifier)];
	});

	if (source["builds"].isNull())
	{
		VLC->modh->identifiers.tryRequestIdentifier( source.meta, "building." + town.faction->getJsonKey(), stringID, [=, &town](si32 identifier) mutable
		{
			ret->building = town.buildings[BuildingID(identifier)];
		});
	}
	else
	{
		VLC->modh->identifiers.requestIdentifier("building." + town.faction->getJsonKey(), source["builds"], [=, &town](si32 identifier) mutable
		{
			ret->buildable = town.buildings[BuildingID(identifier)];
		});
	}

	ret->identifier = stringID;
	ret->pos.x = static_cast<si32>(source["x"].Float());
	ret->pos.y = static_cast<si32>(source["y"].Float());
	ret->pos.z = static_cast<si32>(source["z"].Float());

	ret->hiddenUpgrade = source["hidden"].Bool();
	ret->defName = source["animation"].String();
	ret->borderName = source["border"].String();
	ret->areaName = source["area"].String();

	town.clientInfo.structures.push_back(ret);
}

void CTownHandler::loadStructures(CTown &town, const JsonNode & source)
{
	for(auto &node : source.Struct())
	{
		if (!node.second.isNull())
			loadStructure(town, node.first, node.second);
	}
}

void CTownHandler::loadTownHall(CTown &town, const JsonNode & source)
{
	auto & dstSlots = town.clientInfo.hallSlots;
	auto & srcSlots = source.Vector();
	dstSlots.resize(srcSlots.size());

	for(size_t i=0; i<dstSlots.size(); i++)
	{
		auto & dstRow = dstSlots[i];
		auto & srcRow = srcSlots[i].Vector();
		dstRow.resize(srcRow.size());

		for(size_t j=0; j < dstRow.size(); j++)
		{
			auto & dstBox = dstRow[j];
			auto & srcBox = srcRow[j].Vector();
			dstBox.resize(srcBox.size());

			for(size_t k=0; k<dstBox.size(); k++)
			{
				auto & dst = dstBox[k];
				auto & src = srcBox[k];

				VLC->modh->identifiers.requestIdentifier("building." + town.faction->getJsonKey(), src, [&](si32 identifier)
				{
					dst = BuildingID(identifier);
				});
			}
		}
	}
}

CTown::ClientInfo::Point JsonToPoint(const JsonNode & node)
{
	CTown::ClientInfo::Point ret;
	ret.x = static_cast<si32>(node["x"].Float());
	ret.y = static_cast<si32>(node["y"].Float());
	return ret;
}

void CTownHandler::loadSiegeScreen(CTown &town, const JsonNode & source)
{
	town.clientInfo.siegePrefix = source["imagePrefix"].String();
	town.clientInfo.towerIconSmall = source["towerIconSmall"].String();
	town.clientInfo.towerIconLarge = source["towerIconLarge"].String();

	VLC->modh->identifiers.requestIdentifier("creature", source["shooter"], [&town](si32 creature)
	{
		auto crId = CreatureID(creature);
		if(!(*VLC->creh)[crId]->animation.missleFrameAngles.size())
			logMod->error("Mod '%s' error: Creature '%s' on the Archer's tower is not a shooter. Mod should be fixed. Siege will not work properly!"
				, town.faction->getNameTranslated()
				, (*VLC->creh)[crId]->getNameSingularTranslated());

		town.clientInfo.siegeShooter = crId;
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

void CTownHandler::loadClientData(CTown &town, const JsonNode & source)
{
	CTown::ClientInfo & info = town.clientInfo;

	readIcon(source["icons"]["village"]["normal"], info.iconSmall[0][0], info.iconLarge[0][0]);
	readIcon(source["icons"]["village"]["built"], info.iconSmall[0][1], info.iconLarge[0][1]);
	readIcon(source["icons"]["fort"]["normal"], info.iconSmall[1][0], info.iconLarge[1][0]);
	readIcon(source["icons"]["fort"]["built"], info.iconSmall[1][1], info.iconLarge[1][1]);

	info.hallBackground = source["hallBackground"].String();
	info.musicTheme = source["musicTheme"].String();
	info.townBackground = source["townBackground"].String();
	info.guildWindow = source["guildWindow"].String();
	info.buildingsIcons = source["buildingsIcons"].String();

	//left for back compatibility - will be removed later
	if(!source["guildBackground"].String().empty())
		info.guildBackground = source["guildBackground"].String();
	else
		info.guildBackground = "TPMAGE.bmp";
	if(!source["tavernVideo"].String().empty())
	    info.tavernVideo = source["tavernVideo"].String();
	else
		info.tavernVideo = "TAVERN.BIK";
	//end of legacy assignment

	loadTownHall(town,   source["hallSlots"]);
	loadStructures(town, source["structures"]);
	loadSiegeScreen(town, source["siege"]);
}

void CTownHandler::loadTown(CTown * town, const JsonNode & source)
{
	auto resIter = boost::find(GameConstants::RESOURCE_NAMES, source["primaryResource"].String());
	if(resIter == std::end(GameConstants::RESOURCE_NAMES))
		town->primaryRes = Res::WOOD_AND_ORE; //Wood + Ore
	else
		town->primaryRes = static_cast<ui16>(resIter - std::begin(GameConstants::RESOURCE_NAMES));

	warMachinesToLoad[town] = source["warMachine"];

	town->moatDamage = static_cast<si32>(source["moatDamage"].Float());
	town->moatHexes = source["moatHexes"].convertTo<std::vector<BattleHex> >();

	town->mageLevel = static_cast<ui32>(source["mageGuild"].Float());

	town->namesCount = 0;
	for (auto const & name : source["names"].Vector())
	{
		VLC->generaltexth->registerString(town->getRandomNameTextID(town->namesCount), name.String());
		town->namesCount += 1;
	}

	//  Horde building creature level
	for(const JsonNode &node : source["horde"].Vector())
		town->hordeLvl[(int)town->hordeLvl.size()] = static_cast<int>(node.Float());

	// town needs to have exactly 2 horde entries. Validation will take care of 2+ entries
	// but anything below 2 must be handled here
	for (size_t i=source["horde"].Vector().size(); i<2; i++)
		town->hordeLvl[(int)i] = -1;

	const JsonVector & creatures = source["creatures"].Vector();

	town->creatures.resize(creatures.size());

	for (size_t i=0; i< creatures.size(); i++)
	{
		const JsonVector & level = creatures[i].Vector();

		town->creatures[i].resize(level.size());

		for (size_t j=0; j<level.size(); j++)
		{
			VLC->modh->identifiers.requestIdentifier("creature", level[j], [=](si32 creature)
			{
				town->creatures[i][j] = CreatureID(creature);
			});
		}
	}

	town->defaultTavernChance = static_cast<ui32>(source["defaultTavern"].Float());
	/// set chance of specific hero class to appear in this town
	for(auto &node : source["tavern"].Struct())
	{
		int chance = static_cast<int>(node.second.Float());

		VLC->modh->identifiers.requestIdentifier(node.second.meta, "heroClass",node.first, [=](si32 classID)
		{
			VLC->heroh->classes[HeroClassID(classID)]->selectionProbability[town->faction->getIndex()] = chance;
		});
	}

	for(auto &node : source["guildSpells"].Struct())
	{
		int chance = static_cast<int>(node.second.Float());

		VLC->modh->identifiers.requestIdentifier(node.second.meta, "spell", node.first, [=](si32 spellID)
		{
			VLC->spellh->objects.at(spellID)->probabilities[town->faction->getIndex()] = chance;
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

void CTownHandler::loadPuzzle(CFaction &faction, const JsonNode &source)
{
	faction.puzzleMap.reserve(GameConstants::PUZZLE_MAP_PIECES);

	std::string prefix = source["prefix"].String();
	for(const JsonNode &piece : source["pieces"].Vector())
	{
		size_t index = faction.puzzleMap.size();
		SPuzzleInfo spi;

		spi.x = static_cast<si16>(piece["x"].Float());
		spi.y = static_cast<si16>(piece["y"].Float());
		spi.whenUncovered = static_cast<ui16>(piece["index"].Float());
		spi.number = static_cast<ui16>(index);

		// filename calculation
		std::ostringstream suffix;
		suffix << std::setfill('0') << std::setw(2) << index;

		spi.filename = prefix + suffix.str();

		faction.puzzleMap.push_back(spi);
	}
	assert(faction.puzzleMap.size() == GameConstants::PUZZLE_MAP_PIECES);
}

CFaction * CTownHandler::loadFromJson(const std::string & scope, const JsonNode & source, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto faction = new CFaction();

	faction->index = static_cast<TFaction>(index);
	faction->modScope = scope;
	faction->identifier = identifier;

	VLC->generaltexth->registerString(faction->getNameTextID(), source["name"].String());

	faction->creatureBg120 = source["creatureBackground"]["120px"].String();
	faction->creatureBg130 = source["creatureBackground"]["130px"].String();

	int alignment = vstd::find_pos(EAlignment::names, source["alignment"].String());
	if (alignment == -1)
		faction->alignment = EAlignment::NEUTRAL;
	else
		faction->alignment = static_cast<EAlignment::EAlignment>(alignment);
	
	auto preferUndergound = source["preferUndergroundPlacement"];
	faction->preferUndergroundPlacement = preferUndergound.isNull() ? false : preferUndergound.Bool();

	// NOTE: semi-workaround - normally, towns are supposed to have native terrains.
	// Towns without one are exceptions. So, vcmi requires nativeTerrain to be defined
	// But allows it to be defined with explicit value of "none" if town should not have native terrain
	// This is better than allowing such terrain-less towns silently, leading to issues with RMG
	faction->nativeTerrain = ETerrainId::NONE;
	if ( !source["nativeTerrain"].isNull() && source["nativeTerrain"].String() != "none")
	{
		VLC->modh->identifiers.requestIdentifier("terrain", source["nativeTerrain"], [=](int32_t index){
			faction->nativeTerrain = TerrainId(index);
		});
	}

	if (!source["town"].isNull())
	{
		faction->town = new CTown();
		faction->town->faction = faction;
		loadTown(faction->town, source["town"]);
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

	objects.push_back(object);

	if (object->town)
	{
		auto & info = object->town->clientInfo;
		info.icons[0][0] = 8 + object->index * 4 + 0;
		info.icons[0][1] = 8 + object->index * 4 + 1;
		info.icons[1][0] = 8 + object->index * 4 + 2;
		info.icons[1][1] = 8 + object->index * 4 + 3;

		VLC->modh->identifiers.requestIdentifier(scope, "object", "town", [=](si32 index)
		{
			// register town once objects are loaded
			JsonNode config = data["town"]["mapObject"];
			config["faction"].String() = name;
			config["faction"].meta = scope;
			if (config.meta.empty())// MODS COMPATIBILITY FOR 0.96
				config.meta = scope;
			VLC->objtypeh->loadSubObject(object->identifier, config, index, object->index);

			// MODS COMPATIBILITY FOR 0.96
			auto & advMap = data["town"]["adventureMap"];
			if (!advMap.isNull())
			{
				logMod->warn("Outdated town mod. Will try to generate valid templates out of fort");
				JsonNode config;
				config["animation"] = advMap["castle"];
				VLC->objtypeh->getHandlerFor(index, object->index)->addTemplate(config);
			}
		});
	}

	registerObject(scope, "faction", name, object->index);
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
		info.icons[0][0] = (GameConstants::F_NUMBER + object->index) * 2 + 0;
		info.icons[0][1] = (GameConstants::F_NUMBER + object->index) * 2 + 1;
		info.icons[1][0] = object->index * 2 + 0;
		info.icons[1][1] = object->index * 2 + 1;

		VLC->modh->identifiers.requestIdentifier(scope, "object", "town", [=](si32 index)
		{
			// register town once objects are loaded
			JsonNode config = data["town"]["mapObject"];
			config["faction"].String() = name;
			config["faction"].meta = scope;
			VLC->objtypeh->loadSubObject(object->identifier, config, index, object->index);
		});
	}

	registerObject(scope, "faction", name, object->index);
}

void CTownHandler::loadRandomFaction()
{
	static const ResourceID randomFactionPath("config/factions/random.json");

	JsonNode randomFactionJson(randomFactionPath);
	randomFactionJson.setMeta(CModHandler::scopeBuiltin(), true);
	loadBuildings(randomTown, randomFactionJson["random"]["town"]["buildings"]);
}

void CTownHandler::loadCustom()
{
	loadRandomFaction();
}

void CTownHandler::afterLoadFinalization()
{
	initializeRequirements();
	initializeOverridden();
	initializeWarMachines();
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
				logMod->warn("Unexpected length of town buildings requirements: %d", node.Vector().size());
				logMod->warn("Entry contains: ");
				logMod->warn(node.toJson());
			}
			return BuildingID(VLC->modh->identifiers.getIdentifier(requirement.town->getBuildingScope(), node.Vector()[0]).get());
		});
	}
	requirementsToLoad.clear();
}

void CTownHandler::initializeOverridden()
{
	for(auto & bidHelper : overriddenBidsToLoad)
	{
		auto jsonNode = bidHelper.json;
		auto scope = bidHelper.town->getBuildingScope();

		for(auto b : jsonNode.Vector())
		{
			auto bid = BuildingID(VLC->modh->identifiers.getIdentifier(scope, b).get());
			bidHelper.building->overrideBids.insert(bid);
		}
	}
	overriddenBidsToLoad.clear();
}

void CTownHandler::initializeWarMachines()
{
	// must be done separately after all objects are loaded
	for(auto & p : warMachinesToLoad)
	{
		CTown * t = p.first;
		JsonNode creatureKey = p.second;

		auto ret = VLC->modh->identifiers.getIdentifier("creature", creatureKey, false);

		if(ret)
		{
			const CCreature * creature = CreatureID(*ret).toCreature();

			t->warMachine = creature->warMachine;
		}
	}

	warMachinesToLoad.clear();
}

std::vector<bool> CTownHandler::getDefaultAllowed() const
{
	std::vector<bool> allowedFactions;
	for(auto town : objects)
	{
		allowedFactions.push_back(town->town != nullptr);
	}
	return allowedFactions;
}

std::set<TFaction> CTownHandler::getAllowedFactions(bool withTown) const
{
	std::set<TFaction> allowedFactions;
	std::vector<bool> allowed;
	if (withTown)
		allowed = getDefaultAllowed();
	else
		allowed.resize( objects.size(), true);

	for (size_t i=0; i<allowed.size(); i++)
		if (allowed[i])
			allowedFactions.insert((TFaction)i);

	return allowedFactions;
}

const std::vector<std::string> & CTownHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "faction", "town" };
	return typeNames;
}


VCMI_LIB_NAMESPACE_END
