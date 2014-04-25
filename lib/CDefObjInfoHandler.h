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

class DLL_LINKAGE CDefObjInfoHandler
{
	/// list of all object templates loaded from text files
	/// actual object have ObjectTemplate as member "appearance"
	std::vector<ObjectTemplate> objects;

	/// reads one of H3 text files that contain object templates description
	void readTextFile(std::string path);
public:

	CDefObjInfoHandler();

	/// Erases all templates with given type/subtype
	void eraseAll(Obj type, si32 subtype);

	/// Add new template into the list
	void registerTemplate(ObjectTemplate obj);

	/// picks all possible candidates for specific pair <type, subtype>
	std::vector<ObjectTemplate> pickCandidates(Obj type, si32 subtype) const;
	/// picks all candidates for <type, subtype> and of possible - also filters them by terrain
	std::vector<ObjectTemplate> pickCandidates(Obj type, si32 subtype, ETerrainType terrain) const;
	/// as above, but also filters out templates that are not applicable according to accepted test
	std::vector<ObjectTemplate> pickCandidates(Obj type, si32 subtype, ETerrainType terrain, std::function<bool(ObjectTemplate &)> filter) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
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

class IObjectTypeHandler
{
public:
	virtual CGObjectInstance * create(ui32 id, ui32 subID) const = 0;

	virtual bool handlesID(ui32 id) const = 0;

	virtual void configureObject(CGObjectInstance * object) const = 0;

	virtual IObjectInfo * getObjectInfo(ui32 id, ui32 subID) const = 0;
};

typedef std::shared_ptr<IObjectTypeHandler> TObjectTypeHandler;

class CObjectTypesHandler
{
	/// list of object handlers, each of them handles 1 or more object type
	std::vector<TObjectTypeHandler> objectTypes;

public:
	/// returns handler for specified object (ID-based). ObjectHandler keeps ownership
	IObjectTypeHandler * getHandlerFor(CObjectTemplate tmpl) const;

	/// creates object based on specified template
	CGObjectInstance * createObject(CObjectTemplate tmpl);

	template<typename CObjectClass>
	CObjectClass * createObjectTyped(CObjectTemplate tmpl)
	{
		auto objInst  = createObject(tmpl);
		auto objClass = dynamic_cast<CObjectClass*>(objInst);
		assert(objClass);
		return objClass;
	}
}
