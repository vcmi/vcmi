/*
 * ObjectTemplate.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"
#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBinaryReader;
class CLegacyConfigParser;
class JsonNode;
class int3;

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
	std::set<TerrainId> allowedTerrains;

	/// or, allow placing object on any terrain
	bool anyLandTerrain;

	void afterLoadFixup();

public:
	/// H3 ID/subID of this object
	Obj id;
	si32 subid;
	/// print priority, objects with higher priority will be print first, below everything else
	si32 printPriority;
	/// animation file that should be used to display object
	std::string animationFile;

	/// map editor only animation file
	std::string editorAnimationFile;

	/// string ID, equals to def base name for h3m files (lower case, no extension) or specified in mod data
	std::string stringID;

	inline ui32 getWidth() const
	{
		return width;
	};

	inline ui32 getHeight() const
	{ 
		return height;
	};

	void setSize(ui32 width, ui32 height);

	inline bool isVisitable() const
	{
		return visitable;
	};

	// Checks object used tiles
	// Position is relative to bottom-right corner of the object, can not be negative
	bool isWithin(si32 X, si32 Y) const;
	bool isVisitableAt(si32 X, si32 Y) const;
	bool isVisibleAt(si32 X, si32 Y) const;
	bool isBlockedAt(si32 X, si32 Y) const;

	inline std::set<int3> getBlockedOffsets() const
	{
		return blockedOffsets;
	};

	inline int3 getBlockMapOffset() const
	{
		return blockMapOffset; 
	}

	inline int3 getTopVisibleOffset() const
	{
		return topVisibleOffset;
	}

	// Checks if object is visitable from certain direction. X and Y must be between -1..+1
	bool isVisitableFrom(si8 X, si8 Y) const;
	inline int3 getVisitableOffset() const
	{
		//logGlobal->warn("Warning: getVisitableOffset called on non-visitable obj!");
		return visitableOffset;
	};

	inline bool isVisitableFromTop() const
	{
		return visitDir & 2;
	};

	inline bool canBePlacedAtAnyTerrain() const
	{
		return anyLandTerrain;
	};

	const std::set<TerrainId>& getAllowedTerrains() const
	{
		return allowedTerrains;
	}

	// Checks if object can be placed on specific terrain
	bool canBePlacedAt(TerrainId terrain) const;

	ObjectTemplate();
	//custom copy constructor is required
	ObjectTemplate(const ObjectTemplate & other);

	ObjectTemplate& operator=(const ObjectTemplate & rhs);

	void readTxt(const std::string & parser);
	void readMsk();
	void readMap(CBinaryReader & reader);
	void readJson(const JsonNode & node, bool withTerrain = true);
	void writeJson(JsonNode & node, bool withTerrain = true) const;

	bool operator==(const ObjectTemplate& ot) const { return (id == ot.id && subid == ot.subid); }

private:
	ui32 width;
	ui32 height;
	bool visitable;

	std::set<int3> blockedOffsets;
	int3 blockMapOffset;
	int3 visitableOffset;
	int3 topVisibleOffset;

	void recalculate();

	void calculateWidth();
	void calculateHeight();
	void calculateVisitable();
	void calculateBlockedOffsets();
	void calculateBlockMapOffset();
	void calculateVisitableOffset();
	void calculateTopVisibleOffset();

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & usedTiles;
		h & allowedTerrains;
		h & anyLandTerrain;
		h & animationFile;
		h & stringID;
		h & id;
		h & subid;
		h & printPriority;
		h & visitDir;
		h & editorAnimationFile;

		if (!h.saving)
		{
			recalculate();
		}
	}
};


VCMI_LIB_NAMESPACE_END
