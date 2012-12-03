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

void CIdentifierStorage::requestIdentifier(std::string name, const boost::function<void(si32)> & callback)
{
	auto iter = registeredObjects.find(name);

	if (iter != registeredObjects.end())
		callback(iter->second); //already registered - trigger callback immediately
	else
		missingObjects[name].push_back(callback); // queue callback
}

void CIdentifierStorage::registerObject(std::string name, si32 identifier)
{
	// do not allow to register same object twice
	assert(registeredObjects.find(name) == registeredObjects.end());

	registeredObjects[name] = identifier;

	auto iter = missingObjects.find(name);
	if (iter != missingObjects.end())
	{
		//call all awaiting callbacks
		BOOST_FOREACH(auto function, iter->second)
		{
			function(identifier);
		}
		missingObjects.erase(iter);
	}
}

void CIdentifierStorage::finalize() const
{
	// print list of missing objects and crash
	// in future should try to do some cleanup (like returning all id's as 0)
	BOOST_FOREACH(auto object, missingObjects)
	{
		tlog1 << "Error: object " << object.first << " was not found!\n";
	}
	assert(missingObjects.empty());
}

CModHandler::CModHandler()
{
	VLC->modh = this;

	loadConfigFromFile ("defaultMods");
	findAvailableMods();
	//CResourceHandler::loadModsFilesystems(); //scan for all mods
	//TODO: mod filesystem is already initialized at LibClasses launch
	//TODO: load default (last?) config
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


void CModHandler::findAvailableMods()
{
	//TODO: read mods from Mods/ folder

	auto & configList = CResourceHandler::get()->getResourcesWithName (ResourceID("CONFIG/mod.json"));

	BOOST_FOREACH(auto & entry, configList)
	{
		auto stream = entry.getLoader()->load (entry.getResourceName());
		std::unique_ptr<ui8[]> textData (new ui8[stream->getSize()]);
		stream->read (textData.get(), stream->getSize());

		tlog3 << "\t\tFound mod file: " << entry.getResourceName() << "\n";
		allMods[allMods.size()].config.reset(new JsonNode((char*)textData.get(), stream->getSize()));
	}
}

void CModHandler::loadActiveMods()
{
	BOOST_FOREACH(auto & mod, allMods)
	{
		const JsonNode & config = *mod.second.config;

		VLC->townh->load(config["factions"]);
		VLC->creh->load(config["creatures"]);
	}
	VLC->creh->buildBonusTreeForTiers(); //do that after all new creatures are loaded
	identifiers.finalize();
}

void CModHandler::reload()
{
	{
		assert(!VLC->dobjinfo->gobjs[Obj::MONSTER].empty()); //make sure that at least some def info was found

		const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::MONSTER].begin()->second;

		BOOST_FOREACH(auto & crea, VLC->creh->creatures)
		{
			if (!vstd::contains(VLC->dobjinfo->gobjs[Obj::MONSTER], crea->idNumber)) // no obj info for this type
			{
				CGDefInfo * info = new CGDefInfo(*baseInfo);
				info->subid = crea->idNumber;
				info->name = crea->advMapDef;

				VLC->dobjinfo->gobjs[Obj::MONSTER][crea->idNumber] = info;
			}
		}
	}

	{
		assert(!VLC->dobjinfo->gobjs[Obj::TOWN].empty()); //make sure that at least some def info was found

		const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::TOWN].begin()->second;
		auto & townInfos = VLC->dobjinfo->gobjs[Obj::TOWN];

		BOOST_FOREACH(auto & town, VLC->townh->towns)
		{
			auto & cientInfo = town.second.clientInfo;

			if (!vstd::contains(VLC->dobjinfo->gobjs[Obj::TOWN], town.first)) // no obj info for this type
			{
				CGDefInfo * info = new CGDefInfo(*baseInfo);
				info->subid = town.first;

				townInfos[town.first] = info;
			}
			townInfos[town.first]->name = cientInfo.advMapCastle;

			VLC->dobjinfo->villages[town.first] = new CGDefInfo(*townInfos[town.first]);
			VLC->dobjinfo->villages[town.first]->name = cientInfo.advMapVillage;

			VLC->dobjinfo->capitols[town.first] = new CGDefInfo(*townInfos[town.first]);
			VLC->dobjinfo->capitols[town.first]->name = cientInfo.advMapCapitol;
		}
	}
}
