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

