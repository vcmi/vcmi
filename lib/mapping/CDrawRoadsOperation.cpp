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

const std::vector<CDrawRoadsOperation::RoadPattern> CDrawRoadsOperation::rules = 
{
	//single tile. fallback patern
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
        false,
        true
	},
	//Dead end vertical
	{
		{
          "?","-","?",
          "-","+","-",
          "?","+","?" 
        },
        {14,14},{9,10},
        true,
        false
	},
	//T-cross horizontal
	{
		{
          "?","+","?",
          "-","+","+",
          "?","+","?"  
        },
        {6,7},{7,8},
        false,
        true
	},
	//T-cross vertical
	{
		{
          "?","-","?",
          "+","+","+",
          "?","+","?" 
        },
        {8,9},{5,6},
        true,
        false
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

///CDrawRoadsOperation
CDrawRoadsOperation::CDrawRoadsOperation(CMap * map, const CTerrainSelection & terrainSel, ERoadType::ERoadType roadType, CRandomGenerator * gen):
	CMapOperation(map),terrainSel(terrainSel), roadType(roadType), gen(gen)
{
	
}

void CDrawRoadsOperation::execute()
{
	
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

