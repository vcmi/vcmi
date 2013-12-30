#include "StdInc.h"
#include "CModHandler.h"
#include "CDefObjInfoHandler.h"
#include "JsonNode.h"
#include "filesystem/Filesystem.h"
#include "filesystem/AdapterLoaders.h"
#include "filesystem/CFilesystemLoader.h"

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

	scheduledRequests.push_back(callback);
}

void CIdentifierStorage::requestIdentifier(std::string scope, std::string type, std::string name, const std::function<void(si32)> & callback)
{
	auto pair = splitString(name, ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(scope, pair.first, type, pair.second, callback, false));
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

boost::optional<si32> CIdentifierStorage::getIdentifier(std::string type, const JsonNode & name, bool silent)
{
	auto pair = splitString(name.String(), ':'); // remoteScope:name
	auto idList = getPossibleIdentifiers(ObjectCallback(name.meta, pair.first, type, pair.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logGlobal->errorStream() << "Failed to resolve identifier " << name.String() << " from mod " << type;

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
		logGlobal->errorStream() << "Failed to resolve identifier " << name.String() << " from mod " << pair2.first;

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
		return true;

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
	bool errorsFound = false;

	for(const ObjectCallback & request : scheduledRequests)
	{
		errorsFound |= !resolveIdentifier(request);
	}

	if (errorsFound)
	{
		for(auto object : registeredObjects)
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
				if (validate)
					result &= JsonUtils::validate(originalData[index], "vcmi:" + objectName, name);
				handler->loadObject(modName, name, originalData[index], index);

				originalData[index].clear(); // do not use same data twice (same ID)

				continue;
			}
		}
		// normal new object or one with index bigger that data size
		if (validate)
			result &= JsonUtils::validate(data, "vcmi:" + objectName, name);
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
	handlers.insert(std::make_pair("heroes", ContentTypeHandler(VLC->heroh, "hero")));

	//TODO: spells, bonuses, something else?
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

static JsonNode updateModSettingsFormat(JsonNode config)
{
	for (auto & entry : config["activeMods"].Struct())
	{
		if (entry.second.getType() == JsonNode::DATA_BOOL)
		{
			entry.second["active"].Bool() = entry.second.Bool();
		}
	}
	return config;
}

static JsonNode loadModSettings(std::string path)
{
	if (CResourceHandler::get()->existsResource(ResourceID(path)))
	{
		// mod compatibility: check if modSettings has old, 0.94 format
		return updateModSettingsFormat(JsonNode(ResourceID(path, EResType::TEXT)));
	}
	// Probably new install. Create initial configuration
	CResourceHandler::get()->createResource(path);
	return JsonNode();
}

JsonNode addMeta(JsonNode config, std::string meta)
{
	config.setMeta(meta);
	return std::move(config);
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

JsonNode CModInfo::saveLocalData()
{
	std::ostringstream stream;
	stream << std::noshowbase << std::hex << std::setw(8) << std::setfill('0') << checksum;

	JsonNode conf;
	conf["active"].Bool() = enabled;
	conf["validated"].Bool() = validation != FAILED;
	conf["checksum"].String() = stream.str();
	return conf;
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
	if (data.isNull())
	{
		enabled = true;
		checksum = 0;
	}
	else
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

void CModHandler::initializeMods(std::vector<std::string> availableMods)
{
	const JsonNode modConfig = loadModSettings("config/modSettings.json");
	const JsonNode & modList = modConfig["activeMods"];

	std::vector <TModID> detectedMods;

	for(std::string name : availableMods)
	{
		boost::to_lower(name);
		std::string modFileName = "mods/" + name + "/mod.json";

		if (CResourceHandler::get()->existsResource(ResourceID(modFileName)))
		{
			CModInfo mod(name, modList[name], JsonNode(ResourceID(modFileName)));

			allMods[name] = mod;
			if (mod.enabled)
				detectedMods.push_back(name);
		}
		else
            logGlobal->warnStream() << "\t\t Directory " << name << " does not contains VCMI mod";
	}
	coreMod = CModInfo("core", modConfig["core"], JsonNode(ResourceID("config/gameConfig.json")));
	coreMod.name = "Original game files";

	if (!checkDependencies(detectedMods))
	{
        logGlobal->errorStream() << "Critical error: failed to load mods! Exiting...";
		exit(1);
	}

	activeMods = resolveDependencies(detectedMods);
	loadModFilesystems();
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
		return CResourceHandler::createFileSystem("mods/" + modName, conf["filesystem"]);
	else
		return CResourceHandler::createFileSystem("mods/" + modName, defaultFS);
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
		ResourceID modConfFile("mods/" + modName + "/mod", EResType::TEXT);
		ui32 configChecksum = CResourceHandler::getInitial()->load(modConfFile)->calculateCRC32();
		modChecksum.process_bytes(reinterpret_cast<const void *>(&configChecksum), sizeof(configChecksum));
	}
	// third - add all detected text files from this mod into checksum
	auto files = filesystem->getFilteredFiles([](const ResourceID & resID)
	{
		return resID.getType() == EResType::TEXT &&
			   ( boost::starts_with(resID.getName(), "DATA") ||
				 boost::starts_with(resID.getName(), "CONFIG"));
	});

	// these two files may change between two runs of vcmi and must be handled separately
	files.erase(ResourceID("CONFIG/SETTINGS", EResType::TEXT));
	files.erase(ResourceID("CONFIG/MODSETTINGS", EResType::TEXT));

	for (const ResourceID & file : files)
	{
		ui32 fileChecksum = filesystem->load(file)->calculateCRC32();
		modChecksum.process_bytes(reinterpret_cast<const void *>(&fileChecksum), sizeof(fileChecksum));
	}
	return modChecksum.checksum();
}

void CModHandler::loadModFilesystems()
{
	coreMod.updateChecksum(calculateModChecksum("core", CResourceHandler::get("core")));

	for(std::string & modName : activeMods)
	{
		CModInfo & mod = allMods[modName];
		CResourceHandler::addFilesystem(modName, genModFilesystem(modName, mod.config));
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
		modSettings["activeMods"][modEntry.first] = modEntry.second.saveLocalData();
	modSettings["core"] = coreMod.saveLocalData();

	std::ofstream file(*CResourceHandler::get()->getResourceName(ResourceID("config/modSettings.json")), std::ofstream::trunc);
	file << modSettings;
	reload();
}

void CModHandler::reload()
{
	{
		//recreate adventure map defs
		assert(!VLC->dobjinfo->gobjs[Obj::MONSTER].empty()); //make sure that at least some def info was found

		const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::MONSTER].begin()->second;

		for(auto & crea : VLC->creh->creatures)
		{
			if (!vstd::contains(VLC->dobjinfo->gobjs[Obj::MONSTER], crea->idNumber)) // no obj info for this type
			{
				auto  info = new CGDefInfo(*baseInfo);
				info->subid = crea->idNumber;
				info->name = crea->advMapDef;

				VLC->dobjinfo->gobjs[Obj::MONSTER][crea->idNumber] = info;
			}
		}
	}
	{
		assert(!VLC->dobjinfo->gobjs[Obj::ARTIFACT].empty());

		const CGDefInfo * baseInfo = VLC->dobjinfo->gobjs[Obj::ARTIFACT].begin()->second;

		for(auto & art : VLC->arth->artifacts)
		{
			if (!vstd::contains(VLC->dobjinfo->gobjs[Obj::ARTIFACT], art->id)) // no obj info for this type
			{
				auto  info = new CGDefInfo(*baseInfo);
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

		for(auto & faction : VLC->townh->factions)
		{
			TFaction index = faction->index;
			CTown * town = faction->town;
			if (town)
			{
				auto & cientInfo = town->clientInfo;

				if (!vstd::contains(VLC->dobjinfo->gobjs[Obj::TOWN], index)) // no obj info for this type
				{
					auto  info = new CGDefInfo(*baseInfo);
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

					for (auto cre : town->creatures[i]) //both unupgraded and upgraded get same dwelling
					{
						auto  info = new CGDefInfo(*baseInfo);
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
