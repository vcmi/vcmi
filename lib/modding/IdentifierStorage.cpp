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

#include "../JsonNode.h"
#include "../VCMI_Lib.h"

#include <vstd/StringUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

CIdentifierStorage::CIdentifierStorage():
	state(LOADING)
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

void CIdentifierStorage::requestIdentifier(ObjectCallback callback) const
{
	checkIdentifier(callback.type);
	checkIdentifier(callback.name);

	assert(!callback.localScope.empty());

	if (state != FINISHED) // enqueue request if loading is still in progress
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
	return result;
}

CIdentifierStorage::ObjectCallback CIdentifierStorage::ObjectCallback::fromNameAndType(const std::string & scope, const std::string & type, const std::string & fullName, const std::function<void(si32)> & callback, bool optional)
{
	assert(!scope.empty());

	auto scopeAndFullName = vstd::splitStringToPair(fullName, ':');
	auto typeAndName = vstd::splitStringToPair(scopeAndFullName.second, '.');

	if(!typeAndName.first.empty())
	{
		if (typeAndName.first != type)
			logMod->error("Identifier '%s' from mod '%s' requested with different type! Type '%s' expected!", fullName, scope, type);
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
	return result;
}

void CIdentifierStorage::requestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(scope, type, name, callback, false));
}

void CIdentifierStorage::requestIdentifier(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameWithType(scope, fullName, callback, false));
}

void CIdentifierStorage::requestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(name.meta, type, name.String(), callback, false));
}

void CIdentifierStorage::requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameWithType(name.meta, name.String(), callback, false));
}

void CIdentifierStorage::tryRequestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(scope, type, name, callback, true));
}

void CIdentifierStorage::tryRequestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const
{
	requestIdentifier(ObjectCallback::fromNameAndType(name.meta, type, name.String(), callback, true));
}

std::optional<si32> CIdentifierStorage::getIdentifier(const std::string & scope, const std::string & type, const std::string & name, bool silent) const
{
	auto idList = getPossibleIdentifiers(ObjectCallback::fromNameAndType(scope, type, name, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s of type %s from mod %s", name , type ,scope);

	return std::optional<si32>();
}

std::optional<si32> CIdentifierStorage::getIdentifier(const std::string & type, const JsonNode & name, bool silent) const
{
	auto idList = getPossibleIdentifiers(ObjectCallback::fromNameAndType(name.meta, type, name.String(), std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s of type %s from mod %s", name.String(), type, name.meta);

	return std::optional<si32>();
}

std::optional<si32> CIdentifierStorage::getIdentifier(const JsonNode & name, bool silent) const
{
	auto idList = getPossibleIdentifiers(ObjectCallback::fromNameWithType(name.meta, name.String(), std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s from mod %s", name.String(), name.meta);

	return std::optional<si32>();
}

std::optional<si32> CIdentifierStorage::getIdentifier(const std::string & scope, const std::string & fullName, bool silent) const
{
	auto idList = getPossibleIdentifiers(ObjectCallback::fromNameWithType(scope, fullName, std::function<void(si32)>(), silent));

	if (idList.size() == 1)
		return idList.front().id;
	if (!silent)
		logMod->error("Failed to resolve identifier %s from mod %s", fullName, scope);

	return std::optional<si32>();
}

void CIdentifierStorage::registerObject(const std::string & scope, const std::string & type, const std::string & name, si32 identifier)
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
			for(const auto & modName : VLC->modh->getActiveMods())
				allowedScopes.insert(modName);
		}

		// normally ID's from all required mods, own mod and virtual built-in mod are allowed
		else if(request.localScope != ModScope::scopeBuiltin() && !request.localScope.empty())
		{
			allowedScopes = VLC->modh->getModDependencies(request.localScope, isValidScope);

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
		else
		{
			// allow access only if mod is in our dependencies
			auto myDeps = VLC->modh->getModDependencies(request.localScope, isValidScope);

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

	if (request.optional && identifiers.empty()) // failed to resolve optinal ID
	{
		return true;
	}

	// error found. Try to generate some debug info
	if(identifiers.empty())
		logMod->error("Unknown identifier!");
	else
		logMod->error("Ambiguous identifier request!");

	 logMod->error("Request for %s.%s from mod %s", request.type, request.name, request.localScope);

	for(const auto & id : identifiers)
	{
		logMod->error("\tID is available in mod %s", id.scope);
	}
	return false;
}

void CIdentifierStorage::finalize()
{
	state = FINALIZING;
	bool errorsFound = false;

	while ( !scheduledRequests.empty() )
	{
		// Use local copy since new requests may appear during resolving, invalidating any iterators
		auto request = scheduledRequests.back();
		scheduledRequests.pop_back();

		if (!resolveIdentifier(request))
			errorsFound = true;
	}

	if (errorsFound)
	{
		for(const auto & object : registeredObjects)
		{
			logMod->trace("%s : %s -> %d", object.second.scope, object.first, object.second.id);
		}
		logMod->error("All known identifiers were dumped into log file");
	}
	assert(errorsFound == false);
	state = FINISHED;
}

VCMI_LIB_NAMESPACE_END
