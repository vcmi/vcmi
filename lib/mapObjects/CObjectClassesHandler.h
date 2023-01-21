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

class DLL_LINKAGE AObjectTypeHandler : public boost::noncopyable
{
	RandomMapInfo rmgInfo;

	/// Human-readable name of this object, used for objects like banks and dwellings, if set
	boost::optional<std::string> objectName;

	JsonNode base; /// describes base template

	std::vector<std::shared_ptr<const ObjectTemplate>> templates;

	SObjectSounds sounds;

	boost::optional<si32> aiValue;

	boost::optional<std::string> battlefield;

protected:
	void preInitObject(CGObjectInstance * obj) const;
	virtual bool objectFilter(const CGObjectInstance *, std::shared_ptr<const ObjectTemplate>) const;

	/// initialization for classes that inherit this one
	virtual void initTypeData(const JsonNode & input);
public:
	std::string typeName;
	std::string subTypeName;

	si32 type;
	si32 subtype;
	AObjectTypeHandler();
	virtual ~AObjectTypeHandler();

	void setType(si32 type, si32 subtype);
	void setTypeName(std::string type, std::string subtype);

	/// loads generic data from Json structure and passes it towards type-specific constructors
	void init(const JsonNode & input, boost::optional<std::string> name = boost::optional<std::string>());

	/// Returns object-specific name, if set
	boost::optional<std::string> getCustomName() const;
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

	/// returns preferred template for this object, if present (e.g. one of 3 possible templates for town - village, fort and castle)
	/// note that appearance will not be changed - this must be done separately (either by assignment or via pack from server)

	const RandomMapInfo & getRMGInfo();

	boost::optional<si32> getAiValue() const;

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
		h & objectName;
		h & typeName;
		h & subTypeName;
		h & sounds;
		h & aiValue;
		h & battlefield;
	}
};

typedef std::shared_ptr<AObjectTypeHandler> TObjectTypeHandler;

class DLL_LINKAGE CObjectClassesHandler : public IHandlerBase
{
	/// Small internal structure that contains information on specific group of objects
	/// (creating separate entity is overcomplicating at least at this point)
	struct ObjectContainter
	{
		si32 id;
		std::string identifier;
		std::string name; // human-readable name
		std::string handlerName; // ID of handler that controls this object, should be determined using handlerConstructor map

		JsonNode base;
		std::map<si32, TObjectTypeHandler> subObjects;
		std::map<std::string, si32> subIds;//full id from core scope -> subtype

		SObjectSounds sounds;

		boost::optional<si32> groupDefaultAiValue;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & name;
			h & handlerName;
			h & base;
			h & subObjects;
			h & identifier;
			h & subIds;
			h & sounds;
			h & groupDefaultAiValue;
		}
	};

	/// list of object handlers, each of them handles only one type
	std::map<si32, ObjectContainter * > objects;

	/// map that is filled during contruction with all known handlers. Not serializeable due to usage of std::function
	std::map<std::string, std::function<TObjectTypeHandler()> > handlerConstructors;

	/// container with H3 templates, used only during loading, no need to serialize it
	typedef std::multimap<std::pair<si32, si32>, std::shared_ptr<const ObjectTemplate>> TTemplatesContainer;
	TTemplatesContainer legacyTemplates;

	/// contains list of custom names for H3 objects (e.g. Dwellings), used to load H3 data
	/// format: customNames[primaryID][secondaryID] -> name
	std::map<si32, std::vector<std::string>> customNames;

	void loadObjectEntry(const std::string & identifier, const JsonNode & entry, ObjectContainter * obj, bool isSubobject = false);
	ObjectContainter * loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name);

public:
	CObjectClassesHandler();
	~CObjectClassesHandler();

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void loadSubObject(const std::string & identifier, JsonNode config, si32 ID, boost::optional<si32> subID = boost::optional<si32>());
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

	std::string getObjectName(si32 type) const;
	std::string getObjectName(si32 type, si32 subtype) const;

	SObjectSounds getObjectSounds(si32 type) const;
	SObjectSounds getObjectSounds(si32 type, si32 subtype) const;

	/// Returns handler string describing the handler (for use in client)
	std::string getObjectHandlerName(si32 type) const;

	boost::optional<si32> getObjGroupAiValue(si32 primaryID) const; //default AI value of objects belonging to particular primaryID

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
	}
};

VCMI_LIB_NAMESPACE_END
