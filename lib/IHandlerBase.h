#pragma once

/*
 * IHandlerBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class JsonNode;

/// base class for all handlers that can be accessed from mod system
class DLL_LINKAGE IHandlerBase
{
	// there also should be private member with such signature:
	// Object * loadFromJson(const JsonNode & json);
	// where Object is type of data loaded by handler
	// primary used in loadObject methods

public:
	/// loads all original game data in vector of json nodes
	/// dataSize - is number of items that must be loaded (normally - constant from GameConstants)
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) = 0;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data) = 0;
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) = 0;

	/**
	 * Gets a list of objects that are allowed by default on maps
	 *
	 * @return a list of allowed objects, the index is the object id
	 */
	virtual std::vector<bool> getDefaultAllowed() const = 0;

	virtual ~IHandlerBase(){}
};