/*
 * CMapOperation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../int3.h"
#include "MapEditUtils.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CMap;
class CRandomGenerator;

/// The abstract base class CMapOperation defines an operation that can be executed, undone and redone.
class DLL_LINKAGE CMapOperation : public boost::noncopyable
{
public:
	explicit CMapOperation(CMap * map);
	virtual ~CMapOperation() = default;

	virtual void execute() = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual std::string getLabel() const = 0; /// Returns a operation display name.

	static const int FLIP_PATTERN_HORIZONTAL = 1;
	static const int FLIP_PATTERN_VERTICAL = 2;
	static const int FLIP_PATTERN_BOTH = 3;

protected:
	MapRect extendTileAround(const int3 & centerPos) const;
	MapRect extendTileAroundSafely(const int3 & centerPos) const; /// doesn't exceed map size

	CMap * map;
};

/// The CComposedOperation is an operation which consists of several operations.
class DLL_LINKAGE CComposedOperation : public CMapOperation
{
public:
	CComposedOperation(CMap * map);

	void execute() override;
	void undo() override;
	void redo() override;
	std::string getLabel() const override;

	void addOperation(std::unique_ptr<CMapOperation> && operation);

private:
	std::list<std::unique_ptr<CMapOperation> > operations;
};

/// The CDrawTerrainOperation class draws a terrain area on the map.
class CDrawTerrainOperation : public CMapOperation
{
public:
	CDrawTerrainOperation(CMap * map, const CTerrainSelection & terrainSel, Terrain terType, CRandomGenerator * gen);

	void execute() override;
	void undo() override;
	void redo() override;
	std::string getLabel() const override;

private:
	struct ValidationResult
	{
		ValidationResult(bool result, const std::string & transitionReplacement = "");

		bool result;
		/// The replacement of a T rule, either D or S.
		std::string transitionReplacement;
		int flip;
	};

	struct InvalidTiles
	{
		std::set<int3> foreignTiles, nativeTiles;
		bool centerPosValid;

		InvalidTiles() : centerPosValid(false) { }
	};

	void updateTerrainTypes();
	void invalidateTerrainViews(const int3 & centerPos);
	InvalidTiles getInvalidTiles(const int3 & centerPos) const;

	void updateTerrainViews();
	/// Validates the terrain view of the given position and with the given pattern. The first method wraps the
	/// second method to validate the terrain view with the given pattern in all four flip directions(horizontal, vertical).
	ValidationResult validateTerrainView(const int3 & pos, const std::vector<TerrainViewPattern> * pattern, int recDepth = 0) const;
	ValidationResult validateTerrainViewInner(const int3 & pos, const TerrainViewPattern & pattern, int recDepth = 0) const;

	CTerrainSelection terrainSel;
	Terrain terType;
	CRandomGenerator* gen;
	std::set<int3> invalidatedTerViews;
};

/// The CClearTerrainOperation clears+initializes the terrain.
class CClearTerrainOperation : public CComposedOperation
{
public:
	CClearTerrainOperation(CMap * map, CRandomGenerator * gen);

	std::string getLabel() const override;
};

/// The CInsertObjectOperation class inserts an object to the map.
class CInsertObjectOperation : public CMapOperation
{
public:
	CInsertObjectOperation(CMap * map, CGObjectInstance * obj);

	void execute() override;
	void undo() override;
	void redo() override;
	std::string getLabel() const override;

private:
	CGObjectInstance * obj;
};

/// The CMoveObjectOperation class moves object to another position
class CMoveObjectOperation : public CMapOperation
{
public:
	CMoveObjectOperation(CMap * map, CGObjectInstance * obj, const int3 & targetPosition);

	void execute() override;
	void undo() override;
	void redo() override;
	std::string getLabel() const override;

private:
	CGObjectInstance * obj;
	int3 initialPos;
	int3 targetPos;
};

/// The CRemoveObjectOperation class removes object from the map
class CRemoveObjectOperation : public CMapOperation
{
public:
	CRemoveObjectOperation(CMap* map, CGObjectInstance * obj);
	~CRemoveObjectOperation();

	void execute() override;
	void undo() override;
	void redo() override;
	std::string getLabel() const override;

private:
	CGObjectInstance* obj;
};

VCMI_LIB_NAMESPACE_END
