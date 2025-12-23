/*
 * IdentifierStorage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

/// class that stores all object identifiers strings and maps them to numeric ID's
/// if possible, objects ID's should be in format <type>.<name>, camelCase e.g. "creature.grandElf"
class DLL_LINKAGE CIdentifierStorage
{
	enum class ELoadingState
	{
		LOADING,
		FINALIZING,
		FINISHED
	};

	struct ObjectCallback // entry created on ID request
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

		/// Builds callback from identifier in form "targetMod:type.name"
		static ObjectCallback fromNameWithType(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback, bool optional, bool caseSensitive);

		/// Builds callback from identifier in form "targetMod:name"
		static ObjectCallback fromNameAndType(const std::string & scope, const std::string & type, const std::string & fullName, const std::function<void(si32)> & callback, bool optional, bool bypassDependenciesCheck, bool caseSensitive);

	private:
		ObjectCallback() = default;
	};

	struct ObjectData // entry created on ID registration
	{
		si32 id;
		std::string scope; /// scope in which this ID located

		bool operator==(const ObjectData & other) const
		{
			return id == other.id && scope == other.scope;
		}
	};

	std::multimap<std::string, ObjectData> registeredObjects;
	std::map<std::string, std::string> registeredObjectsCaseLookup;
	mutable std::vector<ObjectCallback> scheduledRequests;

	/// All non-optional requests that have failed to resolve, for debug & error reporting
	mutable std::vector<ObjectCallback> failedRequests;

	ELoadingState state = ELoadingState::LOADING;

	/// Helper method that dumps all registered identifier into log file
	void debugDumpIdentifiers();

	/// Check if identifier can be valid (camelCase, point as separator)
	static void checkIdentifier(const std::string & ID);

	void requestIdentifier(const ObjectCallback & callback) const;
	bool resolveIdentifier(const ObjectCallback & callback) const;
	std::vector<ObjectData> getPossibleIdentifiers(const ObjectCallback & callback) const;

	void showIdentifierResolutionErrorDetails(const ObjectCallback & callback) const;
	std::optional<si32> getIdentifierImpl(const ObjectCallback & callback, bool silent) const;
public:
	CIdentifierStorage();
	virtual ~CIdentifierStorage() = default;

	/// request identifier for specific object name.
	/// Function callback will be called during ID resolution phase of loading
	void requestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const;
	///fullName = [remoteScope:]type.name
	void requestIdentifier(const std::string & scope, const std::string & fullName, const std::function<void(si32)> & callback) const;
	void requestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const;
	void requestIdentifier(const JsonNode & name, const std::function<void(si32)> & callback) const;

	void requestIdentifierIfNotNull(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const;
	void requestIdentifierIfFound(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const;
	void requestIdentifierIfFound(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const;

	/// try to request ID. If ID with such name won't be loaded, callback function will not be called
	void tryRequestIdentifier(const std::string & scope, const std::string & type, const std::string & name, const std::function<void(si32)> & callback) const;
	void tryRequestIdentifier(const std::string & type, const JsonNode & name, const std::function<void(si32)> & callback) const;

	/// get identifier immediately. If identifier is not know and not silent call will result in error message
	std::optional<si32> getIdentifier(const std::string & scope, const std::string & type, const std::string & name, bool silent = false) const;
	std::optional<si32> getIdentifier(const std::string & type, const JsonNode & name, bool silent = false) const;
	std::optional<si32> getIdentifier(const JsonNode & name, bool silent = false) const;
	std::optional<si32> getIdentifier(const std::string & scope, const std::string & fullName, bool silent = false) const;
	std::optional<si32> getIdentifierCaseInsensitive(const std::string & scope, const std::string & type, const std::string & name, bool silent) const;

	/// registers new object
	void registerObject(const std::string & scope, const std::string & type, const std::string & name, si32 identifier);

	/// called at the very end of loading to check for any missing ID's
	void finalize();

	/// Returns list of all mods, from which at least one identifier has failed to resolve
	std::vector<std::string> getModsWithFailedRequests() const;
};

VCMI_LIB_NAMESPACE_END
