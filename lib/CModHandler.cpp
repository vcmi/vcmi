#include "StdInc.h"
#include "CModHandler.h"
#include "JsonNode.h"

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
	auto hardcodedFeatures = config["hardcodedFeatures"];

	settings.CREEP_SIZE = hardcodedFeatures["CREEP_SIZE"].Float();
	settings.WEEKLY_GROWTH = hardcodedFeatures["WEEKLY_GROWTH_PERCENT"].Float();
	settings.NEUTRAL_STACK_EXP = hardcodedFeatures["NEUTRAL_STACK_EXP_DAILY"].Float();
	settings.DWELLINGS_ACCUMULATE_CREATURES = hardcodedFeatures["DWELLINGS_ACCUMULATE_CREATURES"].Bool();
	settings.ALL_CREATURES_GET_DOUBLE_MONTHS = hardcodedFeatures["ALL_CREATURES_GET_DOUBLE_MONTHS"].Bool();
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
void CModHandler::recreateHandlers()
{
	//TODO: consider some template magic to unify all handlers?

	VLC->arth->artifacts.clear(); 
	VLC->creh->creatures.clear(); //TODO: what about items from original game?

	BOOST_FOREACH (auto mod, activeMods)
	{
		BOOST_FOREACH (auto art, allMods[mod].artifacts)
		{
			VLC->arth->artifacts.push_back (artifacts[art]);
		}
		BOOST_FOREACH (auto creature, allMods[mod].creatures)
		{
			VLC->creh->creatures.push_back (creatures[creature]);
		}
	}
}

CModHandler::~CModHandler()
{
}