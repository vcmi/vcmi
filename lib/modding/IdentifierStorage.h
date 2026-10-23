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

class JsonNode;
class CModHandler;
struct IdentifierRequest;

/// class that stores all object identifiers strings and maps them to numeric ID's
/// if possible, objects ID's should be in format <type>.<name>, camelCase e.g. "creature.grandElf"
class DLL_LINKAGE CIdentifierStorage
{
	struct ObjectData // entry created on ID registration
	{
		si32 id;
		std::string scope; /// scope in which this ID located

		bool operator==(const ObjectData & other) const
		{
			return id == other.id && scope == other.scope;
		}
	};

	/// source of active mods and mod dependencies
	const CModHandler & modHandler;

	std::multimap<std::string, ObjectData> registeredObjects;
	std::map<std::string, std::string> registeredObjectsCaseLookup;

	/// Helper method that dumps all registered identifier into log file
	void debugDumpIdentifiers();

	std::vector<ObjectData> getPossibleIdentifiers(const IdentifierRequest & request) const;

	void showIdentifierResolutionErrorDetails(const IdentifierRequest & request) const;
	std::optional<si32> getIdentifierImpl(const IdentifierRequest & request, bool silent) const;
public:
	explicit CIdentifierStorage(const CModHandler & modHandler_);
	virtual ~CIdentifierStorage() = default;

	/// Check if identifier can be valid (camelCase, point as separator)
	static void checkIdentifier(const std::string & ID);

	/// Calls request callback if identifier is found. Returns false and logs error if request has failed
	bool resolveIdentifier(const IdentifierRequest & request) const;

	/// get identifier immediately. If identifier is not know and not silent call will result in error message
	std::optional<si32> getIdentifier(const std::string & scope, const std::string & type, const std::string & name, bool silent = false) const;
	std::optional<si32> getIdentifier(const std::string & type, const JsonNode & name, bool silent = false) const;
	std::optional<si32> getIdentifier(const JsonNode & name, bool silent = false) const;
	std::optional<si32> getIdentifier(const std::string & scope, const std::string & fullName, bool silent = false) const;
	std::optional<si32> getIdentifierCaseInsensitive(const std::string & scope, const std::string & type, const std::string & name, bool silent) const;

	/// registers new object
	void registerObject(const std::string & scope, const std::string & type, const std::string & name, si32 identifier);
};
