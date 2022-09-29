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

VCMI_LIB_NAMESPACE_BEGIN

struct TerrainTile;

class CDrawLinesOperation : public CMapOperation
{
public:
	void execute() override;
	void undo() override;
	void redo() override;
	
protected:
	
	struct LinePattern
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
	
	CDrawLinesOperation(CMap * map, const CTerrainSelection & terrainSel, CRandomGenerator * gen);
	
	virtual void executeTile(TerrainTile & tile) = 0;
	virtual bool canApplyPattern(const CDrawLinesOperation::LinePattern & pattern) const = 0;
	virtual bool needUpdateTile(const TerrainTile & tile) const = 0;
	virtual bool tileHasSomething(const int3 & pos) const = 0;
	virtual void updateTile(TerrainTile & tile, const CDrawLinesOperation::LinePattern & pattern, const int flip) = 0;
	
	static const std::vector<LinePattern> patterns;
	
	void flipPattern(LinePattern & pattern, int flip) const;
	
	void updateTiles(std::set<int3> & invalidated);
	
	ValidationResult validateTile(const LinePattern & pattern, const int3 & pos);
	
	CTerrainSelection terrainSel;
	CRandomGenerator * gen;
};

class CDrawRoadsOperation : public CDrawLinesOperation
{
public:
	CDrawRoadsOperation(CMap * map, const CTerrainSelection & terrainSel, RoadId roadType, CRandomGenerator * gen);
	std::string getLabel() const override;
	
protected:
	void executeTile(TerrainTile & tile) override;
	bool canApplyPattern(const CDrawLinesOperation::LinePattern & pattern) const override;
	bool needUpdateTile(const TerrainTile & tile) const override;
	bool tileHasSomething(const int3 & pos) const override;
	void updateTile(TerrainTile & tile, const CDrawLinesOperation::LinePattern & pattern, const int flip) override;
	
private:
	RoadId roadType;
};

class CDrawRiversOperation : public CDrawLinesOperation
{
public:
	CDrawRiversOperation(CMap * map, const CTerrainSelection & terrainSel, RoadId roadType, CRandomGenerator * gen);
	std::string getLabel() const override;
	
protected:
	void executeTile(TerrainTile & tile) override;
	bool canApplyPattern(const CDrawLinesOperation::LinePattern & pattern) const override;
	bool needUpdateTile(const TerrainTile & tile) const override;
	bool tileHasSomething(const int3 & pos) const override;
	void updateTile(TerrainTile & tile, const CDrawLinesOperation::LinePattern & pattern, const int flip) override;
	
private:
	RiverId riverType;
};

VCMI_LIB_NAMESPACE_END
