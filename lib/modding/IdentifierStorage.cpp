/*
 * IdentifierStorage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "IdentifierStorage.h"

#include "CModHandler.h"
#include "ModScope.h"

#include "../GameLibrary.h"
#include "../constants/StringConstants.h"
#include "../spells/CSpellHandler.h"

#include <vstd/StringUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

CIdentifierStorage::CIdentifierStorage()
{
	registerObject(ModScope::scopeBuiltin(), "spellSchool", "any", SpellSchool::ANY.getNum());

	for (int i = 0; i < GameConstants::RESOURCE_QUANTITY; ++i)
		registerObject(ModScope::scopeBuiltin(), "resource", GameConstants::RESOURCE_NAMES[i], i);

	for (int i = 0; i < std::size(GameConstants::PLAYER_COLOR_NAMES); ++i)
		registerObject(ModScope::scopeBuiltin(), "playerColor", GameConstants::PLAYER_COLOR_NAMES[i], i);


	for(int i=0; i<GameConstants::PRIMARY_SKILLS; ++i)
	{
		registerObject(ModScope::scopeBuiltin(), "primSkill", NPrimarySkill::names[i], i);
		registerObject(ModScope::scopeBuiltin(), "primarySkill", NPrimarySkill::names[i], i);
	}

	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureDamageBoth", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureDamageMin", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureDamageMax", 2);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "damageTypeAll", -1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "damageTypeMelee", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "damageTypeRanged", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "heroMovementLand", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "heroMovementSea", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "deathStareGorgon", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "deathStareCommander", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "deathStareNoRangePenalty", 2);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "deathStareRangePenalty", 3);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "deathStareObstaclePenalty", 4);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "deathStareRangeObstaclePenalty", 5);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "rebirthRegular", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "rebirthSpecial", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "visionsMonsters", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "visionsHeroes", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "visionsTowns", 2);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "immunityBattleWide", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "immunityEnemyHero", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "transmutationPerHealth", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "transmutationPerUnit", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "destructionKillPercentage", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "destructionKillAmount", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "soulStealPermanent", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "soulStealBattle", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "movementFlying", 0);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "movementTeleporting", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "spellLevel1", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "spellLevel2", 2);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "spellLevel3", 3);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "spellLevel4", 4);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "spellLevel5", 5);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureLevel1", 1);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureLevel2", 2);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureLevel3", 3);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureLevel4", 4);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureLevel5", 5);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureLevel6", 6);
	registerObject(ModScope::scopeBuiltin(), "bonusSubtype", "creatureLevel7", 7);
	registerObject(ModScope::scopeBuiltin(), "spell", "preset", SpellID::PRESET);
	registerObject(ModScope::scopeBuiltin(), "spell", "spellbook_preset", SpellID::SPELLBOOK_PRESET);
}

void CIdentifierStorage::checkIdentifier(const std::string & ID)
{
	if (boost::algorithm::ends_with(ID, "."))
		logMod->error("BIG WARNING: identifier %s seems to be broken!", ID);
}

void CIdentifierStorage::requestIdentifier(const ObjectCallback & callback) const
{
	checkIdentifier(callback.type);
	checkIdentifier(callback.name);

	assert(!callback.localScope.empty());

	if (state != ELoadingState::FINISHED) // enqueue request if loading is still in progress
		scheduledRequests.push_back(callback);
	else // execute immediately for "late" requests
		resolveIdentifier(callback);
}

CIdentifierStorage::ObjectCallback CIdentifierStorage::ObjectCallback::fromNameWithType(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback, bool optional)
{
	assert(!scope.empty());

	auto scopeAndFullName = vstd::splitStringToPair(fullName, ':');
	auto typeAndName = vstd::splitStringToPair(scopeAndFullName.second, '.');

	if (scope == scopeAndFullName.first)
		logMod->debug("Target scope for identifier '%s' is redundant! Identifier already defined in mod '%s'", fullName, scope);

	ObjectCallback result;
	result.localScope = scope;
	result.remoteScope = scopeAndFullName.first;
	result.type = typeAndName.first;
	result.name = typeAndName.second;
	result.callback = callback;
	result.optional = optional;
	result.dynamicType = true;
	return result;
}

CIdentifierStorage::ObjectCallback CIdentifierStorage::ObjectCallback::fromNameAndType(const std::string & scope, const std::string & type, const std::string & fullName, const std::function<void(si32)> & callback, bool optional, bool bypassDependenciesCheck)
{
	assert(!scope.empty());

	auto scopeAndFullName = vstd::splitStringToPair(fullName, ':');
	auto typeAndName = vstd::splitStringToPair(scopeAndFullName.second, '.');

	if(!typeAndName.first.empty())
	{
		if (typeAndName.first != type)
			logMod->warn("Identifier '%s' from mod '%s' requested with different type! Type '%s' expected!", fullName, scope, type);
		else
			logMod->debug("Target type for identifier '%s' defined in mod '%s' is redundant!", fullName, scope);
	}

	if (scope == scopeAndFullName.first)
		logMod->debug("Target scope for identifier '%s' is redundant! Identifier already defined in mod '%s'", fullName, scope);

	ObjectCallback result;
	result.localScope = scope;
	result.remoteScope = scopeAndFullName.first;
	result.type = type;
	result.name = typeAndName.second;
	result.callback = callback;
	result.optional = optional;
	result.bypassDependenciesCheck = bypassDependenciesCheck;
	result.dynamicType = false;
	return result;
}

void CIdentifierStorage::requestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(scope, type, name, callback, false, false));
}

void CIdentifierStorage::requestIdentifier(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameWithType(scope, fullName, callback, false));
}

void CIdentifierStorage::requestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(name.getModScope(), type, name.String(), callback, false, false));
}

void CIdentifierStorage::requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameWithType(name.getModScope(), name.String(), callback, false));
}

void CIdentifierStorage::requestIdentifierIfNotNull(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const
{
	if (!name.isNull())
		requestIdentifier(type, name, callback);
}

void CIdentifierStorage::requestIdentifierIfFound(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(name.getModScope(), type, name.String(), callback, false, true));
}

void CIdentifierStorage::requestIdentifierIfFound(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(scope, type, name, callback, false, true));
}

void CIdentifierStorage::tryRequestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(scope, type, name, callback, true, false));
}

void CIdentifierStorage::tryRequestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(name.getModScope(), type, name.String(), callback, true, false));
}

std::optional<si32> CIdentifierStorage::getIdentifier(const std::string & scope, const std::string & type, const std::string & name, bool silent) const
{
	//assert(state != ELoadingState::LOADING);

	auto options = ObjectCallback::fromNameAndType(scope, type, name, std::function<void(si32)>(), silent, false);
	return getIdentifierImpl(options, silent);
}

std::optional<si32> CIdentifierStorage::getIdentifier(const std::string & type, const JsonNode & name, bool silent) const
{
	assert(state != ELoadingState::LOADING);

	auto options = ObjectCallback::fromNameAndType(name.getModScope(), type, name.String(), std::function<void(si32)>(), silent, false);

	return getIdentifierImpl(options, silent);
}

std::optional<si32> CIdentifierStorage::getIdentifier(const JsonNode & name, bool silent) const
{
	assert(state != ELoadingState::LOADING);

	auto options = ObjectCallback::fromNameWithType(name.getModScope(), name.String(), std::function<void(si32)>(), silent);
	return getIdentifierImpl(options, silent);
}

std::optional<si32> CIdentifierStorage::getIdentifier(const std::string & scope, const std::string & fullName, bool silent) const
{
	assert(state != ELoadingState::LOADING);

	auto options = ObjectCallback::fromNameWithType(scope, fullName, std::function<void(si32)>(), silent);
	return getIdentifierImpl(options, silent);
}

std::optional<si32> CIdentifierStorage::getIdentifierImpl(const ObjectCallback & options, bool silent) const
{
	auto idList = getPossibleIdentifiers(options);

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		showIdentifierResolutionErrorDetails(options);
	return std::optional<si32>();
}

void CIdentifierStorage::showIdentifierResolutionErrorDetails(const ObjectCallback & options) const
{
	auto idList = getPossibleIdentifiers(options);

	logMod->error("Failed to resolve identifier '%s' of type '%s' from mod '%s'", options.name, options.type, options.localScope);

	if (options.dynamicType && options.type.empty())
	{
		bool suggestionFound = false;

		for (auto const & entry : registeredObjects)
		{
			if (!boost::algorithm::ends_with(entry.first, options.name))
				continue;

			suggestionFound = true;
			logMod->error("Perhaps you wanted to use identifier '%s' from mod '%s' instead?", entry.first, entry.second.scope);
		}

		if (suggestionFound)
			return;
	}

	if (idList.empty())
	{
		// check whether identifier is unavailable due to a missing dependency on a mod
		ObjectCallback testOptions = options;
		testOptions.localScope = ModScope::scopeGame();
		testOptions.remoteScope = {};

		auto testList = getPossibleIdentifiers(testOptions);
		if (testList.empty())
		{
			logMod->error("Identifier '%s' of type '%s' does not exists in any loaded mod!", options.name, options.type);
		}
		else
		{
			// such identifier(s) exists, but were not picked for some reason
			for (auto const & testOption : testList)
			{
				// attempt to access identifier from mods that is not dependency
				bool isValidScope = true;
				const auto & dependencies = LIBRARY->modh->getModDependencies(options.localScope, isValidScope);
				if (!vstd::contains(dependencies, testOption.scope))
				{
					logMod->error("Identifier '%s' exists in mod %s", options.name, testOption.scope);
					logMod->error("Please add mod '%s' as dependency of mod '%s' to access this identifier", testOption.scope, options.localScope);
					continue;
				}

				// attempt to access identifier in form 'modName:object', but identifier is only present in different mod
				if (options.remoteScope.empty())
				{
					logMod->error("Identifier '%s' exists in mod '%s' but identifier was explicitly requested from mod '%s'!", options.name, testOption.scope, options.remoteScope);
					if (options.dynamicType)
						logMod->error("Please use form '%s.%s' or '%s:%s.%s' to access this identifier", options.type, options.name, testOption.scope, options.type, options.name);
					else
						logMod->error("Please use form '%s' or '%s:%s' to access this identifier", options.name, testOption.scope, options.name);
					continue;
				}
			}
		}
	}
	else
	{
		logMod->error("Multiple possible candidates:");
		for (auto const & testOption : idList)
		{
			logMod->error("Identifier %s exists in mod %s", options.name, testOption.scope);
			if (options.dynamicType)
				logMod->error("Please use '%s:%s.%s' to access this identifier", testOption.scope, options.type, options.name);
			else
				logMod->error("Please use '%s:%s' to access this identifier", testOption.scope, options.name);
		}
	}
}

void CIdentifierStorage::registerObject(const std::string & scope, const std::string & type, const std::string & name, si32 identifier)
{
	assert(state != ELoadingState::FINISHED);

	ObjectData data;
	data.scope = scope;
	data.id = identifier;

	std::string fullID = type + '.' + name;
	checkIdentifier(fullID);

	std::pair<const std::string, ObjectData> mapping = std::make_pair(fullID, data);
	if(!vstd::containsMapping(registeredObjects, mapping))
	{
		logMod->trace("registered '%s' as %s:%s", fullID, scope, identifier);
		registeredObjects.insert(mapping);
	}
	else
	{
		logMod->trace("Duplicate object '%s' found!", fullID);
	}
}

std::vector<CIdentifierStorage::ObjectData> CIdentifierStorage::getPossibleIdentifiers(const ObjectCallback & request) const
{
	std::set<std::string> allowedScopes;
	bool isValidScope = true;

	// called have not specified destination mod explicitly
	if (request.remoteScope.empty())
	{
		// special scope that should have access to all in-game objects
		if (request.localScope == ModScope::scopeGame())
		{
			for(const auto & modName : LIBRARY->modh->getActiveMods())
				allowedScopes.insert(modName);
		}

		// normally ID's from all required mods, own mod and virtual built-in mod are allowed
		else if(request.localScope != ModScope::scopeBuiltin() && !request.localScope.empty())
		{
			allowedScopes = LIBRARY->modh->getModDependencies(request.localScope, isValidScope);

			if(!isValidScope)
				return std::vector<ObjectData>();

			allowedScopes.insert(request.localScope);
		}

		// all mods can access built-in mod
		allowedScopes.insert(ModScope::scopeBuiltin());
	}
	else
	{
		//if destination mod was specified explicitly, restrict lookup to this mod
		if(request.remoteScope == ModScope::scopeBuiltin() )
		{
			//built-in mod is an implicit dependency for all mods, allow access into it
			allowedScopes.insert(request.remoteScope);
		}
		else if ( request.localScope == ModScope::scopeGame() )
		{
			// allow access, this is special scope that should have access to all in-game objects
			allowedScopes.insert(request.remoteScope);
		}
		else if(request.remoteScope == request.localScope )
		{
			// allow self-access
			allowedScopes.insert(request.remoteScope);
		}
		else if (request.bypassDependenciesCheck)
		{
			// this is request for an identifier that bypasses mod dependencies check
			allowedScopes.insert(request.remoteScope);
		}
		else
		{
			// allow access only if mod is in our dependencies
			auto myDeps = LIBRARY->modh->getModDependencies(request.localScope, isValidScope);

			if(!isValidScope)
				return std::vector<ObjectData>();

			if(myDeps.count(request.remoteScope))
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

bool CIdentifierStorage::resolveIdentifier(const ObjectCallback & request) const
{
	auto identifiers = getPossibleIdentifiers(request);
	if (identifiers.size() == 1) // normally resolved ID
	{
		request.callback(identifiers.front().id);
		return true;
	}

	if (request.optional && identifiers.empty()) // failed to resolve optional ID
	{
		return true;
	}

	if (request.bypassDependenciesCheck)
	{
		if (!vstd::contains(LIBRARY->modh->getActiveMods(), request.remoteScope))
		{
			logMod->debug("Mod '%s' requested identifier '%s' from not loaded mod '%s'. Ignoring.", request.localScope, request.remoteScope, request.name);
			return true; // mod was not loaded - ignore
		}
	}

	// error found. Try to generate some debug info
	failedRequests.push_back(request);
	showIdentifierResolutionErrorDetails(request);
	return false;
}

void CIdentifierStorage::finalize()
{
	assert(state == ELoadingState::LOADING);

	state = ELoadingState::FINALIZING;

	while ( !scheduledRequests.empty() )
	{
		// Use local copy since new requests may appear during resolving, invalidating any iterators
		auto request = scheduledRequests.back();
		scheduledRequests.pop_back();
		resolveIdentifier(request);
	}

	state = ELoadingState::FINISHED;
}

void CIdentifierStorage::debugDumpIdentifiers()
{
	logMod->trace("List of all registered objects:");

	std::map<std::string, std::vector<std::string>> objectList;

	for(const auto & object : registeredObjects)
	{
		size_t categoryLength = object.first.find('.');
		assert(categoryLength != std::string::npos);

		std::string objectCategory = object.first.substr(0, categoryLength);
		std::string objectName = object.first.substr(categoryLength + 1);

		objectList[objectCategory].push_back("[" + object.second.scope + "] " + objectName);
	}

	for(auto & category : objectList)
		boost::range::sort(category.second);

	for(const auto & category : objectList)
	{
		logMod->trace("");
		logMod->trace("### %s", category.first);
		logMod->trace("");

		for(const auto & entry : category.second)
			logMod->trace("- " + entry);
	}
}

std::vector<std::string> CIdentifierStorage::getModsWithFailedRequests() const
{
	std::vector<std::string> result;

	for (const auto & request : failedRequests)
		if (!vstd::contains(result, request.localScope) && !ModScope::isScopeReserved(request.localScope))
			result.push_back(request.localScope);

	return result;
}

VCMI_LIB_NAMESPACE_END
