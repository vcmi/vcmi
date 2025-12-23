/*
 * CDrawRoadsOperation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CDrawRoadsOperation.h"
#include "CMap.h"

#include "../RoadHandler.h"
#include "../RiverHandler.h"
#include "../GameLibrary.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

template <typename T>
const std::vector<typename CDrawLinesOperation<T>::LinePattern> CDrawLinesOperation<T>::patterns =
{
	//single tile. fall-back pattern
	{
		{
          "-","-","-",
          "-","+","-",
          "-","-","-"
        },
        {14,14},
        {9,9},
        false,
        false
	},
	//Road straight with angle
	{
		{
          "?","-","+",
          "-","+","+",
          "+","+","?"
        },
        {2,5},
        {-1,-1},
        true,
        true
	},
	//Turn
	{
		{
          "?","-","?",
          "-","+","+",
          "?","+","?"
        },
        {0,1},
        {0,3},
        true,
        true
	},
	//Dead end horizontal
	{
		{
          "?","-","?",
          "-","+","+",
          "?","-","?"
        },
        {15,15},{11,12},
        true,
        false
	},
	//Dead end vertical
	{
		{
          "?","-","?",
          "-","+","-",
          "?","+","?"
        },
        {14,14},{9,10},
        false,
        true
	},
	//T-cross horizontal
	{
		{
          "?","+","?",
          "-","+","+",
          "?","+","?"
        },
        {6,7},{7,8},
        true,
        false
	},
	//T-cross vertical
	{
		{
          "?","-","?",
          "+","+","+",
          "?","+","?"
        },
        {8,9},{5,6},
        false,
        true
	},
	//Straight Horizontal
	{
		{
          "?","-","?",
          "+","+","+",
          "?","-","?"
        },
        {12,13},{11,12},
        false,
        false
	},
	//Straight Vertical
	{
		{
          "?","+","?",
          "-","+","-",
          "?","+","?"
        },
        {10,11},{9,10},
        false,
        false
	},
	//X-cross
	{
		{
          "?","+","?",
          "+","+","+",
          "?","+","?"
        },
        {16,16},{4,4},
        false,
        false
	}

};

static bool ruleIsNone(const std::string & rule)
{
	return rule == "-";
}

static bool ruleIsSomething(const std::string & rule)
{
	return rule == "+";
}

#ifndef NDEBUG
static bool ruleIsAny(const std::string & rule)
{
	return rule == "?";
}
#endif

///CDrawLinesOperation
template <typename T>
CDrawLinesOperation<T>::CDrawLinesOperation(CMap * map, CTerrainSelection terrainSel, T lineType, vstd::RNG * gen):
	CMapOperation(map),
	terrainSel(std::move(terrainSel)),
	lineType(lineType),
	gen(gen)
{
}

///CDrawRoadsOperation
CDrawRoadsOperation::CDrawRoadsOperation(CMap * map, const CTerrainSelection & terrainSel, RoadId roadType, vstd::RNG * gen):
	CDrawLinesOperation(map, terrainSel,roadType, gen)
{}

///CDrawRiversOperation
CDrawRiversOperation::CDrawRiversOperation(CMap * map, const CTerrainSelection & terrainSel, RiverId riverType, vstd::RNG * gen):
	CDrawLinesOperation(map, terrainSel, riverType, gen)
{}

template <typename T>
void CDrawLinesOperation<T>::execute()
{
	for(const auto & pos : terrainSel.getSelectedItems())
	{
		auto identifier = getIdentifier(map->getTile(pos));
		if (formerState.find(identifier) == formerState.end())
			formerState.insert({identifier, CTerrainSelection(terrainSel.getMap())});
		formerState.at(identifier).select(pos);
	}

	drawLines(terrainSel, lineType);
}

template <typename T>
void CDrawLinesOperation<T>::drawLines(CTerrainSelection selection, T type)
{
	std::set<int3> invalidated;

	for(const auto & pos : selection.getSelectedItems())
	{
		executeTile(map->getTile(pos), type);

		auto rect = extendTileAroundSafely(pos);
		rect.forEach([&invalidated](const int3 & pos)
		{
			invalidated.insert(pos);
		});
	}

	updateTiles(invalidated);
}

template <typename T>
void CDrawLinesOperation<T>::undo()
{
	for (auto const& typeToSelection : formerState)
	{
		drawLines(typeToSelection.second, typeToSelection.first);
	}
}

template <typename T>
void CDrawLinesOperation<T>::redo()
{
  drawLines(terrainSel, lineType);
}

template <typename T>
void CDrawLinesOperation<T>::flipPattern(LinePattern& pattern, int flip) const
{
	//todo: use cashing here and also in terrain patterns

	if(flip == 0)
	{
		return;
	}


	if(flip == FLIP_PATTERN_HORIZONTAL || flip == FLIP_PATTERN_BOTH)
	{
		for(int i = 0; i < 3; ++i)
		{
			int y = i * 3;
			std::swap(pattern.data[y], pattern.data[y + 2]);
		}
	}

	if(flip == FLIP_PATTERN_VERTICAL || flip == FLIP_PATTERN_BOTH)
	{
		for(int i = 0; i < 3; ++i)
		{
			std::swap(pattern.data[i], pattern.data[6 + i]);
		}
	}
}

template <typename T>
void CDrawLinesOperation<T>::updateTiles(std::set<int3> & invalidated)
{
	for(const int3 & coord : invalidated)
	{
		TerrainTile & tile = map->getTile(coord);
		ValidationResult result(false);

		if(!needUpdateTile(tile))
			continue;

		int bestPattern = -1;

		for(int k = 0; k < patterns.size(); ++k)
		{
			result = validateTile(patterns[k], coord);

			if(result.result)
			{
				bestPattern = k;
				break;
			}
		}

		if(bestPattern != -1)
		{
			updateTile(tile, patterns[bestPattern], result.flip);
		}

	}
}

template <typename T>
typename CDrawLinesOperation<T>::ValidationResult CDrawLinesOperation<T>::validateTile(const LinePattern & pattern, const int3 & pos)
{
	ValidationResult result(false);

	if(!canApplyPattern(pattern))
		return result;


	for(int flip = 0; flip < 4; ++flip)
	{
		if((flip == FLIP_PATTERN_BOTH) && !(pattern.hasHFlip && pattern.hasVFlip))
			continue;
		if((flip == FLIP_PATTERN_HORIZONTAL) && !pattern.hasHFlip)
			continue;
		if((flip == FLIP_PATTERN_VERTICAL) && !(pattern.hasVFlip))
			continue;

		LinePattern flipped = pattern;

		flipPattern(flipped, flip);

		bool validated = true;

		for(int i = 0; i < 9; ++i)
		{
			if(4 == i)
				continue;
			int cx = pos.x + (i % 3) - 1;
			int cy = pos.y + (i / 3) - 1;

			int3 currentPos(cx, cy, pos.z);

			bool hasSomething = map->isInTheMap(currentPos) && tileHasSomething(currentPos);

			if(ruleIsSomething(flipped.data[i]))
			{
				if(!hasSomething)
				{
					validated = false;
					break;
				}
			}
			else if(ruleIsNone(flipped.data[i]))
			{
				if(hasSomething)
				{
					validated = false;
					break;
				}
			}
			else
			{
				assert(ruleIsAny(flipped.data[i]));
			}

		}

		if(validated)
		{
			result.result = true;
			result.flip = flip;
			return result;
		}
	}

	return result;
}

std::string CDrawRoadsOperation::getLabel() const
{
	return "Draw Roads";
}

std::string CDrawRiversOperation::getLabel() const
{
	return "Draw Rivers";
}

void CDrawRoadsOperation::executeTile(TerrainTile & tile, RoadId type)
{
	tile.roadType = type;
}

void CDrawRiversOperation::executeTile(TerrainTile & tile, RiverId type)
{
	tile.riverType = type;
}

bool CDrawRoadsOperation::canApplyPattern(const LinePattern & pattern) const
{
	return pattern.roadMapping.first >= 0;
}

bool CDrawRiversOperation::canApplyPattern(const LinePattern & pattern) const
{
	return pattern.riverMapping.first >= 0;
}

bool CDrawRoadsOperation::needUpdateTile(const TerrainTile & tile) const
{
	return tile.hasRoad();
}

bool CDrawRiversOperation::needUpdateTile(const TerrainTile & tile) const
{
	return tile.hasRiver();
}

bool CDrawRoadsOperation::tileHasSomething(const int3& pos) const
{
	return map->getTile(pos).hasRoad();
}

bool CDrawRiversOperation::tileHasSomething(const int3& pos) const
{
	return map->getTile(pos).hasRiver();
}

void CDrawRoadsOperation::updateTile(TerrainTile & tile, const LinePattern & pattern, const int flip)
{
	const std::pair<int, int> & mapping  = pattern.roadMapping;

	tile.roadDir = gen->nextInt(mapping.first, mapping.second);
	tile.extTileFlags = (tile.extTileFlags & 0b11001111) | (flip << 4);
}

void CDrawRiversOperation::updateTile(TerrainTile & tile, const LinePattern & pattern, const int flip)
{
	const std::pair<int, int> & mapping  = pattern.riverMapping;
	
	tile.riverDir = gen->nextInt(mapping.first, mapping.second);
	tile.extTileFlags = (tile.extTileFlags & 0b00111111) | (flip << 2);
}

RiverId CDrawRiversOperation::getIdentifier(TerrainTile & tile) const
{
	return tile.riverType;
}

RoadId CDrawRoadsOperation::getIdentifier(TerrainTile & tile) const
{
	return tile.roadType;
}

VCMI_LIB_NAMESPACE_END
