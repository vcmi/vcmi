#pragma once

#include "GameConstants.h"
#include "../lib/ConstTransitivePtr.h"
#include "IHandlerBase.h"

/*
 * CObjectClassesHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CBinaryReader;
class CLegacyConfigParser;
class JsonNode;
class CRandomGenerator;

class DLL_LINKAGE ObjectTemplate
{
	enum EBlockMapBits
	{
		VISIBLE = 1,
		VISITABLE = 2,
		BLOCKED = 4
	};

	/// tiles that are covered by this object, uses EBlockMapBits enum as flags
	std::vector<std::vector<ui8>> usedTiles;
	/// directions from which object can be entered, format same as for moveDir in CGHeroInstance(but 0 - 7)
	ui8 visitDir;
	/// list of terrains on which this object can be placed
	std::set<ETerrainType> allowedTerrains;

public:
	/// H3 ID/subID of this object
	Obj id;
	si32 subid;
	/// print priority, objects with higher priority will be print first, below everything else
	si32 printPriority;
	/// animation file that should be used to display object
	std::string animationFile;

	/// string ID, equals to def base name for h3m files (lower case, no extension) or specified in mod data
	std::string stringID;

	ui32 getWidth() const;
	ui32 getHeight() const;
	void setSize(ui32 width, ui32 height);

	bool isVisitable() const;

	// Checks object used tiles
	// Position is relative to bottom-right corner of the object, can not be negative
	bool isWithin(si32 X, si32 Y) const;
	bool isVisitableAt(si32 X, si32 Y) const;
	bool isVisibleAt(si32 X, si32 Y) const;
	bool isBlockedAt(si32 X, si32 Y) const;

	// Checks if object is visitable from certain direction. X and Y must be between -1..+1
	bool isVisitableFrom(si8 X, si8 Y) const;

	// Checks if object can be placed on specific terrain
	bool canBePlacedAt(ETerrainType terrain) const;

	ObjectTemplate();

	void readTxt(CLegacyConfigParser & parser);
	void readMsk();
	void readMap(CBinaryReader & reader);
	void readJson(const JsonNode & node);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & usedTiles & allowedTerrains & animationFile & stringID;
		h & id & subid & printPriority & visitDir;
	}
};

class IObjectInfo
{
public:
	virtual bool givesResources() const = 0;

	virtual bool givesExperience() const = 0;
	virtual bool givesMana() const = 0;
	virtual bool givesMovement() const = 0;

	virtual bool givesPrimarySkills() const = 0;
	virtual bool givesSecondarySkills() const = 0;

	virtual bool givesArtifacts() const = 0;
	virtual bool givesCreatures() const = 0;
	virtual bool givesSpells() const = 0;

	virtual bool givesBonuses() const = 0;
};

class CGObjectInstance;

class AObjectTypeHandler
{
	si32 type;
	si32 subtype;

	JsonNode base; /// describes base template

	std::vector<ObjectTemplate> templates;
protected:
	void setType(si32 type, si32 subtype);

	virtual bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const;
public:
	/// returns true if type is not configurable and new objects can be created without valid config
	virtual bool confFree();

	/// loads templates from Json structure using fields "base" and "templates"
	virtual void init(const JsonNode & input);

	void addTemplate(const ObjectTemplate & templ);

	/// returns all templates, without any filters
	std::vector<ObjectTemplate> getTemplates() const;

	/// returns all templates that can be placed on specific terrain type
	std::vector<ObjectTemplate> getTemplates(si32 terrainType) const;

	/// returns preferred template for this object, if present (e.g. one of 3 possible templates for town - village, fort and castle)
	/// note that appearance will not be changed - this must be done separately (either by assignment or via pack from server)
	boost::optional<ObjectTemplate> getOverride(si32 terrainType, const CGObjectInstance * object) const;

	/// Creates object and set up core properties (like ID/subID). Object is NOT initialized
	/// to allow creating objects before game start (e.g. map loading)
	virtual CGObjectInstance * create(ObjectTemplate tmpl) const = 0;

	/// Configures object properties. Should be re-entrable, resetting state of the object if necessarily
	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const = 0;

	/// Returns object configuration, if available. Othervice returns NULL
	virtual const IObjectInfo * getObjectInfo(ObjectTemplate tmpl) const = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & subtype & templates;
	}
};

/// Class that is used for objects that do not have dedicated handler
template<class ObjectType>
class CDefaultObjectTypeHandler : public AObjectTypeHandler
{
	CGObjectInstance * create(ObjectTemplate tmpl) const
	{
		auto obj = new ObjectType();
		obj->ID = tmpl.id;
		obj->subID = tmpl.subid;
		obj->appearance = tmpl;
		return obj;
	}

	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
	{
	}

	virtual const IObjectInfo * getObjectInfo(ObjectTemplate tmpl) const
	{
		return nullptr;
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

		std::string name; // human-readable name
		std::string handlerName; // ID of handler that controls this object, shoul be determined using hadlerConstructor map

		JsonNode base;
		std::map<si32, TObjectTypeHandler> objects;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & base & objects;
		}
	};

	/// list of object handlers, each of them handles only one type
	std::vector<ObjectContainter * > objects;

	/// map that is filled during contruction with all known handlers. Not serializeable
	std::map<std::string, std::function<TObjectTypeHandler()> > handlerConstructors;

	ObjectContainter * loadFromJson(const JsonNode & json);
public:
	CObjectClassesHandler();

	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize);

	virtual void loadObject(std::string scope, std::string name, const JsonNode & data);
	virtual void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index);

	virtual void afterLoadFinalization(){};

	virtual std::vector<bool> getDefaultAllowed() const;

	/// returns handler for specified object (ID-based). ObjectHandler keeps ownership
	TObjectTypeHandler getHandlerFor(si32 type, si32 subtype) const;

	std::string getObjectName(si32 type) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
	}
};
