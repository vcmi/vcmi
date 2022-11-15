/*
 * CModHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CModHandler.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "filesystem/FileStream.h"
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
#include "CSkillHandler.h"
#include "ScriptHandler.h"
#include "BattleFieldHandler.h"
#include "ObstacleHandler.h"

#include <vstd/StringUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

CIdentifierStorage::CIdentifierStorage():
	state(LOADING)
{
}

CIdentifierStorage::~CIdentifierStorage()
{
}

void CIdentifierStorage::checkIdentifier(std::string & ID)
{
	if (boost::algorithm::ends_with(ID, "."))
		logMod->warn("BIG WARNING: identifier %s seems to be broken!", ID);
	else
	{
		size_t pos = 0;
		do
		{
			if (std::tolower(ID[pos]) != ID[pos] ) //Not in camelCase
			{
				logMod->warn("Warning: identifier %s is not in camelCase!", ID);
				ID[pos] = std::tolower(ID[pos]);// Try to fix the ID
			}
			pos = ID.find('.', pos);
		}
		while(pos++ != std::string::npos);
	}
}

CIdentifierStorage::ObjectCallback::ObjectCallback(
		std::string localScope, std::string remoteScope, std::string type,
		std::string name, const std::function<void(si32)> & callback,
		bool optional):
	localScope(localScope),
	remoteScope(remoteScope),
	type(type),
	name(name),
	callback(callback),
	optional(optional)
{}

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
	auto pair = vstd::splitStringToPair(name, ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(scope, pair.first, type, pair.second, callback, false));
}

void CIdentifierStorage::requestIdentifier(std::string scope, std::string fullName, const std::function<void(si32)>& callback)
{
	auto scopeAndFullName = vstd::splitStringToPair(fullName, ':');
	auto typeAndName = vstd::splitStringToPair(scopeAndFullName.second, '.');

	requestIdentifier(ObjectCallback(scope, scopeAndFullName.first, typeAndName.first, typeAndName.second, callback, false));
}

void CIdentifierStorage::requestIdentifier(std::string type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	auto pair = vstd::splitStringToPair(name.String(), ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(name.meta, pair.first, type, pair.second, callback, false));
}

void CIdentifierStorage::requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback)
{
	auto pair  = vstd::splitStringToPair(name.String(), ':'); // remoteScope:<type.name>
	auto pair2 = vstd::splitStringToPair(pair.second,   '.'); // type.name

	requestIdentifier(ObjectCallback(name.meta, pair.first, pair2.first, pair2.second, callback, false));
}

void CIdentifierStorage::tryRequestIdentifier(std::string scope, std::string type, std::string name, const std::function<void(si32)> & callback)
{
	auto pair = vstd::splitStringToPair(name, ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(scope, pair.first, type, pair.second, callback, true));
}

void CIdentifierStorage::tryRequestIdentifier(std::string type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	auto pair = vstd::splitStringToPair(name.String(), ':'); // remoteScope:name

	requestIdentifier(ObjectCallback(name.meta, pair.first, type, pair.second, callback, true));
}

boost::optional<si32> CIdentifierStorage::getIdentifier(std::string scope, std::string type, std::string name, bool silent)
{
	auto pair = vstd::splitStringToPair(name, ':'); // remoteScope:name
	auto idList = getPossibleIdentifiers(ObjectCallback(scope, pair.first, type, pair.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s of type %s from mod %s", name , type ,scope);

	return boost::optional<si32>();
}

boost::optional<si32> CIdentifierStorage::getIdentifier(std::string type, const JsonNode & name, bool silent)
{
	auto pair = vstd::splitStringToPair(name.String(), ':'); // remoteScope:name
	auto idList = getPossibleIdentifiers(ObjectCallback(name.meta, pair.first, type, pair.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s of type %s from mod %s", name.String(), type, name.meta);

	return boost::optional<si32>();
}

boost::optional<si32> CIdentifierStorage::getIdentifier(const JsonNode & name, bool silent)
{
	auto pair  = vstd::splitStringToPair(name.String(), ':'); // remoteScope:<type.name>
	auto pair2 = vstd::splitStringToPair(pair.second,   '.'); // type.name
	auto idList = getPossibleIdentifiers(ObjectCallback(name.meta, pair.first, pair2.first, pair2.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s of type %s from mod %s", name.String(), pair2.first, name.meta);

	return boost::optional<si32>();
}

boost::optional<si32> CIdentifierStorage::getIdentifier(std::string scope, std::string fullName, bool silent)
{
	auto pair  = vstd::splitStringToPair(fullName, ':'); // remoteScope:<type.name>
	auto pair2 = vstd::splitStringToPair(pair.second,   '.'); // type.name
	auto idList = getPossibleIdentifiers(ObjectCallback(scope, pair.first, pair2.first, pair2.second, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s of type %s from mod %s", fullName, pair2.first, scope);

	return boost::optional<si32>();
}

void CIdentifierStorage::registerObject(std::string scope, std::string type, std::string name, si32 identifier)
{
	ObjectData data;
	data.scope = scope;
	data.id = identifier;

	std::string fullID = type + '.' + name;
	checkIdentifier(fullID);

	std::pair<const std::string, ObjectData> mapping = std::make_pair(fullID, data);
	if(!vstd::containsMapping(registeredObjects, mapping))
	{
		logMod->trace("registered %s as %s:%s", fullID, scope, identifier);
		registeredObjects.insert(mapping);
	}
}

std::vector<CIdentifierStorage::ObjectData> CIdentifierStorage::getPossibleIdentifiers(const ObjectCallback & request)
{
	std::set<std::string> allowedScopes;
	bool isValidScope = true;

	if (request.remoteScope.empty())
	{
		// normally ID's from all required mods, own mod and virtual "core" mod are allowed
		if(request.localScope != "core" && !request.localScope.empty())
		{
			allowedScopes = VLC->modh->getModDependencies(request.localScope, isValidScope);

			if(!isValidScope)
				return std::vector<ObjectData>();
		}
		allowedScopes.insert(request.localScope);
		allowedScopes.insert("core");
	}
	else
	{
		//...unless destination mod was specified explicitly
		//note: getModDependencies does not work for "core" by design

		//for map format support core mod has access to any mod
		//TODO: better solution for access from map?
		if(request.localScope == "core" || request.localScope.empty())
		{
			allowedScopes.insert(request.remoteScope);
		}
		else
		{
			// allow only available to all core mod or dependencies
			auto myDeps = VLC->modh->getModDependencies(request.localScope, isValidScope);

			if(!isValidScope)
				return std::vector<ObjectData>();

			if(request.remoteScope == "core" || request.remoteScope == request.localScope || myDeps.count(request.remoteScope))
				allowedScopes.insert(request.remoteScope);
		}
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
		logMod->error("Unknown identifier!");
	else
		logMod->error("Ambiguous identifier request!");

	 logMod->error("Request for %s.%s from mod %s", request.type, request.name, request.localScope);

	for (auto id : identifiers)
	{
		logMod->error("\tID is available in mod %s", id.scope);
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
			logMod->trace("%s : %s -> %d", object.second.scope, object.first, object.second.id);
		}
		logMod->error("All known identifiers were dumped into log file");
	}
	assert(errorsFound == false);
	state = FINISHED;
}

ContentTypeHandler::ContentTypeHandler(IHandlerBase * handler, std::string objectName):
	handler(handler),
	objectName(objectName),
	originalData(handler->loadLegacyData((size_t)VLC->modh->settings.data["textData"][objectName].Float()))
{
	for(auto & node : originalData)
	{
		node.setMeta("core");
	}
}

bool ContentTypeHandler::preloadModData(std::string modName, std::vector<std::string> fileList, bool validate)
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
				logMod->warn("Redundant namespace definition for %s", objectName);

			logMod->trace("Patching object %s (%s) from %s", objectName, remoteName, modName);
			JsonNode & remoteConf = modData[remoteName].patches[objectName];

			JsonUtils::merge(remoteConf, entry.second);
		}
	}
	return result;
}

bool ContentTypeHandler::loadMod(std::string modName, bool validate)
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
			size_t index = static_cast<size_t>(data["index"].Float());

			if(originalData.size() > index)
			{
				logMod->trace("found original data in loadMod(%s) at index %d", name, index);
				JsonUtils::merge(originalData[index], data);
				std::swap(originalData[index], data);
				originalData[index].clear(); // do not use same data twice (same ID)
			}
			else
			{
				logMod->warn("no original data in loadMod(%s) at index %d", name, index);
			}
			performValidate(data, name);
			handler->loadObject(modName, name, data, index);
		}
		else
		{
			// normal new object
			logMod->trace("no index in loadMod(%s)", name);
			performValidate(data,name);
			handler->loadObject(modName, name, data);
		}
	}
	return result;
}


void ContentTypeHandler::loadCustom()
{
	handler->loadCustom();
}

void ContentTypeHandler::afterLoadFinalization()
{
	handler->afterLoadFinalization();
}

CContentHandler::CContentHandler()
{
}

void CContentHandler::init()
{
 	handlers.insert(std::make_pair("heroClasses", ContentTypeHandler(&VLC->heroh->classes, "heroClass")));
	handlers.insert(std::make_pair("artifacts", ContentTypeHandler(VLC->arth, "artifact")));
	handlers.insert(std::make_pair("creatures", ContentTypeHandler(VLC->creh, "creature")));
	handlers.insert(std::make_pair("factions", ContentTypeHandler(VLC->townh, "faction")));
	handlers.insert(std::make_pair("objects", ContentTypeHandler(VLC->objtypeh, "object")));
	handlers.insert(std::make_pair("heroes", ContentTypeHandler(VLC->heroh, "hero")));
	handlers.insert(std::make_pair("spells", ContentTypeHandler(VLC->spellh, "spell")));
	handlers.insert(std::make_pair("skills", ContentTypeHandler(VLC->skillh, "skill")));
	handlers.insert(std::make_pair("templates", ContentTypeHandler((IHandlerBase *)VLC->tplh, "template")));
#if SCRIPTING_ENABLED
	handlers.insert(std::make_pair("scripts", ContentTypeHandler(VLC->scriptHandler, "script")));
#endif
	handlers.insert(std::make_pair("battlefields", ContentTypeHandler(VLC->battlefieldsHandler, "battlefield")));
	handlers.insert(std::make_pair("obstacles", ContentTypeHandler(VLC->obstacleHandler, "obstacle")));
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

void CContentHandler::loadCustom()
{
	for(auto & handler : handlers)
	{
		handler.second.loadCustom();
	}
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
	logMod->info("\t\t[%08x]%s", mod.checksum, mod.name);

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
			logMod->info("\t\t[DONE] %s", mod.name);
		else
			logMod->error("\t\t[FAIL] %s", mod.name);
	}
	else
		logMod->info("\t\t[SKIP] %s", mod.name);
}

const ContentTypeHandler & CContentHandler::operator[](const std::string & name) const
{
	return handlers.at(name);
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

CModInfo::Version CModInfo::Version::GameVersion()
{
	return Version(VCMI_VERSION_MAJOR, VCMI_VERSION_MINOR, VCMI_VERSION_PATCH);
}

CModInfo::Version CModInfo::Version::fromString(std::string from)
{
	int major = 0, minor = 0, patch = 0;
	try
	{
		auto pointPos = from.find('.');
		major = std::stoi(from.substr(0, pointPos));
		if(pointPos != std::string::npos)
		{
			from = from.substr(pointPos + 1);
			pointPos = from.find('.');
			minor = std::stoi(from.substr(0, pointPos));
			if(pointPos != std::string::npos)
				patch = std::stoi(from.substr(pointPos + 1));
		}
	}
	catch(const std::invalid_argument & e)
	{
		return Version();
	}
	return Version(major, minor, patch);
}

std::string CModInfo::Version::toString() const
{
	return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(patch);
}

bool CModInfo::Version::compatible(const Version & other, bool checkMinor, bool checkPatch) const
{
	return  (major == other.major &&
			(!checkMinor || minor >= other.minor) &&
			(!checkPatch || minor > other.minor || (minor == other.minor && patch >= other.patch)));
}

bool CModInfo::Version::isNull() const
{
	return major == 0 && minor == 0 && patch == 0;
}

CModInfo::CModInfo():
	checksum(0),
	enabled(false),
	validation(PENDING)
{

}

CModInfo::CModInfo(std::string identifier,const JsonNode & local, const JsonNode & config):
	identifier(identifier),
	name(config["name"].String()),
	description(config["description"].String()),
	dependencies(config["depends"].convertTo<std::set<std::string> >()),
	conflicts(config["conflicts"].convertTo<std::set<std::string> >()),
	checksum(0),
	enabled(false),
	validation(PENDING),
	config(addMeta(config, identifier))
{
	version = Version::fromString(config["version"].String());
	if(!config["compatibility"].isNull())
	{
		vcmiCompatibleMin = Version::fromString(config["compatibility"]["min"].String());
		vcmiCompatibleMax = Version::fromString(config["compatibility"]["max"].String());
	}
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
	if (data.getType() == JsonNode::JsonType::DATA_BOOL)
	{
		enabled = data.Bool();
	}
	if (data.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		enabled   = data["active"].Bool();
		validated = data["validated"].Bool();
		checksum  = strtol(data["checksum"].String().c_str(), nullptr, 16);
	}
	
	//check compatibility
	bool wasEnabled = enabled;
	enabled = enabled && (vcmiCompatibleMin.isNull() || Version::GameVersion().compatible(vcmiCompatibleMin));
	enabled = enabled && (vcmiCompatibleMax.isNull() || vcmiCompatibleMax.compatible(Version::GameVersion()));

	if(wasEnabled && !enabled)
		logGlobal->warn("Mod %s is incompatible with current version of VCMI and cannot be enabled", name);

	if (enabled)
		validation = validated ? PASSED : PENDING;
	else
		validation = validated ? PASSED : FAILED;
}

CModHandler::CModHandler() : content(std::make_shared<CContentHandler>())
{
    modules.COMMANDERS = false;
    modules.STACK_ARTIFACT = false;
    modules.STACK_EXP = false;
    modules.MITHRIL = false;
	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
	{
		identifiers.registerObject("core", "resource", GameConstants::RESOURCE_NAMES[i], i);
	}

	for(int i=0; i<GameConstants::PRIMARY_SKILLS; ++i)
	{
		identifiers.registerObject("core", "primSkill", PrimarySkill::names[i], i);
		identifiers.registerObject("core", "primarySkill", PrimarySkill::names[i], i);
	}
}

CModHandler::~CModHandler()
{
}

void CModHandler::loadConfigFromFile (std::string name)
{
	std::string paths;
	for(auto& p : CResourceHandler::get()->getResourceNames(ResourceID("config/" + name)))
	{
		paths += p.string() + ", ";
	}
	paths = paths.substr(0, paths.size() - 2);
	logMod->debug("Loading hardcoded features settings from [%s], result:", paths);
	settings.data = JsonUtils::assembleFromFiles("config/" + name);
	const JsonNode & hardcodedFeatures = settings.data["hardcodedFeatures"];
	settings.MAX_HEROES_AVAILABLE_PER_PLAYER = static_cast<int>(hardcodedFeatures["MAX_HEROES_AVAILABLE_PER_PLAYER"].Integer());
	logMod->debug("\tMAX_HEROES_AVAILABLE_PER_PLAYER\t%d", settings.MAX_HEROES_AVAILABLE_PER_PLAYER);
	settings.MAX_HEROES_ON_MAP_PER_PLAYER = static_cast<int>(hardcodedFeatures["MAX_HEROES_ON_MAP_PER_PLAYER"].Integer());
	logMod->debug("\tMAX_HEROES_ON_MAP_PER_PLAYER\t%d", settings.MAX_HEROES_ON_MAP_PER_PLAYER);
	settings.CREEP_SIZE = static_cast<int>(hardcodedFeatures["CREEP_SIZE"].Integer());
	logMod->debug("\tCREEP_SIZE\t%d", settings.CREEP_SIZE);
	settings.WEEKLY_GROWTH = static_cast<int>(hardcodedFeatures["WEEKLY_GROWTH_PERCENT"].Integer());
	logMod->debug("\tWEEKLY_GROWTH\t%d", settings.WEEKLY_GROWTH);
	settings.NEUTRAL_STACK_EXP = static_cast<int>(hardcodedFeatures["NEUTRAL_STACK_EXP_DAILY"].Integer());
	logMod->debug("\tNEUTRAL_STACK_EXP\t%d", settings.NEUTRAL_STACK_EXP);
	settings.MAX_BUILDING_PER_TURN = static_cast<int>(hardcodedFeatures["MAX_BUILDING_PER_TURN"].Integer());
	logMod->debug("\tMAX_BUILDING_PER_TURN\t%d", settings.MAX_BUILDING_PER_TURN);
	settings.DWELLINGS_ACCUMULATE_CREATURES = hardcodedFeatures["DWELLINGS_ACCUMULATE_CREATURES"].Bool();
	logMod->debug("\tDWELLINGS_ACCUMULATE_CREATURES\t%d", static_cast<int>(settings.DWELLINGS_ACCUMULATE_CREATURES));
	settings.ALL_CREATURES_GET_DOUBLE_MONTHS = hardcodedFeatures["ALL_CREATURES_GET_DOUBLE_MONTHS"].Bool();
	logMod->debug("\tALL_CREATURES_GET_DOUBLE_MONTHS\t%d", static_cast<int>(settings.ALL_CREATURES_GET_DOUBLE_MONTHS));
	settings.WINNING_HERO_WITH_NO_TROOPS_RETREATS = hardcodedFeatures["WINNING_HERO_WITH_NO_TROOPS_RETREATS"].Bool();
	logMod->debug("\tWINNING_HERO_WITH_NO_TROOPS_RETREATS\t%d", static_cast<int>(settings.WINNING_HERO_WITH_NO_TROOPS_RETREATS));
	settings.BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE = hardcodedFeatures["BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE"].Bool();
	logMod->debug("\tBLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE\t%d", static_cast<int>(settings.BLACK_MARKET_MONTHLY_ARTIFACTS_CHANGE));
	settings.NO_RANDOM_SPECIAL_WEEKS_AND_MONTHS = hardcodedFeatures["NO_RANDOM_SPECIAL_WEEKS_AND_MONTHS"].Bool();
	logMod->debug("\tNO_RANDOM_SPECIAL_WEEKS_AND_MONTHS\t%d", static_cast<int>(settings.NO_RANDOM_SPECIAL_WEEKS_AND_MONTHS));

	const JsonNode & gameModules = settings.data["modules"];
	modules.STACK_EXP = gameModules["STACK_EXPERIENCE"].Bool();
	logMod->debug("\tSTACK_EXP\t%d", static_cast<int>(modules.STACK_EXP));
	modules.STACK_ARTIFACT = gameModules["STACK_ARTIFACTS"].Bool();
	logMod->debug("\tSTACK_ARTIFACT\t%d", static_cast<int>(modules.STACK_ARTIFACT));
	modules.COMMANDERS = gameModules["COMMANDERS"].Bool();
	logMod->debug("\tCOMMANDERS\t%d", static_cast<int>(modules.COMMANDERS));
	modules.MITHRIL = gameModules["MITHRIL"].Bool();
	logMod->debug("\tMITHRIL\t%d", static_cast<int>(modules.MITHRIL));
}

// currentList is passed by value to get current list of depending mods
bool CModHandler::hasCircularDependency(TModID modID, std::set <TModID> currentList) const
{
	const CModInfo & mod = allMods.at(modID);

	// Mod already present? We found a loop
	if (vstd::contains(currentList, modID))
	{
		logMod->error("Error: Circular dependency detected! Printing dependency list:");
		logMod->error("\t%s -> ", mod.name);
		return true;
	}

	currentList.insert(modID);

	// recursively check every dependency of this mod
	for(const TModID & dependency : mod.dependencies)
	{
		if (hasCircularDependency(dependency, currentList))
		{
			logMod->error("\t%s ->\n", mod.name); // conflict detected, print dependency list
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
			if(!vstd::contains(input, dep))
			{
				logMod->error("Error: Mod %s requires missing %s!", mod.name, dep);
				return false;
			}
		}

		for(const TModID & conflicting : mod.conflicts)
		{
			if(vstd::contains(input, conflicting))
			{
				logMod->error("Error: Mod %s conflicts with %s!", mod.name, allMods.at(conflicting).name);
				return false;
			}
		}

		if(hasCircularDependency(id))
			return false;
	}
	return true;
}

// Returned vector affects the resource loaders call order (see CFilesystemList::load).
// The loaders call order matters when dependent mod overrides resources in its dependencies.
std::vector <TModID> CModHandler::validateAndSortDependencies(std::vector <TModID> modsToResolve) const
{
	// Topological sort algorithm.
	// TODO: Investigate possible ways to improve performance.
	boost::range::sort(modsToResolve); // Sort mods per name
	std::vector <TModID> sortedValidMods; // Vector keeps order of elements (LIFO) 
	sortedValidMods.reserve(modsToResolve.size()); // push_back calls won't cause memory reallocation
	std::set <TModID> resolvedModIDs; // Use a set for validation for performance reason, but set does not keep order of elements

	// Mod is resolved if it has not dependencies or all its dependencies are already resolved
	auto isResolved = [&](const CModInfo & mod) -> CModInfo::EValidationStatus
	{
		if(mod.dependencies.size() > resolvedModIDs.size())
			return CModInfo::PENDING;

		for(const TModID & dependency : mod.dependencies)
		{
			if(!vstd::contains(resolvedModIDs, dependency))
				return CModInfo::PENDING;
		}
		return CModInfo::PASSED;
	};

	while(true)
	{
		std::set <TModID> resolvedOnCurrentTreeLevel;
		for(auto it = modsToResolve.begin(); it != modsToResolve.end();) // One iteration - one level of mods tree
		{
			if(isResolved(allMods.at(*it)) == CModInfo::PASSED)
			{
				resolvedOnCurrentTreeLevel.insert(*it); // Not to the resolvedModIDs, so current node childs will be resolved on the next iteration
				sortedValidMods.push_back(*it);
				it = modsToResolve.erase(it);
				continue;
			}
			it++;
		}
		if(resolvedOnCurrentTreeLevel.size())
		{
			resolvedModIDs.insert(resolvedOnCurrentTreeLevel.begin(), resolvedOnCurrentTreeLevel.end());
		            continue;
		}
		// If there're no valid mods on the current mods tree level, no more mod can be resolved, should be end.
		break;
	}

	// Left mods have unresolved dependencies, output all to log.
	for(const auto & brokenModID : modsToResolve)
	{
		const CModInfo & brokenMod = allMods.at(brokenModID);
		for(const TModID & dependency : brokenMod.dependencies)
		{
			if(!vstd::contains(resolvedModIDs, dependency))
				logMod->error("Mod '%s' will not work: it depends on mod '%s', which is not installed.", brokenMod.name, dependency);
		}
	}
	return sortedValidMods;
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

		if (!name.empty())
			foundMods.push_back(name);
	}
	return foundMods;
}

void CModHandler::loadMods(std::string path, std::string parent, const JsonNode & modSettings, bool enableMods)
{
	for(std::string modName : getModList(path))
		loadOneMod(modName, parent, modSettings, enableMods);
}

void CModHandler::loadOneMod(std::string modName, std::string parent, const JsonNode & modSettings, bool enableMods)
{
	boost::to_lower(modName);
	std::string modFullName = parent.empty() ? modName : parent + '.' + modName;

	if(CResourceHandler::get("initial")->existsResource(ResourceID(CModInfo::getModFile(modFullName))))
	{
		CModInfo mod(modFullName, modSettings[modName], JsonNode(ResourceID(CModInfo::getModFile(modFullName))));
		if (!parent.empty()) // this is submod, add parent to dependencies
			mod.dependencies.insert(parent);

		allMods[modFullName] = mod;
		if (mod.enabled && enableMods)
			activeMods.push_back(modFullName);

		loadMods(CModInfo::getModDir(modFullName) + '/', modFullName, modSettings[modName]["mods"], enableMods && mod.enabled);
	}
}

void CModHandler::loadMods(bool onlyEssential)
{
	JsonNode modConfig;

	if(onlyEssential)
	{
		loadOneMod("vcmi", "", modConfig, true);//only vcmi and submods
	}
	else
	{
		modConfig = loadModSettings("config/modSettings.json");
		loadMods("", "", modConfig["activeMods"], true);
	}

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
	activeMods = validateAndSortDependencies(activeMods);

	coreMod.updateChecksum(calculateModChecksum("core", CResourceHandler::get("core")));

	for(std::string & modName : activeMods)
	{
		CModInfo & mod = allMods[modName];
		CResourceHandler::addFilesystem("data", modName, genModFilesystem(modName, mod.config));
	}
}

std::set<TModID> CModHandler::getModDependencies(TModID modId, bool & isModFound)
{
	auto it = allMods.find(modId);
	isModFound = (it != allMods.end());

	if(isModFound)
		return it->second.dependencies;

	logMod->error("Mod not found: '%s'", modId);
	return std::set<TModID>();
}

void CModHandler::initializeConfig()
{
	loadConfigFromFile("defaultMods.json");
}

void CModHandler::load()
{
	CStopWatch totalTime, timer;

	logMod->info("\tInitializing content handler: %d ms", timer.getDiff());

	content->init();

	for(const TModID & modName : activeMods)
	{
		logMod->trace("Generating checksum for %s", modName);
		allMods[modName].updateChecksum(calculateModChecksum(modName, CResourceHandler::get(modName)));
	}

	// first - load virtual "core" mod that contains all data
	// TODO? move all data into real mods? RoE, AB, SoD, WoG
	content->preloadData(coreMod);
	for(const TModID & modName : activeMods)
		content->preloadData(allMods[modName]);
	logMod->info("\tParsing mod data: %d ms", timer.getDiff());

	content->load(coreMod);
	for(const TModID & modName : activeMods)
		content->load(allMods[modName]);

#if SCRIPTING_ENABLED
	VLC->scriptHandler->performRegistration(VLC);//todo: this should be done before any other handlers load
#endif

	content->loadCustom();

	logMod->info("\tLoading mod data: %d ms", timer.getDiff());

	VLC->creh->loadCrExpBon();
	VLC->creh->buildBonusTreeForTiers(); //do that after all new creatures are loaded

	identifiers.finalize();
	logMod->info("\tResolving identifiers: %d ms", timer.getDiff());

	content->afterLoadFinalization();
	logMod->info("\tHandlers post-load finalization: %d ms ", timer.getDiff());
	logMod->info("\tAll game content loaded in %d ms", totalTime.getDiff());
}

void CModHandler::afterLoad(bool onlyEssential)
{
	JsonNode modSettings;
	for (auto & modEntry : allMods)
	{
		std::string pointer = "/" + boost::algorithm::replace_all_copy(modEntry.first, ".", "/mods/");

		modSettings["activeMods"].resolvePointer(pointer) = modEntry.second.saveLocalData();
	}
	modSettings["core"] = coreMod.saveLocalData();

	if(!onlyEssential)
	{
		FileStream file(*CResourceHandler::get()->getResourceName(ResourceID("config/modSettings.json")), std::ofstream::out | std::ofstream::trunc);
		file << modSettings.toJson();
	}

}

std::string CModHandler::normalizeIdentifier(const std::string & scope, const std::string & remoteScope, const std::string & identifier)
{
	auto p = vstd::splitStringToPair(identifier, ':');

	if(p.first.empty())
		p.first = scope;

	if(p.first == remoteScope)
		p.first.clear();

	return p.first.empty() ? p.second : p.first + ":" + p.second;
}

void CModHandler::parseIdentifier(const std::string & fullIdentifier, std::string & scope, std::string & type, std::string & identifier)
{
	auto p = vstd::splitStringToPair(fullIdentifier, ':');

	scope = p.first;

	auto p2 = vstd::splitStringToPair(p.second, '.');

	if(p2.first != "")
	{
		type = p2.first;
		identifier = p2.second;
	}
	else
	{
		type = p.second;
		identifier.clear();
	}
}

std::string CModHandler::makeFullIdentifier(const std::string & scope, const std::string & type, const std::string & identifier)
{
	if(type.empty())
		logGlobal->error("Full identifier (%s %s) requires type name", scope, identifier);

	std::string actualScope = scope;
	std::string actualName = identifier;

	//ignore scope if identifier is scoped
	auto scopeAndName = vstd::splitStringToPair(identifier, ':');

	if(scopeAndName.first != "")
	{
		actualScope = scopeAndName.first;
		actualName = scopeAndName.second;
	}

	if(actualScope.empty())
	{
		return actualName.empty() ? type : type + "." + actualName;
	}
	else
	{
		return actualName.empty() ? actualScope+ ":" + type : actualScope + ":" + type + "." + actualName;
	}
}

VCMI_LIB_NAMESPACE_END
