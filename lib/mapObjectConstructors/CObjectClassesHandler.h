/*
 * CObjectClassesHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../IHandlerBase.h"
#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomGenerator;
class AObjectTypeHandler;
class ObjectTemplate;
struct SObjectSounds;

struct DLL_LINKAGE CompoundMapObjectID
{
	si32 primaryID;
	si32 secondaryID;

	CompoundMapObjectID(si32 primID, si32 secID) : primaryID(primID), secondaryID(secID) {};

	bool operator<(const CompoundMapObjectID& other) const
	{
		if(this->primaryID != other.primaryID)
			return this->primaryID < other.primaryID;
		else
			return this->secondaryID < other.secondaryID;
	}

	bool operator==(const CompoundMapObjectID& other) const
	{
		return (this->primaryID == other.primaryID) && (this->secondaryID == other.secondaryID);
	}
};

class CGObjectInstance;

using TObjectTypeHandler = std::shared_ptr<AObjectTypeHandler>;

/// Class responsible for creation of adventure map objects of specific type
class DLL_LINKAGE ObjectClass
{
public:
	std::string modScope;
	std::string identifier;

	si32 id;
	std::string handlerName; // ID of handler that controls this object, should be determined using handlerConstructor map

	JsonNode base;
	std::vector<TObjectTypeHandler> objects;

	ObjectClass() = default;

	std::string getJsonKey() const;
	std::string getNameTextID() const;
	std::string getNameTranslated() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & handlerName;
		h & base;
		h & objects;
		h & identifier;
		h & modScope;
	}
};

/// Main class responsible for creation of all adventure map objects
class DLL_LINKAGE CObjectClassesHandler : public IHandlerBase
{
	/// list of object handlers, each of them handles only one type
	std::vector<ObjectClass * > objects;

	/// map that is filled during contruction with all known handlers. Not serializeable due to usage of std::function
	std::map<std::string, std::function<TObjectTypeHandler()> > handlerConstructors;

	/// container with H3 templates, used only during loading, no need to serialize it
	using TTemplatesContainer = std::multimap<std::pair<si32, si32>, std::shared_ptr<const ObjectTemplate>>;
	TTemplatesContainer legacyTemplates;

	TObjectTypeHandler loadSubObjectFromJson(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index);

	void loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj);
	void loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index);

	ObjectClass * loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index);

	void generateExtraMonolithsForRMG();

public:
	CObjectClassesHandler();
	~CObjectClassesHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void loadSubObject(const std::string & identifier, JsonNode config, si32 ID, si32 subID);
	void removeSubObject(si32 ID, si32 subID);

	void beforeValidate(JsonNode & object) override;
	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;

	/// Queries to detect loaded objects
	std::set<si32> knownObjects() const;
	std::set<si32> knownSubObjects(si32 primaryID) const;

	/// returns handler for specified object (ID-based). ObjectHandler keeps ownership
	TObjectTypeHandler getHandlerFor(si32 type, si32 subtype) const;
	TObjectTypeHandler getHandlerFor(const std::string & scope, const std::string & type, const std::string & subtype) const;
	TObjectTypeHandler getHandlerFor(CompoundMapObjectID compoundIdentifier) const;

	std::string getObjectName(si32 type, si32 subtype) const;

	SObjectSounds getObjectSounds(si32 type, si32 subtype) const;

	/// Returns handler string describing the handler (for use in client)
	std::string getObjectHandlerName(si32 type) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
	}
};

VCMI_LIB_NAMESPACE_END
