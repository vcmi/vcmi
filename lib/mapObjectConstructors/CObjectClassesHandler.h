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

#include "../mapObjects/CompoundMapObjectID.h"
#include "../IHandlerBase.h"
#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class AObjectTypeHandler;
class ObjectTemplate;
struct SObjectSounds;

class CGObjectInstance;

using TObjectTypeHandler = std::shared_ptr<AObjectTypeHandler>;

/// Class responsible for creation of adventure map objects of specific type
class DLL_LINKAGE ObjectClass : boost::noncopyable
{
public:
	std::string modScope;
	std::string identifier;

	si32 id;
	std::string handlerName; // ID of handler that controls this object, should be determined using handlerConstructor map

	JsonNode base;
	std::vector<TObjectTypeHandler> objectTypeHandlers;

	ObjectClass();
	~ObjectClass();

	std::string getJsonKey() const;
	std::string getNameTextID() const;
	std::string getNameTranslated() const;
};

/// Main class responsible for creation of all adventure map objects
class DLL_LINKAGE CObjectClassesHandler : public IHandlerBase
{
	/// list of object handlers, each of them handles only one type
	std::vector< std::unique_ptr<ObjectClass> > mapObjectTypes;

	/// map that is filled during construction with all known handlers. Not serializeable due to usage of std::function
	std::map<std::string, std::function<TObjectTypeHandler()> > handlerConstructors;

	std::vector<std::pair<CompoundMapObjectID, std::function<void(CompoundMapObjectID)>>> objectIdHandlers;

	/// container with H3 templates, used only during loading, no need to serialize it
	using TTemplatesContainer = std::multimap<std::pair<MapObjectID, MapObjectSubID>, std::shared_ptr<const ObjectTemplate>>;
	TTemplatesContainer legacyTemplates;

	TObjectTypeHandler loadSubObjectFromJson(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index);

	void loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj);
	void loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index);

	std::unique_ptr<ObjectClass> loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index);

	void generateExtraMonolithsForRMG(ObjectClass * container);

public:
	CObjectClassesHandler();
	~CObjectClassesHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void loadSubObject(const std::string & identifier, JsonNode config, MapObjectID ID, MapObjectSubID subID);
	void removeSubObject(MapObjectID ID, MapObjectSubID subID);

	void beforeValidate(JsonNode & object) override;
	void afterLoadFinalization() override;

	/// Queries to detect loaded objects
	std::set<MapObjectID> knownObjects() const;
	std::set<MapObjectSubID> knownSubObjects(MapObjectID primaryID) const;

	/// returns handler for specified object (ID-based). ObjectHandler keeps ownership
	TObjectTypeHandler getHandlerFor(MapObjectID type, MapObjectSubID subtype) const;
	TObjectTypeHandler getHandlerFor(const std::string & scope, const std::string & type, const std::string & subtype) const;
	TObjectTypeHandler getHandlerFor(CompoundMapObjectID compoundIdentifier) const;
	CompoundMapObjectID getCompoundIdentifier(const std::string & scope, const std::string & type, const std::string & subtype) const;
	CompoundMapObjectID getCompoundIdentifier(const std::string & objectName) const;

	std::string getObjectName(MapObjectID type, MapObjectSubID subtype) const;

	SObjectSounds getObjectSounds(MapObjectID type, MapObjectSubID subtype) const;

	void resolveObjectCompoundId(const std::string & id, std::function<void(CompoundMapObjectID)> callback);

	/// Returns handler string describing the handler (for use in client)
	std::string getObjectHandlerName(MapObjectID type) const;

	std::string getJsonKey(MapObjectID type) const;
};

VCMI_LIB_NAMESPACE_END
