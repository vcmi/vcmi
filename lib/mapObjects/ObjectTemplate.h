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

class CBinaryReader;
class CLegacyConfigParser;
class JsonNode;
class int3;

typedef ui8 TBlockMapBits;

enum class EBlockMapBits : TBlockMapBits
{
	EMPTY = 0, //default, with no effect
	VISIBLE = 1,
	VISITABLE = 2,
	BLOCKED = 4
};

inline EBlockMapBits& operator |=(EBlockMapBits& a, EBlockMapBits b) noexcept
{
	return a = static_cast<EBlockMapBits>(static_cast<TBlockMapBits>(a) | static_cast<TBlockMapBits>(b));
}

inline EBlockMapBits operator |(EBlockMapBits a, EBlockMapBits b) noexcept
{
	return static_cast<EBlockMapBits>(static_cast<TBlockMapBits>(a) | static_cast<TBlockMapBits>(b));
}

inline const bool& operator &(const EBlockMapBits& a, EBlockMapBits b) noexcept
{
	return (static_cast<TBlockMapBits>(a) & static_cast<TBlockMapBits>(b));
}

class DLL_LINKAGE ObjectTemplate
{
	enum usedTilesDimensions
	{
		TILES_WIDTH = 8,
		TILES_HEIGHT = 6
	};

	/// tiles that are covered by this object, uses EBlockMapBits enum as flags
	//std::vector<std::vector<ui8>> usedTiles;
	boost::multi_array <EBlockMapBits, 2> usedTiles; //[x][y]
	/// directions from which object can be entered, format same as for moveDir in CGHeroInstance(but 0 - 7)
	ui8 visitDir;
	/// list of terrains on which this object can be placed
	std::set<ETerrainType> allowedTerrains;

	//TODO: precalculate at init / construction
	ui32 width, height;
	int3 visitableOffset;
	int3 blockMapOffset;

	void afterLoadFixup();
	void precalculateOffsets();
	void calculateWidthHeight();
	void calculateVisitableOffset();
	void calculateBlockMapOffset();

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
	std::set<int3> getBlockedOffsets() const;
	int3 getBlockMapOffset() const; //bottom-right corner when firts blocked tile is

	// Checks if object is visitable from certain direction. X and Y must be between -1..+1
	bool isVisitableFrom(si8 X, si8 Y) const;
	int3 getVisitableOffset() const;
	bool isVisitableFromTop() const;

	// Checks if object can be placed on specific terrain
	bool canBePlacedAt(ETerrainType terrain) const;

	ObjectTemplate();

	void readTxt(CLegacyConfigParser & parser);
	void readMsk();
	void readMap(CBinaryReader & reader);
	void readJson(const JsonNode & node, const bool withTerrain = true);
	void writeJson(JsonNode & node, const bool withTerrain = true) const;

	bool operator==(const ObjectTemplate& ot) const
	{
		return (id == ot.id && subid == ot.subid);
	}

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		if (version >= 792)
		{
			h & width;
			h & height;
			h & visitableOffset;
			h & blockMapOffset;

			if (h.saving)
			{
				h & usedTiles;
			}
			else //load vector of vectors from old save to multi_array<2>
			{	
				setSize(width, height); //initialize multi_array<2>

				std::vector<std::vector<ui8>> oldTiles;
				oldTiles.resize(width);
				for (int i = 0; i < width; ++i)
				{
					oldTiles[i].resize(height);
					for (int j = 0; j < height; ++j)
					{
						h & oldTiles[i][j]; //deserialize vector of vectors
						usedTiles[i][j] = static_cast<EBlockMapBits>(oldTiles[i][j]); //copy to multi_array<2>
					}
				}
			}
		}
		else 
		{
			h & usedTiles; //old format
		}

		h & allowedTerrains;
		h & animationFile;
		h & stringID;
		h & id;
		h & subid;
		h & printPriority;
		h & visitDir;

		if(version >= 770)
		{
			h & editorAnimationFile;
		}
	}
};

