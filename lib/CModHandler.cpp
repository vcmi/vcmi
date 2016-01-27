#include "StdInc.h"
#include "CModHandler.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "JsonNode.h"
#include "filesystem/FileStream.h"
#include "filesystem/Filesystem.h"
#include "filesystem/AdapterLoaders.h"
#include "filesystem/CFilesystemLoader.h"

#include "CCreatureHandler.h"
#include "CArtHandler.h"
#include "CTownHandler.h"
#include "CHeroHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "StringConstants.h"
#include "CStopWatch.h"
#include "IHandlerBase.h"
#include "spells/CSpellHandler.h"

/*
 * CModHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CIdentifierStorage::CIdentifierStorage():
	state(LOADING)
{
}

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
												   std::string name, const std::function<void(si32)> & callback, bool optional):
    localScope(localScope),
    remoteScope(remoteScope),
    type(type),
    name(name),
	callback(callback),
	optional(optional)
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

	if (state != FINISHED) // enqueue request if loading is still in progress
		scheduledRequests.push_back(callback);
	else // execute immediately for "late" requests
		resolveIdentifier(callback);
}

void CIdentifierStorage::requestIdentifier(std::string scope, std::string type, std::string name, const std::function<void(si32)> & callback)
{
	auto pair = splitString(name, ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(scope, pair.first, type, pair.second, callback, false));
}

void CIdentifierStorage::requestIdentifier(std::string scope, std::string fullName, const std::function<void(si32)>& callback)
{
	auto scopeAndFullName = splitString(fullName, ':');
	auto typeAndName = splitString(scopeAndFullName.second, '.');

	requestIdentifier(ObjectCallback(scope, scopeAndFullName.first, typeAndName.first, typeAndName.second, callback, false));
}

void CIdentifierStorage::requestIdentifier(std::string type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	auto pair = splitString(name.String(), ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(name.meta, pair.first, type, pair.second, callback, false));
}

void CIdentifierStorage::requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback)
{
	auto pair  = splitString(name.String(), ':'); // remoteScope:<type.name>
	auto pair2 = splitString(pair.second,   '.'); // type.name

	requestIdentifier(ObjectCallback(name.meta, pair.first, pair2.first, pair2.second, callback, false));
}

void CIdentifierStorage::tryRequestIdentifier(std::string scope, std::string type, std::string name, const std::function<void(si32)> & callback)
{
	auto pair = splitString(name, ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(scope, pair.first, type, pair.second, callback, true));
}

void CIdentifierStorage::tryRequestIdentifier(std::string type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	auto pair = splitString(name.String(), ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(name.meta, pair.first, type, pair.second, callback, true));
}

boost::optional<si32> CIdentifierStorage::getIdentifier(std::string scope, std::string type, std::string name, bool silent)
{
	auto pair = splitString(name, ':'); // remoteScope:name
	auto idList = getPossibleIdentifiers(ObjectCallback(scope, pair.first, type, pair.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logGlobal->errorStream() << "Failed to resolve identifier " << name << " of type " << type << " from mod " << scope;

	return boost::optional<si32>();
}

boost::optional<si32> CIdentifierStorage::getIdentifier(std::string type, const JsonNode & name, bool silent)
{
	auto pair = splitString(name.String(), ':'); // remoteScope:name
	auto idList = getPossibleIdentifiers(ObjectCallback(name.meta, pair.first, type, pair.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logGlobal->errorStream() << "Failed to resolve identifier " << name.String() << " of type " << type << " from mod " << name.meta;

	return boost::optional<si32>();
}

boost::optional<si32> CIdentifierStorage::getIdentifier(const JsonNode & name, bool silent)
{
	auto pair  = splitString(name.String(), ':'); // remoteScope:<type.name>
	auto pair2 = splitString(pair.second,   '.'); // type.name
	auto idList = getPossibleIdentifiers(ObjectCallback(name.meta, pair.first, pair2.first, pair2.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logGlobal->errorStream() << "Failed to resolve identifier " << name.String() << " of type " << pair2.first << " from mod " << name.meta;

	return boost::optional<si32>();
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

std::vector<CIdentifierStorage::ObjectData> CIdentifierStorage::getPossibleIdentifiers(const ObjectCallback & request)
{
	std::set<std::string> allowedScopes;

	if (request.remoteScope.empty())
	{
		// normally ID's from all required mods, own mod and virtual "core" mod are allowed
		if (request.localScope != "core" && request.localScope != "")
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
		std::vector<ObjectData> locatedIDs;

		for (auto it = entries.first; it != entries.second; it++)
		{
			if (vstd::contains(allowedScopes, it->second.scope))
			{
				locatedIDs.push_back(it->second);
			}
		}
		return locatedIDs;
	}
	return std::vector<ObjectData>();
}

bool CIdentifierStorage::resolveIdentifier(const ObjectCallback & request)
{
	auto identifiers = getPossibleIdentifiers(request);
	if (identifiers.size() == 1) // normally resolved ID
	{
		request.callback(identifiers.front().id);
		return true;
	}

	if (request.optional && identifiers.empty()) // failed to resolve optinal ID
	{
		return true;
	}

	// error found. Try to generate some debug info
	if (identifiers.size() == 0)
		logGlobal->errorStream() << "Unknown identifier!";
	else
		logGlobal->errorStream() << "Ambiguous identifier request!";

	 logGlobal->errorStream() << "Request for " << request.type << "." << request.name << " from mod " << request.localScope;

	for (auto id : identifiers)
	{
		logGlobal->errorStream() << "\tID is available in mod " << id.scope;
	}
	return false;
}

void CIdentifierStorage::finalize()
{
	state = FINALIZING;
	bool errorsFound = false;

	//Note: we may receive new requests during resolution phase -> end may change -> range for can't be used
	for(auto it = scheduledRequests.begin(); it != scheduledRequests.end(); it++)
	{
		errorsFound |= !resolveIdentifier(*it);
	}

	if (errorsFound)
	{
		for(auto object : registeredObjects)
		{
			logGlobal->traceStream() << object.second.scope << " : " << object.first << " -> " << object.second.id;
		}
		logGlobal->errorStream() << "All known identifiers were dumped into log file";
	}
	assert(errorsFound == false);
	state = FINISHED;
}

CContentHandler::ContentTypeHandler::ContentTypeHandler(IHandlerBase * handler, std::string objectName):
    handler(handler),
    objectName(objectName),
    originalData(handler->loadLegacyData(VLC->modh->settings.data["textData"][objectName].Float()))
{
	for(auto & node : originalData)
	{
		node.setMeta("core");
	}
}

bool CContentHandler::ContentTypeHandler::preloadModData(std::string modName, std::vector<std::string> fileList, bool validate)
{
	bool result;
	JsonNode data = JsonUtils::assembleFromFiles(fileList, result);
	data.setMeta(modName);

	ModInfo & modInfo = modData[modName];

	for(auto entry : data.Struct())
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

			logGlobal->traceStream() << "Patching object " << objectName << " (" << remoteName << ") from " << modName;
			JsonNode & remoteConf = modData[remoteName].patches[objectName];

			JsonUtils::merge(remoteConf, entry.second);
		}
	}
	return result;
}

bool CContentHandler::ContentTypeHandler::loadMod(std::string modName, bool validate)
{
	ModInfo & modInfo = modData[modName];
	bool result = true;

	auto performValidate = [&,this](JsonNode & data, const std::string & name){
		handler->beforeValidate(data);
		if (validate)
			result &= JsonUtils::validate(data, "vcmi:" + objectName, name);
	};

	// apply patches
	if (!modInfo.patches.isNull())
		JsonUtils::merge(modInfo.modData, modInfo.patches);

	for(auto & entry : modInfo.modData.Struct())
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

				performValidate(originalData[index],name);
				handler->loadObject(modName, name, originalData[index], index);

				originalData[index].clear(); // do not use same data twice (same ID)

				continue;
			}
		}
		// normal new object or one with index bigger that data size
		performValidate(data,name);
		handler->loadObject(modName, name, data);
	}
	return result;
}

void CContentHandler::ContentTypeHandler::afterLoadFinalization()
{
	handler->afterLoadFinalization();
}

CContentHandler::CContentHandler()
{
 	handlers.insert(std::make_pair("heroClasses", ContentTypeHandler(&VLC->heroh->classes, "heroClass")));
	handlers.insert(std::make_pair("artifacts", ContentTypeHandler(VLC->arth, "artifact")));
	handlers.insert(std::make_pair("creatures", ContentTypeHandler(VLC->creh, "creature")));
	handlers.insert(std::make_pair("factions", ContentTypeHandler(VLC->townh, "faction")));
	handlers.insert(std::make_pair("objects", ContentTypeHandler(VLC->objtypeh, "object")));
	handlers.insert(std::make_pair("heroes", ContentTypeHandler(VLC->heroh, "hero")));
	handlers.insert(std::make_pair("spells", ContentTypeHandler(VLC->spellh, "spell")));
	handlers.insert(std::make_pair("templates", ContentTypeHandler((IHandlerBase *)VLC->tplh, "template")));

	//TODO: any other types of moddables?
}

bool CContentHandler::preloadModData(std::string modName, JsonNode modConfig, bool validate)
{
	bool result = true;
	for(auto & handler : handlers)
	{
		result &= handler.second.preloadModData(modName, modConfig[handler.first].convertTo<std::vector<std::string> >(), validate);
	}
	return result;
}

bool CContentHandler::loadMod(std::string modName, bool validate)
{
	bool result = true;
	for(auto & handler : handlers)
	{
		result &= handler.second.loadMod(modName, validate);
	}
	return result;
}

void CContentHandler::afterLoadFinalization()
{
	for(auto & handler : handlers)
	{
		handler.second.afterLoadFinalization();
	}
}

void CContentHandler::preloadData(CModInfo & mod)
{
	bool validate = (mod.validation != CModInfo::PASSED);

	// print message in format [<8-symbols checksum>] <modname>
	logGlobal->infoStream() << "\t\t[" << std::noshowbase << std::hex << std::setw(8) << std::setfill('0')
							<< mod.checksum << "] " << mod.name;

	if (validate && mod.identifier != "core")
	{
		if (!JsonUtils::validate(mod.config, "vcmi:mod", mod.identifier))
			mod.validation = CModInfo::FAILED;
	}
	if (!preloadModData(mod.identifier, mod.config, validate))
		mod.validation = CModInfo::FAILED;
}

void CContentHandler::load(CModInfo & mod)
{
	bool validate = (mod.validation != CModInfo::PASSED);

	if (!loadMod(mod.identifier, validate))
		mod.validation = CModInfo::FAILED;

	if (validate)
	{
		if (mod.validation != CModInfo::FAILED)
			logGlobal->infoStream()  << "\t\t[DONE] " << mod.name;
		else
			logGlobal->errorStream() << "\t\t[FAIL] " << mod.name;
	}
	else
		logGlobal->infoStream()  << "\t\t[SKIP] " << mod.name;
}

static JsonNode loadModSettings(std::string path)
{
	if (CResourceHandler::get("local")->existsResource(ResourceID(path)))
	{
		return JsonNode(ResourceID(path, EResType::TEXT));
	}
	// Probably new install. Create initial configuration
	CResourceHandler::get("local")->createResource(path);
	return JsonNode();
}

JsonNode addMeta(JsonNode config, std::string meta)
{
	config.setMeta(meta);
	return config;
}

CModInfo::CModInfo(std::string identifier,const JsonNode & local, const JsonNode & config):
	identifier(identifier),
	name(config["name"].String()),
	description(config["description"].String()),
	dependencies(config["depends"].convertTo<std::set<std::string> >()),
	conflicts(config["conflicts"].convertTo<std::set<std::string> >()),
	validation(PENDING),
	config(addMeta(config, identifier))
{
	loadLocalData(local);
}

JsonNode CModInfo::saveLocalData() const
{
	std::ostringstream stream;
	stream << std::noshowbase << std::hex << std::setw(8) << std::setfill('0') << checksum;

	JsonNode conf;
	conf["active"].Bool() = enabled;
	conf["validated"].Bool() = validation != FAILED;
	conf["checksum"].String() = stream.str();
	return conf;
}

std::string CModInfo::getModDir(std::string name)
{
	return "MODS/" + boost::algorithm::replace_all_copy(name, ".", "/MODS/");
}

std::string CModInfo::getModFile(std::string name)
{
	return getModDir(name) + "/mod.json";
}

void CModInfo::updateChecksum(ui32 newChecksum)
{
	// comment-out next line to force validation of all mods ignoring checksum
	if (newChecksum != checksum)
	{
		checksum = newChecksum;
		validation = PENDING;
	}
}

void CModInfo::loadLocalData(const JsonNode & data)
{
	bool validated = false;
	enabled = true;
	checksum = 0;
	if (data.getType() == JsonNode::DATA_BOOL)
	{
		enabled = data.Bool();
	}
	if (data.getType() == JsonNode::DATA_STRUCT)
	{
		enabled   = data["active"].Bool();
		validated = data["validated"].Bool();
		checksum  = strtol(data["checksum"].String().c_str(), nullptr, 16);
	}

	if (enabled)
		validation = validated ? PASSED : PENDING;
	else
		validation = validated ? PASSED : FAILED;
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
	std::string paths;
	for(auto& p : CResourceHandler::get()->getResourceNames(ResourceID("config/" + name)))
	{
		paths += p.string() + ", ";
	}
	paths = paths.substr(0, paths.size() - 2);
	logGlobal->debugStream() << "Loading hardcoded features settings from [" << paths << "], result:";
	settings.data = JsonUtils::assembleFromFiles("config/" + name);
	const JsonNode & hardcodedFeatures = settings.data["hardcodedFeatures"];
	settings.MAX_HEROES_AVAILABLE_PER_PLAYER = hardcodedFeatures["MAX_HEROES_AVAILABLE_PER_PLAYER"].Float();
	logGlobal->debugStream() << "\tMAX_HEROES_AVAILABLE_PER_PLAYER\t" << settings.MAX_HEROES_AVAILABLE_PER_PLAYER;
	settings.MAX_HEROES_ON_MAP_PER_PLAYER = hardcodedFeatures["MAX_HEROES_ON_MAP_PER_PLAYER"].Float();
	logGlobal->debugStream() << "\tMAX_HEROES_ON_MAP_PER_PLAYER\t" << settings.MAX_HEROES_ON_MAP_PER_PLAYER;
	settings.CREEP_SIZE = hardcodedFeatures["CREEP_SIZE"].Float();
	logGlobal->debugStream() << "\tCREEP_SIZE\t" << settings.CREEP_SIZE;
	settings.WEEKLY_GROWTH = hardcodedFeatures["WEEKLY_GROWTH_PERCENT"].Float();
	logGlobal->debugStream() << "\tWEEKLY_GROWTH\t" << settings.WEEKLY_GROWTH;
	settings.NEUTRAL_STACK_EXP = hardcodedFeatures["NEUTRAL_STACK_EXP_DAILY"].Float();
	logGlobal->debugStream() << "\tNEUTRAL_STACK_EXP\t" << settings.NEUTRAL_STACK_EXP;
	settings.MAX_BUILDING_PER_TURN = hardcodedFeatures["MAX_BUILDING_PER_TURN"].Float();
	logGlobal->debugStream() << "\tMAX_BUILDING_PER_TURN\t" << settings.MAX_BUILDING_PER_TURN;
	settings.DWELLINGS_ACCUMULATE_CREATURES = hardcodedFeatures["DWELLINGS_ACCUMULATE_CREATURES"].Bool();
	logGlobal->debugStream() << "\tDWELLINGS_ACCUMULATE_CREATURES\t" << settings.DWELLINGS_ACCUMULATE_CREATURES;
	settings.ALL_CREATURES_GET_DOUBLE_MONTHS = hardcodedFeatures["ALL_CREATURES_GET_DOUBLE_MONTHS"].Bool();
	logGlobal->debugStream() << "\tALL_CREATURES_GET_DOUBLE_MONTHS\t" << settings.ALL_CREATURES_GET_DOUBLE_MONTHS;
	settings.WINNING_HERO_WITH_NO_TROOPS_RETREATS = hardcodedFeatures["WINNING_HERO_WITH_NO_TROOPS_RETREATS"].Bool();
	logGlobal->debugStream() << "\tWINNING_HERO_WITH_NO_TROOPS_RETREATS\t" << settings.WINNING_HERO_WITH_NO_TROOPS_RETREATS;
	const JsonNode & gameModules = settings.data["modules"];
	modules.STACK_EXP = gameModules["STACK_EXPERIENCE"].Bool();
  logGlobal->debugStream() << "\tSTACK_EXP\t" << modules.STACK_EXP;
	modules.STACK_ARTIFACT = gameModules["STACK_ARTIFACTS"].Bool();
	logGlobal->debugStream() << "\tSTACK_ARTIFACT\t" << modules.STACK_ARTIFACT;
	modules.COMMANDERS = gameModules["COMMANDERS"].Bool();
	logGlobal->debugStream() << "\tCOMMANDERS\t" << modules.COMMANDERS;
	modules.MITHRIL = gameModules["MITHRIL"].Bool();
	logGlobal->debugStream() << "\tMITHRIL\t" << modules.MITHRIL;
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
	for(const TModID & dependency : mod.dependencies)
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
	for(const TModID & id : input)
	{
		const CModInfo & mod = allMods.at(id);

		for(const TModID & dep : mod.dependencies)
		{
			if (!vstd::contains(input, dep))
			{
				logGlobal->errorStream() << "Error: Mod " << mod.name << " requires missing " << dep << "!";
				return false;
			}
		}

		for(const TModID & conflicting : mod.conflicts)
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
		for(const TModID & dependency : mod.dependencies)
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

std::vector<std::string> CModHandler::getModList(std::string path)
{
	std::string modDir = boost::to_upper_copy(path + "MODS/");
	size_t depth = boost::range::count(modDir, '/');

	auto list = CResourceHandler::get("initial")->getFilteredFiles([&](const ResourceID & id) ->  bool
	{
		if (id.getType() != EResType::DIRECTORY)
			return false;
		if (!boost::algorithm::starts_with(id.getName(), modDir))
			return false;
		if (boost::range::count(id.getName(), '/') != depth )
			return false;
		return true;
	});

	//storage for found mods
	std::vector<std::string> foundMods;
	for (auto & entry : list)
	{
		std::string name = entry.getName();
		name.erase(0, modDir.size()); //Remove path prefix

		// check if wog is actually present. Hack-ish but better than crash
		// TODO: remove soon (hopefully - before 0.96)
		if (name == "WOG")
		{
			if (!CResourceHandler::get("initial")->existsResource(ResourceID("DATA/ZVS", EResType::DIRECTORY)) &&
				!CResourceHandler::get("initial")->existsResource(ResourceID("MODS/WOG/DATA/ZVS", EResType::DIRECTORY)))
			{
				continue;
			}
		}

		if (!name.empty())
			foundMods.push_back(name);
	}
	return foundMods;
}

void CModHandler::loadMods(std::string path, std::string parent, const JsonNode & modSettings, bool enableMods)
{
	for (std::string modName : getModList(path))
	{
		boost::to_lower(modName);
		std::string modFullName = parent.empty() ? modName : parent + '.' + modName;

		if (CResourceHandler::get("initial")->existsResource(ResourceID(CModInfo::getModFile(modFullName))))
		{
			CModInfo mod(modFullName, modSettings[modName], JsonNode(ResourceID(CModInfo::getModFile(modFullName))));
			if (!parent.empty()) // this is submod, add parent to dependecies
				mod.dependencies.insert(parent);

			allMods[modFullName] = mod;
			if (mod.enabled && enableMods)
				activeMods.push_back(modFullName);

			loadMods(CModInfo::getModDir(modFullName) + '/', modFullName, modSettings[modName]["mods"], enableMods && mod.enabled);
		}
	}
}

void CModHandler::loadMods()
{
	const JsonNode modConfig = loadModSettings("config/modSettings.json");

	loadMods("", "", modConfig["activeMods"], true);

	coreMod = CModInfo("core", modConfig["core"], JsonNode(ResourceID("config/gameConfig.json")));
	coreMod.name = "Original game files";
}

std::vector<std::string> CModHandler::getAllMods()
{
	std::vector<std::string> modlist;
	for (auto & entry : allMods)
		modlist.push_back(entry.first);
	return modlist;
}

std::vector<std::string> CModHandler::getActiveMods()
{
	return activeMods;
}

static JsonNode genDefaultFS()
{
	// default FS config for mods: directory "Content" that acts as H3 root directory
	JsonNode defaultFS;
	defaultFS[""].Vector().resize(2);
	defaultFS[""].Vector()[0]["type"].String() = "zip";
	defaultFS[""].Vector()[0]["path"].String() = "/Content.zip";
	defaultFS[""].Vector()[1]["type"].String() = "dir";
	defaultFS[""].Vector()[1]["path"].String() = "/Content";
	return defaultFS;
}

static ISimpleResourceLoader * genModFilesystem(const std::string & modName, const JsonNode & conf)
{
	static const JsonNode defaultFS = genDefaultFS();

	if (!conf["filesystem"].isNull())
		return CResourceHandler::createFileSystem(CModInfo::getModDir(modName), conf["filesystem"]);
	else
		return CResourceHandler::createFileSystem(CModInfo::getModDir(modName), defaultFS);
}

static ui32 calculateModChecksum(const std::string modName, ISimpleResourceLoader * filesystem)
{
	boost::crc_32_type modChecksum;
	// first - add current VCMI version into checksum to force re-validation on VCMI updates
	modChecksum.process_bytes(reinterpret_cast<const void*>(GameConstants::VCMI_VERSION.data()), GameConstants::VCMI_VERSION.size());

	// second - add mod.json into checksum because filesystem does not contains this file
	// FIXME: remove workaround for core mod
	if (modName != "core")
	{
		ResourceID modConfFile(CModInfo::getModFile(modName), EResType::TEXT);
		ui32 configChecksum = CResourceHandler::get("initial")->load(modConfFile)->calculateCRC32();
		modChecksum.process_bytes(reinterpret_cast<const void *>(&configChecksum), sizeof(configChecksum));
	}
	// third - add all detected text files from this mod into checksum
	auto files = filesystem->getFilteredFiles([](const ResourceID & resID)
	{
		return resID.getType() == EResType::TEXT &&
			   ( boost::starts_with(resID.getName(), "DATA") ||
				 boost::starts_with(resID.getName(), "CONFIG"));
	});

	for (const ResourceID & file : files)
	{
		ui32 fileChecksum = filesystem->load(file)->calculateCRC32();
		modChecksum.process_bytes(reinterpret_cast<const void *>(&fileChecksum), sizeof(fileChecksum));
	}
	return modChecksum.checksum();
}

void CModHandler::loadModFilesystems()
{
	activeMods = resolveDependencies(activeMods);

	coreMod.updateChecksum(calculateModChecksum("core", CResourceHandler::get("core")));

	for(std::string & modName : activeMods)
	{
		CModInfo & mod = allMods[modName];
		CResourceHandler::addFilesystem("data", modName, genModFilesystem(modName, mod.config));
	}
}

CModInfo & CModHandler::getModData(TModID modId)
{
	CModInfo & mod = allMods.at(modId);
	assert(vstd::contains(activeMods, modId)); // not really necessary but won't hurt
	return mod;
}

void CModHandler::initializeConfig()
{
	loadConfigFromFile("defaultMods.json");
}

void CModHandler::load()
{
	CStopWatch totalTime, timer;

	CContentHandler content;
	logGlobal->infoStream() << "\tInitializing content handler: " << timer.getDiff() << " ms";

	for(const TModID & modName : activeMods)
	{
		logGlobal->traceStream() << "Generating checksum for " << modName;
		allMods[modName].updateChecksum(calculateModChecksum(modName, CResourceHandler::get(modName)));
	}

	// first - load virtual "core" mod that contains all data
	// TODO? move all data into real mods? RoE, AB, SoD, WoG
	content.preloadData(coreMod);
	for(const TModID & modName : activeMods)
		content.preloadData(allMods[modName]);
	logGlobal->infoStream() << "\tParsing mod data: " << timer.getDiff() << " ms";

	content.load(coreMod);
	for(const TModID & modName : activeMods)
		content.load(allMods[modName]);

	logGlobal->infoStream() << "\tLoading mod data: " << timer.getDiff() << "ms";

	VLC->creh->loadCrExpBon();
	VLC->creh->buildBonusTreeForTiers(); //do that after all new creatures are loaded

	identifiers.finalize();
	logGlobal->infoStream() << "\tResolving identifiers: " << timer.getDiff() << " ms";

	content.afterLoadFinalization();
	logGlobal->infoStream() << "\tHandlers post-load finalization: " << timer.getDiff() << " ms";
	logGlobal->infoStream() << "\tAll game content loaded in " << totalTime.getDiff() << " ms";
}

void CModHandler::afterLoad()
{
	JsonNode modSettings;
	for (auto & modEntry : allMods)
	{
		std::string pointer = "/" + boost::algorithm::replace_all_copy(modEntry.first, ".", "/mods/");

		modSettings["activeMods"].resolvePointer(pointer) = modEntry.second.saveLocalData();
	}
	modSettings["core"] = coreMod.saveLocalData();

	FileStream file(*CResourceHandler::get()->getResourceName(ResourceID("config/modSettings.json")), std::ofstream::out | std::ofstream::trunc);
	file << modSettings;
}
