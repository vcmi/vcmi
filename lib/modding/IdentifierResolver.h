/*
 * IdentifierResolver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class JsonNode;
class CIdentifierStorage;

/// Request to resolve string identifier into numeric ID
struct DLL_LINKAGE IdentifierRequest
{
	std::string localScope;  /// scope from which this ID was requested
	std::string remoteScope; /// scope in which this object must be found
	std::string type;        /// type, e.g. creature, faction, hero, etc
	std::string name;        /// string ID
	std::function<void(si32)> callback;
	bool optional = false;
	bool bypassDependenciesCheck = false;
	bool dynamicType = false;
	bool caseSensitive = false;

	/// Builds request from identifier in form "targetMod:type.name"
	static IdentifierRequest fromNameWithType(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback, bool optional, bool caseSensitive);

	/// Builds request from identifier in form "targetMod:name"
	static IdentifierRequest fromNameAndType(const std::string & scope, const std::string & type, const std::string & fullName, const std::function<void(si32)> & callback, bool optional, bool bypassDependenciesCheck, bool caseSensitive);

private:
	IdentifierRequest() = default;
};

/// Resolves identifier requests. Depending on implementation, callback is called immediately or later
class DLL_LINKAGE IIdentifierResolver
{
public:
	virtual ~IIdentifierResolver() = default;

	virtual void requestIdentifier(const IdentifierRequest & request) = 0;

	void requestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback);
	///fullName = [remoteScope:]type.name
	void requestIdentifier(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback);
	void requestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback);
	void requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback);

	void requestIdentifierIfNotNull(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback);
	void requestIdentifierIfFound(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback);
	void requestIdentifierIfFound(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback);

	/// try to request ID. If ID with such name won't be loaded, callback function will not be called
	void tryRequestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback);
	void tryRequestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback);
};

/// Resolver used during library loading: requests are queued and resolved in finalize(), once all identifiers are registered
/// Requests made after finalize() are resolved immediately
class DLL_LINKAGE DeferredIdentifierResolver final : public IIdentifierResolver, boost::noncopyable
{
	enum class ELoadingState
	{
		LOADING,
		FINALIZING,
		FINISHED
	};

	const CIdentifierStorage & storage;
	std::vector<IdentifierRequest> scheduledRequests;

	/// All non-optional requests that have failed to resolve, for debug & error reporting
	std::vector<IdentifierRequest> failedRequests;

	ELoadingState state = ELoadingState::LOADING;

	void resolveIdentifier(const IdentifierRequest & request);

public:
	explicit DeferredIdentifierResolver(const CIdentifierStorage & storage_);

	using IIdentifierResolver::requestIdentifier;
	void requestIdentifier(const IdentifierRequest & request) override;

	/// called at the very end of loading to resolve all queued requests
	void finalize();

	/// Returns list of all mods, from which at least one identifier has failed to resolve
	std::vector<std::string> getModsWithFailedRequests() const;
};

/// Resolver for fully loaded library: requests are resolved immediately, failures are only logged
class DLL_LINKAGE InstantIdentifierResolver final : public IIdentifierResolver
{
	const CIdentifierStorage & storage;

public:
	explicit InstantIdentifierResolver(const CIdentifierStorage & storage_);

	using IIdentifierResolver::requestIdentifier;
	void requestIdentifier(const IdentifierRequest & request) override;
};
