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
		h & usedTiles & allowedTerrains & animationFile;
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

	// TODO: as above, but also filters out templates that are not applicable according to accepted test
	// std::vector<ObjectTemplate> pickCandidates(Obj type, si32 subtype, ETerrainType terrain, std::function<bool(ObjectTemplate &)> accept) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
	}
};
