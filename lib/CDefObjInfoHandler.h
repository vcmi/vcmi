#pragma once

#include "GameConstants.h"
#include "../lib/ConstTransitivePtr.h"

/*
 * CDefObjInfoHandler.h, part of VCMI engine
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

	std::vector<ObjectTemplate> templates;
protected:
	void init(si32 type, si32 subtype);

	/// loads templates from Json structure using fields "base" and "templates"
	void load(const JsonNode & input);

	virtual bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const;
public:
	void addTemplate(const ObjectTemplate & templ);

	/// returns all templates, without any filters
	std::vector<ObjectTemplate> getTemplates() const;

	/// returns all templates that can be placed on specific terrain type
	std::vector<ObjectTemplate> getTemplates(si32 terrainType) const;

	/// returns template suitable for object. If returned template is not equal to current one
	/// it must be replaced with this one (and properly updated on all clients)
	ObjectTemplate selectTemplate(si32 terrainType, CGObjectInstance * object) const;


	virtual CGObjectInstance * create(ObjectTemplate tmpl) const = 0;

	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const = 0;

	virtual const IObjectInfo * getObjectInfo(ObjectTemplate tmpl) const = 0;
};

typedef std::shared_ptr<AObjectTypeHandler> TObjectTypeHandler;

class CObjectTypesHandler
{
	/// list of object handlers, each of them handles only one type
	std::map<si32, std::map<si32, TObjectTypeHandler> > objectTypes;

public:
	void init();

	/// returns handler for specified object (ID-based). ObjectHandler keeps ownership
	TObjectTypeHandler getHandlerFor(si32 type, si32 subtype) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//h & objects;
		if (!h.saving)
			init(); // TODO: implement serialization
	}
};
