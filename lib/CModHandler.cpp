#include "StdInc.h"
#include "CModHandler.h"
#include "CDefObjInfoHandler.h"
#include "JsonNode.h"
#include "Filesystem/CResourceLoader.h"
#include "Filesystem/ISimpleResourceLoader.h"
/*
 * CModHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CBuildingHandler;
class CObjectHandler;
class CDefObjInfoHandler;
class CTownHandler;
class CGeneralTextHandler;
class ResourceLocator;

CModHandler::CModHandler()
{
	VLC->modh = this;

	loadConfigFromFile ("defaultMods");
	//CResourceHandler::loadModsFilesystems(); //scan for all mods
	//TODO: mod filesystem is already initialized at LibClasses launch
	//TODO: load default (last?) config
}
artID CModHandler::addNewArtifact (CArtifact * art)
{
	int id = artifacts.size();
	artifacts.push_back (art);
	return id;
}
creID CModHandler::addNewCreature (CCreature * cre)
{
	int id = creatures.size();
	creatures.push_back (cre);
	return id;
}

void CModHandler::loadConfigFromFile (std::string name)
{

	const JsonNode config(ResourceID("config/" + name + ".json"));
	const JsonNode & hardcodedFeatures = config["hardcodedFeatures"];

	settings.CREEP_SIZE = hardcodedFeatures["CREEP_SIZE"].Float();
	settings.WEEKLY_GROWTH = hardcodedFeatures["WEEKLY_GROWTH_PERCENT"].Float();
	settings.NEUTRAL_STACK_EXP = hardcodedFeatures["NEUTRAL_STACK_EXP_DAILY"].Float();
	settings.MAX_BUILDING_PER_TURN = hardcodedFeatures["MAX_BUILDING_PER_TURN"].Float();
	settings.DWELLINGS_ACCUMULATE_CREATURES = hardcodedFeatures["DWELLINGS_ACCUMULATE_CREATURES"].Bool();
	settings.ALL_CREATURES_GET_DOUBLE_MONTHS = hardcodedFeatures["ALL_CREATURES_GET_DOUBLE_MONTHS"].Bool();

	const JsonNode & gameModules = config["modules"];
	modules.STACK_EXP = gameModules["STACK_EXPERIENCE"].Bool();
	modules.STACK_ARTIFACT = gameModules["STACK_ARTIFACTS"].Bool();
	modules.COMMANDERS = gameModules["COMMANDERS"].Bool();
	modules.MITHRIL = gameModules["MITHRIL"].Bool();

	//TODO: load only mods from the list

	//TODO: read mods from Mods/ folder

	auto & configList = CResourceHandler::get()->getResourcesWithName (ResourceID("CONFIG/mod.json"));

	BOOST_FOREACH(auto & entry, configList)
	{
		auto stream = entry.getLoader()->load (entry.getResourceName());
		std::unique_ptr<ui8[]> textData (new ui8[stream->getSize()]);
		stream->read (textData.get(), stream->getSize());

		tlog3 << "\t\tFound mod file: " << entry.getResourceName() << "\n";
		const JsonNode config ((char*)textData.get(), stream->getSize());

		const JsonNode *value = &config["creatures"];
		BOOST_FOREACH (auto creature, value->Vector())
		{
			auto cre = loadCreature (creature); //create and push back creature
		}
	}
}

void CModHandler::saveConfigToFile (std::string name)
{
	//JsonNode savedConf = config;
	//JsonNode schema(ResourceID("config/defaultSettings.json"));

	//savedConf.Struct().erase("session");
	//savedConf.minimize(schema);

	CResourceHandler::get()->createResource("config/" + name +".json");

	std::ofstream file(CResourceHandler::get()->getResourceName(ResourceID("config/" + name +".json")), std::ofstream::trunc);
	//file << savedConf;
}

CCreature * CModHandler::loadCreature (const JsonNode &node)
{
	CCreature * cre = new CCreature();
	const JsonNode *value; //optional value

	//TODO: ref name?
	const JsonNode & name = node["name"];
	cre->nameSing = name["singular"].String();
	cre->namePl = name["plural"].String();
	cre->nameRef = cre->nameSing;

	//TODO: export resource set to separate function?
	const JsonNode &  cost = node["cost"];
	if (cost.getType() == JsonNode::DATA_FLOAT) //gold
	{
		cre->cost[Res::GOLD] = cost.Float();
	}
	else if (cost.getType() == JsonNode::DATA_VECTOR)
	{
		int i = 0;
		BOOST_FOREACH (auto & val, cost.Vector())
		{
			cre->cost[i++] = val.Float();
		}
	}
	else //damn you...
	{
		value = &cost["gold"];
		if (!value->isNull())
			cre->cost[Res::GOLD] = value->Float();
		value = &cost["gems"];
		if (!value->isNull())
			cre->cost[Res::GEMS] = value->Float();
		value = &cost["crystal"];
		if (!value->isNull())
			cre->cost[Res::CRYSTAL] = value->Float();
		value = &cost["mercury"];
		if (!value->isNull())
			cre->cost[Res::MERCURY] = value->Float();
		value = &cost["sulfur"];
		if (!value->isNull())
			cre->cost[Res::SULFUR] = value->Float();
		value = &cost["ore"];
		if (!value->isNull())
			cre->cost[Res::ORE] = value->Float();
		value = &cost["wood"];
		if (!value->isNull())
			cre->cost[Res::WOOD] = value->Float();
		value = &cost["mithril"];
		if (!value->isNull())
			cre->cost[Res::MITHRIL] = value->Float();
	}

	cre->level = node["level"].Float();
	cre->faction = -1; //neutral
	//TODO: node["faction"].String() to id or just node["faction"].Float();
	cre->fightValue = node["fightValue"].Float();
	cre->AIValue = node["aiValue"].Float();
	cre->growth = node["growth"].Float();

	cre->addBonus(node["hitPoints"].Float(), Bonus::STACK_HEALTH);
	cre->addBonus(node["speed"].Float(), Bonus::STACKS_SPEED);
	cre->addBonus(node["attack"].Float(), Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);
	cre->addBonus(node["defense"].Float(), Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);
	const JsonNode &  vec = node["damage"];
	cre->addBonus(vec["min"].Float(), Bonus::CREATURE_DAMAGE, 1);
	cre->addBonus(vec["max"].Float(), Bonus::CREATURE_DAMAGE, 2);

	auto & amounts = node ["advMapAmount"];
	cre->ammMin = amounts["min"].Float();
	cre->ammMax = amounts["max"].Float();

	//optional
	value = &node["upgrades"];
	if (!value->isNull())
	{
		BOOST_FOREACH (auto & str, value->Vector())
		{
			cre->upgradeNames.insert (str.String());
		}
	}

	value = &node["shots"];
	if (!value->isNull())
		cre->addBonus(value->Float(), Bonus::SHOTS);

	value = &node["spellPoints"];
	if (!value->isNull())
		cre->addBonus(value->Float(), Bonus::CASTS);

	value = &node["doubleWide"];
	if (!value->isNull())
		cre->doubleWide = value->Bool();
	else
		cre->doubleWide = false;

	value = &node["abilities"];
	if (!value->isNull())
	{
		BOOST_FOREACH (const JsonNode &bonus, value->Vector())
		{
			cre->addNewBonus(ParseBonus(bonus));
		}
	}
	//graphics

	const JsonNode & graphics = node["graphics"];
	cre->animDefName = graphics["animation"].String();
	cre->timeBetweenFidgets = graphics["timeBetweenFidgets"].Float();
	cre->troopCountLocationOffset = graphics["troopCountLocationOffset"].Float();
	cre->attackClimaxFrame = graphics["attackClimaxFrame"].Float();

	const JsonNode & animationTime = graphics["animationTime"];
	cre->walkAnimationTime = animationTime["walk"].Float();
	cre->attackAnimationTime = animationTime["attack"].Float();
	cre->flightAnimationDistance = animationTime["flight"].Float(); //?
	//TODO: background?
	const JsonNode & missle = graphics["missle"];
	const JsonNode & offsets = missle["offset"];
	cre->upperRightMissleOffsetX = offsets["upperX"].Float();
	cre->upperRightMissleOffsetY = offsets["upperY"].Float();
	cre->rightMissleOffsetX = offsets["middleX"].Float();
	cre->rightMissleOffsetY = offsets["middleY"].Float();
	cre->lowerRightMissleOffsetX = offsets["lowerX"].Float();
	cre->lowerRightMissleOffsetY = offsets["lowerY"].Float();
	int i = 0;
	BOOST_FOREACH (auto & angle, missle["frameAngles"].Vector())
	{
		cre->missleFrameAngles[i++] = angle.Float();
	}
	cre->advMapDef = graphics["map"].String();
	//TODO: we need to know creature id to add it
	//FIXME: creature handler is not yet initialized
	//VLC->creh->idToProjectile[cre->idNumber] = "PLCBOWX.DEF";
	cre->projectile = "PLCBOWX.DEF";

	const JsonNode & sounds = node["sound"];

#define GET_SOUND_VALUE(value_name) do { value = &sounds[#value_name]; if (!value->isNull()) cre->sounds.value_name = value->String(); } while(0)
	GET_SOUND_VALUE(attack);
	GET_SOUND_VALUE(defend);
	GET_SOUND_VALUE(killed);
	GET_SOUND_VALUE(move);
	GET_SOUND_VALUE(shoot);
	GET_SOUND_VALUE(wince);
	GET_SOUND_VALUE(ext1);
	GET_SOUND_VALUE(ext2);
	GET_SOUND_VALUE(startMoving);
	GET_SOUND_VALUE(endMoving);
#undef GET_SOUND_VALUE

	creatures.push_back(cre);
	return cre;
}
void CModHandler::recreateHandlers()
{
	//TODO: consider some template magic to unify all handlers?

	//VLC->arth->artifacts.clear();
	//VLC->creh->creatures.clear(); //TODO: what about items from original game?

	BOOST_FOREACH (auto creature, creatures)
	{
		creature->idNumber = VLC->creh->creatures.size(); //calculate next index for every used creature
		VLC->creh->creatures.push_back (creature);
		//TODO: use refName?
		//if (creature->nameRef.size())
		//	VLC->creh->nameToID[creature->nameRef] = creature->idNumber;
		VLC->creh->nameToID[creature->nameSing] = creature->idNumber;

		//generate adventure map object info & graphics
		CGDefInfo* nobj = new CGDefInfo (*VLC->dobjinfo->gobjs[GameConstants::CREI_TYPE][0]);//copy all typical properties
		nobj->name = creature->advMapDef; //change only def name (?)
		VLC->dobjinfo->gobjs[GameConstants::CREI_TYPE][creature->idNumber] = nobj;
	}
	BOOST_FOREACH (auto creature, VLC->creh->creatures) //populate upgrades described with string
	{
		BOOST_FOREACH (auto upgradeName, creature->upgradeNames)
		{
			auto it = VLC->creh->nameToID.find(upgradeName);
			if (it != VLC->creh->nameToID.end())
			{
				creature->upgrades.insert (it->second);
			}
		}
	}

	VLC->creh->buildBonusTreeForTiers(); //do that after all new creatures are loaded

	BOOST_FOREACH (auto mod, activeMods) //inactive part
	{
		BOOST_FOREACH (auto art, allMods[mod].artifacts)
		{
			VLC->arth->artifacts.push_back (artifacts[art]);

			//TODO: recreate types / limiters based on string id
		}
		BOOST_FOREACH (auto creature, allMods[mod].creatures)
		{
			VLC->creh->creatures.push_back (creatures[creature]);
			//TODO VLC->creh->notUsedMonster.push_back (creatures[creature]);

			//TODO: recreate upgrades and other properties based on string id
		}
	}
}

CModHandler::~CModHandler()
{
}
