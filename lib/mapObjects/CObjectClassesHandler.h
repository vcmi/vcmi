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

#include "ObjectTemplate.h"

#include "../GameConstants.h"
#include "../ConstTransitivePtr.h"
#include "../IHandlerBase.h"
#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class CRandomGenerator;


struct SObjectSounds
{
	std::vector<std::string> ambient;
	std::vector<std::string> visit;
	std::vector<std::string> removal;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ambient;
		h & visit;
		h & removal;
	}
};

/// Structure that describes placement rules for this object in random map
struct DLL_LINKAGE RandomMapInfo
{
	/// How valuable this object is, 1k = worthless, 10k = Utopia-level
	ui32 value;

	/// How many of such objects can be placed on map, 0 = object can not be placed by RMG
	ui32 mapLimit;

	/// How many of such objects can be placed in one zone, 0 = unplaceable
	ui32 zoneLimit;

	/// Rarity of object, 5 = extremely rare, 100 = common
	ui32 rarity;

	RandomMapInfo():
		value(0),
		mapLimit(0),
		zoneLimit(0),
		rarity(0)
	{}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & value;
		h & mapLimit;
		h & zoneLimit;
		h & rarity;
	}
};

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

class DLL_LINKAGE IObjectInfo
{
public:
	struct CArmyStructure
	{
		ui32 totalStrength;
		ui32 shootersStrength;
		ui32 flyersStrength;
		ui32 walkersStrength;

		CArmyStructure() :
			totalStrength(0),
			shootersStrength(0),
			flyersStrength(0),
			walkersStrength(0)
		{}

		bool operator <(const CArmyStructure & other) const
		{
			return this->totalStrength < other.totalStrength;
		}
	};

	/// Returns possible composition of guards. Actual guards would be
	/// somewhere between these two values
	virtual CArmyStructure minGuards() const { return CArmyStructure(); }
	virtual CArmyStructure maxGuards() const { return CArmyStructure(); }

	virtual bool givesResources() const { return false; }

	virtual bool givesExperience() const { return false; }
	virtual bool givesMana() const { return false; }
	virtual bool givesMovement() const { return false; }

	virtual bool givesPrimarySkills() const { return false; }
	virtual bool givesSecondarySkills() const { return false; }

	virtual bool givesArtifacts() const { return false; }
	virtual bool givesCreatures() const { return false; }
	virtual bool givesSpells() const { return false; }

	virtual bool givesBonuses() const { return false; }

	virtual ~IObjectInfo() = default;
};

class CGObjectInstance;

/// Class responsible for creation of objects of specific type & subtype
class DLL_LINKAGE AObjectTypeHandler : public boost::noncopyable
{
	friend class CObjectClassesHandler;

	RandomMapInfo rmgInfo;

	JsonNode base; /// describes base template

	std::vector<std::shared_ptr<const ObjectTemplate>> templates;

	SObjectSounds sounds;

	boost::optional<si32> aiValue;
	boost::optional<std::string> battlefield;

	std::string modScope;
	std::string typeName;
	std::string subTypeName;

	si32 type;
	si32 subtype;

protected:
	void preInitObject(CGObjectInstance * obj) const;
	virtual bool objectFilter(const CGObjectInstance *, std::shared_ptr<const ObjectTemplate>) const;

	/// initialization for classes that inherit this one
	virtual void initTypeData(const JsonNode & input);
public:

	AObjectTypeHandler();
	virtual ~AObjectTypeHandler();

	si32 getIndex() const;
	si32 getSubIndex() const;

	std::string getTypeName() const;
	std::string getSubTypeName() const;

	/// loads generic data from Json structure and passes it towards type-specific constructors
	void init(const JsonNode & input);

	/// returns full form of identifier of this object in form of modName:objectName
	std::string getJsonKey() const;

	/// Returns object-specific name, if set
	SObjectSounds getSounds() const;

	void addTemplate(std::shared_ptr<const ObjectTemplate> templ);
	void addTemplate(JsonNode config);

	/// returns all templates matching parameters
	std::vector<std::shared_ptr<const ObjectTemplate>> getTemplates() const;
	std::vector<std::shared_ptr<const ObjectTemplate>> getTemplates(const TerrainId terrainType) const;

	/// returns preferred template for this object, if present (e.g. one of 3 possible templates for town - village, fort and castle)
	/// note that appearance will not be changed - this must be done separately (either by assignment or via pack from server)
	std::shared_ptr<const ObjectTemplate> getOverride(TerrainId terrainType, const CGObjectInstance * object) const;

	BattleField getBattlefield() const;

	const RandomMapInfo & getRMGInfo();

	boost::optional<si32> getAiValue() const;

	/// returns true if this class provides custom text ID's instead of generic per-object name
	virtual bool hasNameTextID() const;

	/// returns object's name in form of translatable text ID
	virtual std::string getNameTextID() const;

	/// returns object's name in form of human-readable text
	std::string getNameTranslated() const;

	virtual bool isStaticObject();

	virtual void afterLoadFinalization();

	/// Creates object and set up core properties (like ID/subID). Object is NOT initialized
	/// to allow creating objects before game start (e.g. map loading)
	virtual CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const = 0;

	/// Configures object properties. Should be re-entrable, resetting state of the object if necessarily
	/// This should set remaining properties, including randomized or depending on map
	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const = 0;

	/// Returns object configuration, if available. Otherwise returns NULL
	virtual std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type;
		h & subtype;
		h & templates;
		h & rmgInfo;
		h & modScope;
		h & typeName;
		h & subTypeName;
		h & sounds;
		h & aiValue;
		h & battlefield;
	}
};

typedef std::shared_ptr<AObjectTypeHandler> TObjectTypeHandler;

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
	typedef std::multimap<std::pair<si32, si32>, std::shared_ptr<const ObjectTemplate>> TTemplatesContainer;
	TTemplatesContainer legacyTemplates;

	TObjectTypeHandler loadSubObjectFromJson(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index);

	void loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj);
	void loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index);

	ObjectClass * loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index);

public:
	CObjectClassesHandler();
	~CObjectClassesHandler();

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

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
	TObjectTypeHandler getHandlerFor(std::string scope, std::string type, std::string subtype) const;
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
