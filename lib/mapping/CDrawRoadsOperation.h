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
#include "CMapEditManager.h"

struct TerrainTile;

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
		std::string data[9];
		std::pair<int, int> roadMapping, riverMapping;
		bool hasHFlip, hasVFlip;
	};
	
	struct ValidationResult
	{
		ValidationResult(bool result): result(result), flip(0){};
		bool result;
		int flip;
	};
	
	static const std::vector<RoadPattern> patterns;
	
	void flipPattern(RoadPattern & pattern, int flip) const;
	
	void updateTiles(std::set<int3> & invalidated);
	
	ValidationResult validateTile(const RoadPattern & pattern, const int3 & pos);
	void updateTile(TerrainTile & tile, const RoadPattern & pattern, const int flip);
	
	bool canApplyPattern(const RoadPattern & pattern) const;
	bool needUpdateTile(const TerrainTile & tile) const;
	bool tileHasSomething(const int3 & pos) const;
	
	CTerrainSelection terrainSel;
	ERoadType::ERoadType roadType;
	CRandomGenerator * gen;	
};
