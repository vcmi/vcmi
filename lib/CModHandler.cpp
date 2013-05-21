#include "StdInc.h"
#include "CModHandler.h"
#include "CDefObjInfoHandler.h"
#include "JsonNode.h"
#include "filesystem/CResourceLoader.h"
#include "filesystem/ISimpleResourceLoader.h"

#include "CCreatureHandler.h"
#include "CArtHandler.h"
#include "CTownHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "StringConstants.h"
#include "CStopWatch.h"
#include "IHandlerBase.h"

/*
 * CModHandler.cpp, part of VCMI engine
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
        logGlobal->warnStream() << "BIG WARNING: identifier " << ID << " seems to be broken!";
	else
	{
		size_t pos = 0;
		do
		{
			if (std::tolower(ID[pos]) != ID[pos] ) //Not in camelCase
			{
                logGlobal->warnStream() << "Warning: identifier " << ID << " is not in camelCase!";
				ID[pos] = std::tolower(ID[pos]);// Try to fix the ID
			}
			pos = ID.find('.', pos);
		}
		while(pos++ != std::string::npos);
	}
}

CIdentifierStorage::ObjectCallback::ObjectCallback(std::string localScope, std::string remoteScope, std::string type,
                                                   std::string name, const boost::function<void(si32)> & callback):
    localScope(localScope),
    remoteScope(remoteScope),
    type(type),
    name(name),
    callback(callback)
{}

static std::pair<std::string, std::string> splitString(std::string input, char separator)
{
	std::pair<std::string, std::string> ret;
	size_t splitPos = input.find(separator);

	if (splitPos == std::string::npos)
	{
		ret.first.clear();
		ret.second = input;
	}
	else
	{
		ret.first  = input.substr(0, splitPos);
		ret.second = input.substr(splitPos + 1);
	}
	return ret;
}

void CIdentifierStorage::requestIdentifier(ObjectCallback callback)
{
	checkIdentifier(callback.type);
	checkIdentifier(callback.name);

	assert(!callback.localScope.empty());

	scheduledRequests.push_back(callback);
}

void CIdentifierStorage::requestIdentifier(std::string scope, std::string type, std::string name, const boost::function<void(si32)> & callback)
{
	auto pair = splitString(name, ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(scope, pair.first, type, pair.second, callback));
}

void CIdentifierStorage::requestIdentifier(std::string type, const JsonNode & name, const boost::function<void(si32)> & callback)
{
	auto pair = splitString(name.String(), ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(name.meta, pair.first, type, pair.second, callback));
}

void CIdentifierStorage::requestIdentifier(const JsonNode & name, const boost::function<void(si32)> & callback)
{
	auto pair  = splitString(name.String(), ':'); // remoteScope:<type.name>
	auto pair2 = splitString(pair.second,   '.'); // type.name

	requestIdentifier(ObjectCallback(name.meta, pair.first, pair2.first, pair2.second, callback));
}

void CIdentifierStorage::registerObject(std::string scope, std::string type, std::string name, si32 identifier)
{
	ObjectData data;
	data.scope = scope;
	data.id = identifier;

	std::string fullID = type + '.' + name;
	checkIdentifier(fullID);

	registeredObjects.insert(std::make_pair(fullID, data));
}

bool CIdentifierStorage::resolveIdentifier(const ObjectCallback & request)
{
	std::set<std::string> allowedScopes;

	if (request.remoteScope.empty())
	{
		// normally ID's from all required mods, own mod and virtual "core" mod are allowed
		if (request.localScope != "core")
			allowedScopes = VLC->modh->getModData(request.localScope).dependencies;

		allowedScopes.insert(request.localScope);
		allowedScopes.insert("core");
	}
	else
	{
		//...unless destination mod was specified explicitly
		auto myDeps = VLC->modh->getModData(request.localScope).dependencies;
		if (request.remoteScope == "core" ||   // allow only available to all core mod
		    myDeps.count(request.remoteScope)) // or dependencies
			allowedScopes.insert(request.remoteScope);
	}

	std::string fullID = request.type + '.' + request.name;

	auto entries = registeredObjects.equal_range(fullID);
	if (entries.first != entries.second)
	{
		for (auto it = entries.first; it != entries.second; it++)
		{
			if (vstd::contains(allowedScopes, it->second.scope))
			{
				request.callback(it->second.id);
				return true;
			}
		}

		// error found. Try to generate some debug info
		logGlobal->errorStream() << "Unknown identifier " << request.type << "." << request.name << " from mod " << request.localScope;
		for (auto it = entries.first; it != entries.second; it++)
		{
			logGlobal->errorStream() << "\tID is available in mod " << it->second.scope;
		}

		// temporary code to smooth 0.92->0.93 transition
		request.callback(entries.first->second.id);
		return true;
	}
	logGlobal->errorStream() << "Unknown identifier " << request.type << "." << request.name << " from mod " << request.localScope;
	return false;
}

void CIdentifierStorage::finalize()
{
	bool errorsFound = false;

	BOOST_FOREACH(const ObjectCallback & request, scheduledRequests)
	{
		errorsFound |= !resolveIdentifier(request);
	}

	if (errorsFound)
	{
		BOOST_FOREACH(auto object, registeredObjects)
		{
			logGlobal->traceStream() << object.first << " -> " << object.second.id;
		}
		logGlobal->errorStream() << "All known identifiers were dumped into log file";
	}
	assert(errorsFound == false);
}

CContentHandler::ContentTypeHandler::ContentTypeHandler(IHandlerBase * handler, std::string objectName):
    handler(handler),
    objectName(objectName),
    originalData(handler->loadLegacyData(VLC->modh->settings.data["textData"][objectName].Float()))
{
	BOOST_FOREACH(auto & node, originalData)
	{
		node.setMeta("core");
	}
}

void CContentHandler::ContentTypeHandler::preloadModData(std::string modName, std::vector<std::string> fileList)
{
	JsonNode data = JsonUtils::assembleFromFiles(fileList);
	data.setMeta(modName);

	ModInfo & modInfo = modData[modName];

	BOOST_FOREACH(auto entry, data.Struct())
	{
		size_t colon = entry.first.find(':');

		if (colon == std::string::npos)
		{
			// normal object, local to this mod
			modInfo.modData[entry.first].swap(entry.second);
		}
		else
		{
			std::string remoteName = entry.first.substr(0, colon);
			std::string objectName = entry.first.substr(colon + 1);

			// patching this mod? Send warning and continue - this situation can be handled normally
			if (remoteName == modName)
				logGlobal->warnStream() << "Redundant namespace definition for " << objectName;

			JsonNode & remoteConf = modData[remoteName].patches[objectName];

			JsonUtils::merge(remoteConf, entry.second);
		}
	}
}

void CContentHandler::ContentTypeHandler::loadMod(std::string modName)
{
	ModInfo & modInfo = modData[modName];

	// apply patches
	if (!modInfo.patches.isNull())
		JsonUtils::merge(modInfo.modData, modInfo.patches);

	BOOST_FOREACH(auto entry, modInfo.modData.Struct())
	{
		const std::string & name = entry.first;
		JsonNode & data = entry.second;

		if (vstd::contains(data.Struct(), "index") && !data["index"].isNull())
		{
			// try to add H3 object data
			size_t index = data["index"].Float();

			if (originalData.size() > index)
			{
				JsonUtils::merge(originalData[index], data);
				JsonUtils::validate(originalData[index], "vcmi:" + objectName, name);
				handler->loadObject(modName, name, originalData[index], index);

				originalData[index].clear(); // do not use same data twice (same ID)

				continue;
			}
		}
		// normal new object
		JsonUtils::validate(data, "vcmi:" + objectName, name);
		handler->loadObject(modName, name, data);
	}
}

CContentHandler::CContentHandler()
{
	handlers.insert(std::make_pair("heroClasses", ContentTypeHandler(&VLC->heroh->classes, "heroClass")));
	handlers.insert(std::make_pair("artifacts", ContentTypeHandler(VLC->arth, "artifact")));
	handlers.insert(std::make_pair("creatures", ContentTypeHandler(VLC->creh, "creature")));
	handlers.insert(std::make_pair("factions", ContentTypeHandler(VLC->townh, "faction")));
	handlers.insert(std::make_pair("heroes", ContentTypeHandler(VLC->heroh, "hero")));

	//TODO: spells, bonuses, something else?
}

void CContentHandler::preloadModData(std::string modName, JsonNode modConfig)
{
	BOOST_FOREACH(auto & handler, handlers)
	{
		handler.second.preloadModData(modName, modConfig[handler.first].convertTo<std::vector<std::string> >());
	}
}

void CContentHandler::loadMod(std::string modName)
{
	BOOST_FOREACH(auto & handler, handlers)
	{
		handler.second.loadMod(modName);
	}
}

CModHandler::CModHandler()
{
	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		identifiers.registerObject("core", "resource", GameConstants::RESOURCE_NAMES[i], i);
	}

	for(int i=0; i<GameConstants::PRIMARY_SKILLS; ++i)
		identifiers.registerObject("core", "primSkill", PrimarySkill::names[i], i);

}

void CModHandler::loadConfigFromFile (std::string name)
{
	settings.data = JsonUtils::assembleFromFiles("config/" + name);
	const JsonNode & hardcodedFeatures = settings.data["hardcodedFeatures"];

	settings.CREEP_SIZE = hardcodedFeatures["CREEP_SIZE"].Float();
	settings.WEEKLY_GROWTH = hardcodedFeatures["WEEKLY_GROWTH_PERCENT"].Float();
	settings.NEUTRAL_STACK_EXP = hardcodedFeatures["NEUTRAL_STACK_EXP_DAILY"].Float();
	settings.MAX_BUILDING_PER_TURN = hardcodedFeatures["MAX_BUILDING_PER_TURN"].Float();
	settings.DWELLINGS_ACCUMULATE_CREATURES = hardcodedFeatures["DWELLINGS_ACCUMULATE_CREATURES"].Bool();
	settings.ALL_CREATURES_GET_DOUBLE_MONTHS = hardcodedFeatures["ALL_CREATURES_GET_DOUBLE_MONTHS"].Bool();

	const JsonNode & gameModules = settings.data["modules"];
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
        logGlobal->errorStream() << "Error: Circular dependency detected! Printing dependency list:";
        logGlobal->errorStream() << "\t" << mod.name << " -> ";
		return true;
	}

	currentList.insert(modID);

	// recursively check every dependency of this mod
	BOOST_FOREACH(const TModID & dependency, mod.dependencies)
	{
		if (hasCircularDependency(dependency, currentList))
		{
            logGlobal->errorStream() << "\t" << mod.name << " ->\n"; // conflict detected, print dependency list
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
                logGlobal->errorStream() << "Error: Mod " << mod.name << " requires missing " << dep << "!";
				return false;
			}
		}

		BOOST_FOREACH(const TModID & conflicting, mod.conflicts)
		{
			if (vstd::contains(input, conflicting))
			{
                logGlobal->errorStream() << "Error: Mod " << mod.name << " conflicts with " << allMods.at(conflicting).name << "!";
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
	// Topological sort algorithm
	// May not be the fastest one but VCMI does not needs any speed here
	// Unless user have dozens of mods with complex dependencies this code should be fine

	// first - sort input to have input strictly based on name (and not on hashmap or anything else)
	boost::range::sort(input);

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
		std::set <TModID> toResolve; // list of mods resolved on this iteration

		for (auto it = input.begin(); it != input.end();)
		{
			if (isResolved(allMods.at(*it)))
			{
				toResolve.insert(*it);
				output.push_back(*it);
				it = input.erase(it);
				continue;
			}
			it++;
		}
		resolvedMods.insert(toResolve.begin(), toResolve.end());
	}

	return output;
}

void CModHandler::initialize(std::vector<std::string> availableMods)
{
	std::string confName = "config/modSettings.json";
	JsonNode modConfig;

	// Porbably new install. Create initial configuration
	if (!CResourceHandler::get()->existsResource(ResourceID(confName)))
		CResourceHandler::get()->createResource(confName);
	else
		modConfig = JsonNode(ResourceID(confName));

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
            logGlobal->warnStream() << "\t\t Directory " << name << " does not contains VCMI mod";
	}

	if (!checkDependencies(detectedMods))
	{
        logGlobal->errorStream() << "Critical error: failed to load mods! Exiting...";
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

CModInfo & CModHandler::getModData(TModID modId)
{
	CModInfo & mod = allMods.at(modId);
	assert(vstd::contains(activeMods, modId)); // not really necessary but won't hurt
	return mod;
}

template<typename Handler>
void CModHandler::handleData(Handler handler, const JsonNode & source, std::string listName, std::string schemaName)
{
	JsonNode config = JsonUtils::assembleFromFiles(source[listName].convertTo<std::vector<std::string> >());

	BOOST_FOREACH(auto & entry, config.Struct())
	{
		if (!entry.second.isNull()) // may happens if mod removed object by setting json entry to null
		{
			JsonUtils::validate(entry.second, schemaName, entry.first);
			handler->load(entry.first, entry.second);
		}
	}
}

void CModHandler::beforeLoad()
{
	loadConfigFromFile("defaultMods.json");
}

void CModHandler::loadGameContent()
{
	CStopWatch timer, totalTime;

	CContentHandler content;
	logGlobal->infoStream() << "\tInitializing content hander: " << timer.getDiff() << " ms";

	// first - load virtual "core" mod that contains all data
	// TODO? move all data into real mods? RoE, AB, SoD, WoG
	content.preloadModData("core", JsonNode(ResourceID("config/gameConfig.json")));
	logGlobal->infoStream() << "\tParsing original game data: " << timer.getDiff() << " ms";

	BOOST_FOREACH(const TModID & modName, activeMods)
	{
		logGlobal->infoStream() << "\t\t" << allMods[modName].name;

		std::string modFileName = "mods/" + modName + "/mod.json";

		const JsonNode config = JsonNode(ResourceID(modFileName));
		JsonUtils::validate(config, "vcmi:mod", modName);

		content.preloadModData(modName, config);
	}
	logGlobal->infoStream() << "\tParsing mod data: " << timer.getDiff() << " ms";

	content.loadMod("core");
	logGlobal->infoStream() << "\tLoading original game data: " << timer.getDiff() << " ms";

	BOOST_FOREACH(const TModID & modName, activeMods)
	{
		content.loadMod(modName);
		logGlobal->infoStream() << "\t\t" << allMods[modName].name;
	}
	logGlobal->infoStream() << "\tLoading mod data: " << timer.getDiff() << "ms";

	VLC->creh->loadCrExpBon();
	VLC->creh->buildBonusTreeForTiers(); //do that after all new creatures are loaded
	identifiers.finalize();

	logGlobal->infoStream() << "\tResolving identifiers: " << timer.getDiff() << " ms";
	logGlobal->infoStream() << "\tAll game content loaded in " << totalTime.getDiff() << " ms";
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
	}

	{
		assert(!VLC->dobjinfo->gobjs[Obj::TOWN].empty()); //make sure that at least some def info was found

		const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::TOWN].begin()->second;
		auto & townInfos = VLC->dobjinfo->gobjs[Obj::TOWN];

		BOOST_FOREACH(auto & faction, VLC->townh->factions)
		{
			TFaction index = faction->index;
			CTown * town = faction->town;
			if (town)
			{
				auto & cientInfo = town->clientInfo;

				if (!vstd::contains(VLC->dobjinfo->gobjs[Obj::TOWN], index)) // no obj info for this type
				{
					CGDefInfo * info = new CGDefInfo(*baseInfo);
					info->subid = index;

					townInfos[index] = info;
				}
				townInfos[index]->name = cientInfo.advMapCastle;

				VLC->dobjinfo->villages[index] = new CGDefInfo(*townInfos[index]);
				VLC->dobjinfo->villages[index]->name = cientInfo.advMapVillage;

				VLC->dobjinfo->capitols[index] = new CGDefInfo(*townInfos[index]);
				VLC->dobjinfo->capitols[index]->name = cientInfo.advMapCapitol;

				for (int i = 0; i < town->dwellings.size(); ++i)
				{
					const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::CREATURE_GENERATOR1][i]; //get same blockmap as first dwelling of tier i

					BOOST_FOREACH (auto cre, town->creatures[i]) //both unupgraded and upgraded get same dwelling
					{
						CGDefInfo * info = new CGDefInfo(*baseInfo);
						info->subid = cre;
						info->name = town->dwellings[i];
						VLC->dobjinfo->gobjs[Obj::CREATURE_GENERATOR1][cre] = info;

						VLC->objh->cregens[cre] = cre; //map of dwelling -> creature id
					}
				}
			}
		}
	}
}
