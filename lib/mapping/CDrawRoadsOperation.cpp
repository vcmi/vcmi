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

const std::vector<CDrawRoadsOperation::RoadPattern> CDrawRoadsOperation::patterns = 
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

static bool ruleIsAny(const std::string & rule)
{
	return rule == "?";
}

///CDrawRoadsOperation
CDrawRoadsOperation::CDrawRoadsOperation(CMap * map, const CTerrainSelection & terrainSel, ERoadType::ERoadType roadType, CRandomGenerator * gen):
	CMapOperation(map),terrainSel(terrainSel), roadType(roadType), gen(gen)
{
	
}

void CDrawRoadsOperation::execute()
{
	std::set<int3> invalidated;
	
	for(const auto & pos : terrainSel.getSelectedItems())
	{
		auto & tile = map->getTile(pos);
		tile.roadType = roadType;
		
		auto rect = extendTileAroundSafely(pos);
		rect.forEach([&invalidated](const int3 & pos)
		{
			invalidated.insert(pos);
		});
	}
	
	updateTiles(invalidated);	
}

void CDrawRoadsOperation::undo()
{
  //TODO	
}

void CDrawRoadsOperation::redo()
{
  //TODO	
}

std::string CDrawRoadsOperation::getLabel() const
{
	return "Draw Roads";
}

bool CDrawRoadsOperation::canApplyPattern(const RoadPattern & pattern) const
{
	//TODO: this method should be virtual for river support	
	return pattern.roadMapping.first >= 0;
}

void CDrawRoadsOperation::flipPattern(RoadPattern& pattern, int flip) const
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


bool CDrawRoadsOperation::needUpdateTile(const TerrainTile & tile) const
{
	return tile.roadType != ERoadType::NO_ROAD; //TODO: this method should be virtual for river support
}

void CDrawRoadsOperation::updateTiles(std::set<int3> & invalidated) 
{
	for(int3 coord : invalidated)
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
};

bool CDrawRoadsOperation::tileHasSomething(const int3& pos) const
{
//TODO: this method should be virtual for river support	

   return map->getTile(pos).roadType != ERoadType::NO_ROAD;	
}


void CDrawRoadsOperation::updateTile(TerrainTile & tile, const RoadPattern & pattern, const int flip)
{
  //TODO: this method should be virtual for river support	
  
	const std::pair<int, int> & mapping  = pattern.roadMapping;
  
	tile.roadDir = gen->nextInt(mapping.first, mapping.second);
	tile.extTileFlags = (tile.extTileFlags & 0xCF) | (flip << 4); 
}

CDrawRoadsOperation::ValidationResult CDrawRoadsOperation::validateTile(const RoadPattern & pattern, const int3 & pos)
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
		
		RoadPattern flipped = pattern;		
		
		flipPattern(flipped, flip);
		
		bool validated = true;
		
		for(int i = 0; i < 9; ++i)
		{
			if(4 == i)
				continue;
			int cx = pos.x + (i % 3) - 1;
			int cy = pos.y + (i / 3) - 1;
			
			int3 currentPos(cx, cy, pos.z);
			
			bool hasSomething;
			
			if(!map->isInTheMap(currentPos))
			{
				hasSomething = true; //road/river can go out of map
			}
			else
			{
				hasSomething = tileHasSomething(currentPos);				
			}
			
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

