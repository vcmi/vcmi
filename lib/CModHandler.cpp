#include "StdInc.h"
#include "CModHandler.h"
#include "CDefObjInfoHandler.h"
#include "JsonNode.h"
#include "Filesystem/CResourceLoader.h"
#include "Filesystem/ISimpleResourceLoader.h"

#include "CCreatureHandler.h"
#include "CArtHandler.h"
#include "CTownHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "StringConstants.h"

/*
 * CModHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void CIdentifierStorage::checkIdentifier(std::string & ID)
{
	if (boost::algorithm::ends_with(ID, "."))
		tlog0 << "BIG WARNING: identifier " << ID << " seems to be broken!\n";
	else
	{
		size_t pos = 0;
		do
		{
			if (std::tolower(ID[pos]) != ID[pos] ) //Not in camelCase
			{
				tlog0 << "Warning: identifier " << ID << " is not in camelCase!\n";
				ID[pos] = std::tolower(ID[pos]);// Try to fix the ID
			}
			pos = ID.find('.', pos);
		}
		while(pos++ != std::string::npos);
	}
}

void CIdentifierStorage::requestIdentifier(std::string name, const boost::function<void(si32)> & callback)
{
	checkIdentifier(name);

	auto iter = registeredObjects.find(name);

	if (iter != registeredObjects.end())
		callback(iter->second); //already registered - trigger callback immediately
	else
	{
		if(boost::algorithm::starts_with(name, "primSkill."))
			tlog2 << "incorrect primSkill name requested\n";

		missingObjects[name].push_back(callback); // queue callback
	}
}

void CIdentifierStorage::registerObject(std::string name, si32 identifier)
{
	checkIdentifier(name);

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
	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		identifiers.registerObject("resource." + GameConstants::RESOURCE_NAMES[i], i);
	}

	for(int i=0; i<GameConstants::PRIMARY_SKILLS; ++i)
		identifiers.registerObject("primSkill." + PrimarySkill::names[i], i);

	loadConfigFromFile ("defaultMods");
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
}

// currentList is passed by value to get current list of depending mods
bool CModHandler::hasCircularDependency(TModID modID, std::set <TModID> currentList) const
{
	const CModInfo & mod = allMods.at(modID);

	// Mod already present? We found a loop
	if (vstd::contains(currentList, modID))
	{
		tlog0 << "Error: Circular dependency detected! Printing dependency list:\n";
		tlog0 << "\t" << mod.name << " -> \n";
		return true;
	}

	currentList.insert(modID);

	// recursively check every dependency of this mod
	BOOST_FOREACH(const TModID & dependency, mod.dependencies)
	{
		if (hasCircularDependency(dependency, currentList))
		{
			tlog0 << "\t" << mod.name << " ->\n"; // conflict detected, print dependency list
			return true;
		}
	}
	return false;
}

bool CModHandler::checkDependencies(const std::vector <TModID> & input) const
{
	BOOST_FOREACH(const TModID & id, input)
	{
		const CModInfo & mod = allMods.at(id);

		BOOST_FOREACH(const TModID & dep, mod.dependencies)
		{
			if (!vstd::contains(input, dep))
			{
				tlog0 << "Error: Mod " << mod.name << " requires missing " << dep << "!\n";
				return false;
			}
		}

		BOOST_FOREACH(const TModID & conflicting, mod.conflicts)
		{
			if (vstd::contains(input, conflicting))
			{
				tlog0 << "Error: Mod " << mod.name << " conflicts with " << allMods.at(conflicting).name << "!\n";
				return false;
			}
		}

		if (hasCircularDependency(id))
			return false;
	}
	return true;
}

std::vector <TModID> CModHandler::resolveDependencies(std::vector <TModID> input) const
{
	// Algorithm may not be the fastest one but VCMI does not needs any speed here
	// Unless user have dozens of mods with complex dependencies this cide should be fine

	std::vector <TModID> output;
	output.reserve(input.size());

	std::set <TModID> resolvedMods;

	// Check if all mod dependencies are resolved (moved to resolvedMods)
	auto isResolved = [&](const CModInfo mod) -> bool
	{
		BOOST_FOREACH(const TModID & dependency, mod.dependencies)
		{
			if (!vstd::contains(resolvedMods, dependency))
				return false;
		}
		return true;
	};

	while (!input.empty())
	{
		for (auto it = input.begin(); it != input.end();)
		{
			if (isResolved(allMods.at(*it)))
			{
				resolvedMods.insert(*it);
				output.push_back(*it);
				it = input.erase(it);
				continue;
			}
			it++;
		}
	}

	return output;
}

void CModHandler::initialize(std::vector<std::string> availableMods)
{
	JsonNode modConfig(ResourceID("config/modSettings.json"));
	const JsonNode & modList = modConfig["activeMods"];
	JsonNode resultingList;

	std::vector <TModID> detectedMods;

	BOOST_FOREACH(std::string name, availableMods)
	{
		boost::to_lower(name);
		std::string modFileName = "mods/" + name + "/mod.json";

		if (CResourceHandler::get()->existsResource(ResourceID(modFileName)))
		{
			const JsonNode config = JsonNode(ResourceID(modFileName));

			if (config.isNull())
				continue;

			if (!modList[name].isNull() && modList[name].Bool() == false )
			{
				resultingList[name].Bool() = false;
				continue; // disabled mod
			}
			resultingList[name].Bool() = true;

			CModInfo & mod = allMods[name];

			mod.identifier = name;
			mod.name = config["name"].String();
			mod.description =  config["description"].String();
			mod.dependencies = config["depends"].convertTo<std::set<std::string> >();
			mod.conflicts =    config["conflicts"].convertTo<std::set<std::string> >();
			detectedMods.push_back(name);
		}
		else
			tlog3 << "\t\t Directory " << name << " does not contains VCMI mod\n";
	}

	if (!checkDependencies(detectedMods))
	{
		tlog0 << "Critical error: failed to load mods! Exiting...\n";
		exit(1);
	}

	activeMods = resolveDependencies(detectedMods);

	modConfig["activeMods"] = resultingList;
	CResourceHandler::get()->createResource("CONFIG/modSettings.json");

	std::ofstream file(CResourceHandler::get()->getResourceName(ResourceID("config/modSettings.json")), std::ofstream::trunc);
	file << modConfig;
}


std::vector<std::string> CModHandler::getActiveMods()
{
	return activeMods;
}

template<typename Handler>
void handleData(Handler handler, const JsonNode & config)
{
	handler->load(JsonUtils::assembleFromFiles(config.convertTo<std::vector<std::string> >()));
}

void CModHandler::loadActiveMods()
{
	BOOST_FOREACH(const TModID & modName, activeMods)
	{
		tlog1 << "\t\tLoading mod ";
		tlog2 << allMods[modName].name << "\n";

		std::string modFileName = "mods/" + modName + "/mod.json";

		const JsonNode config = JsonNode(ResourceID(modFileName));

		handleData(VLC->townh, config["factions"]);
		handleData(VLC->creh, config["creatures"]);
		handleData(VLC->arth, config["artifacts"]);
		//todo: spells

		handleData(&VLC->heroh->classes, config["heroClasses"]);
		handleData(VLC->heroh, config["heroes"]);
	}

	VLC->creh->buildBonusTreeForTiers(); //do that after all new creatures are loaded
	identifiers.finalize();
}

void CModHandler::reload()
{
	{
		//recreate adventure map defs
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
		assert(!VLC->dobjinfo->gobjs[Obj::ARTIFACT].empty());

		const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::ARTIFACT].begin()->second;

		BOOST_FOREACH(auto & art, VLC->arth->artifacts)
		{
			if (!vstd::contains(VLC->dobjinfo->gobjs[Obj::ARTIFACT], art->id)) // no obj info for this type
			{
				CGDefInfo * info = new CGDefInfo(*baseInfo);
				info->subid = art->id;
				info->name = art->advMapDef;

				VLC->dobjinfo->gobjs[Obj::ARTIFACT][art->id] = info;
			}
		}

		VLC->arth->reverseMapArtifactConstituents();
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

			for (int i = 0; i < town.second.dwellings.size(); ++i)
			{
				const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::CREATURE_GENERATOR1][i]; //get same blockmap as first dwelling of tier i

				BOOST_FOREACH (auto cre, town.second.creatures[i]) //both unupgraded and upgraded get same dwelling
				{
					CGDefInfo * info = new CGDefInfo(*baseInfo);
					info->subid = cre;
					info->name = town.second.dwellings[i];
					VLC->dobjinfo->gobjs[Obj::CREATURE_GENERATOR1][cre] = info;

					VLC->objh->cregens[cre] = cre; //map of dwelling -> creature id
				}
			}
		}
	}
}
