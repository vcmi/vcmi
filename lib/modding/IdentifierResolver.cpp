/*
 * IdentifierResolver.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "IdentifierResolver.h"

#include "IdentifierStorage.h"
#include "ModScope.h"

#include "../json/JsonNode.h"

#include <vstd/StringUtils.h>

IdentifierRequest IdentifierRequest::fromNameWithType(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback, bool optional, bool caseSensitive)
{
	assert(!scope.empty());

	auto scopeAndFullName = vstd::splitStringToPair(fullName, ':');
	auto typeAndName = vstd::splitStringToPair(scopeAndFullName.second, '.');

	if (scope == scopeAndFullName.first)
		logMod->debug("Target scope for identifier '%s' is redundant! Identifier already defined in mod '%s'", fullName, scope);

	IdentifierRequest result;
	result.localScope = scope;
	result.remoteScope = scopeAndFullName.first;
	result.type = typeAndName.first;
	result.name = typeAndName.second;
	result.callback = callback;
	result.optional = optional;
	result.dynamicType = true;
	result.caseSensitive = caseSensitive;
	return result;
}

IdentifierRequest IdentifierRequest::fromNameAndType(const std::string & scope, const std::string & type, const std::string & fullName, const std::function<void(si32)> & callback, bool optional, bool bypassDependenciesCheck, bool caseSensitive)
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

	IdentifierRequest result;
	result.localScope = boost::to_lower_copy(scope);
	result.remoteScope = boost::to_lower_copy(scopeAndFullName.first);
	result.type = type;
	result.name = typeAndName.second;
	result.callback = callback;
	result.optional = optional;
	result.bypassDependenciesCheck = bypassDependenciesCheck;
	result.dynamicType = false;
	result.caseSensitive = caseSensitive;
	return result;
}

void IIdentifierResolver::requestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameAndType(scope, type, name, callback, false, false, true));
}

void IIdentifierResolver::requestIdentifier(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameWithType(scope, fullName, callback, false, true));
}

void IIdentifierResolver::requestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameAndType(name.getModScope(), type, name.String(), callback, false, false, true));
}

void IIdentifierResolver::requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameWithType(name.getModScope(), name.String(), callback, false, true));
}

void IIdentifierResolver::requestIdentifierIfNotNull(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	if (!name.isNull())
		requestIdentifier(type, name, callback);
}

void IIdentifierResolver::requestIdentifierIfFound(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameAndType(name.getModScope(), type, name.String(), callback, false, true, true));
}

void IIdentifierResolver::requestIdentifierIfFound(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameAndType(scope, type, name, callback, false, true, true));
}

void IIdentifierResolver::tryRequestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameAndType(scope, type, name, callback, true, false, true));
}

void IIdentifierResolver::tryRequestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback)
{
	requestIdentifier(IdentifierRequest::fromNameAndType(name.getModScope(), type, name.String(), callback, true, false, true));
}

DeferredIdentifierResolver::DeferredIdentifierResolver(const CIdentifierStorage & storage_):
	storage(storage_)
{
}

void DeferredIdentifierResolver::requestIdentifier(const IdentifierRequest & request)
{
	CIdentifierStorage::checkIdentifier(request.type);
	CIdentifierStorage::checkIdentifier(request.name);

	assert(!request.localScope.empty());

	if (state != ELoadingState::FINISHED) // enqueue request if loading is still in progress
		scheduledRequests.push_back(request);
	else // execute immediately for "late" requests
		resolveIdentifier(request);
}

void DeferredIdentifierResolver::resolveIdentifier(const IdentifierRequest & request)
{
	if (!storage.resolveIdentifier(request))
		failedRequests.push_back(request);
}

void DeferredIdentifierResolver::finalize()
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

std::vector<std::string> DeferredIdentifierResolver::getModsWithFailedRequests() const
{
	std::vector<std::string> result;

	for (const auto & request : failedRequests)
		if (!vstd::contains(result, request.localScope) && !ModScope::isScopeReserved(request.localScope))
			result.push_back(request.localScope);

	return result;
}

InstantIdentifierResolver::InstantIdentifierResolver(const CIdentifierStorage & storage_):
	storage(storage_)
{
}

void InstantIdentifierResolver::requestIdentifier(const IdentifierRequest & request)
{
	CIdentifierStorage::checkIdentifier(request.type);
	CIdentifierStorage::checkIdentifier(request.name);

	assert(!request.localScope.empty());

	storage.resolveIdentifier(request);
}
