/*
 * CDrawRoadsOperation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#pragma once
 
#include "../CRandomGenerator.h"
#include "CMap.h"
#include "CMapEditManager.h"


class CDrawRoadsOperation : public CMapOperation
{
public:
	CDrawRoadsOperation(CMap * map, const CTerrainSelection & terrainSel, ERoadType::ERoadType roadType, CRandomGenerator * gen);
	void execute() override;
	void undo() override;
	void redo() override;
	std::string getLabel() const override;	
private:
	
	struct RoadPattern
	{
		std::string rules[9];
		std::pair<int, int> roadMapping, riverMapping;
		bool hasHFlip, hasBFlip;
	};
	
	static const  std::vector<RoadPattern> rules;
	
	CTerrainSelection terrainSel;
	ERoadType::ERoadType roadType;
	CRandomGenerator * gen;	
};
