
/*
 * CMapEditManager.h, part of VCMI engine
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

class CGObjectInstance;
class CTerrainViewPatternConfig;
struct TerrainViewPattern;

/// Represents a map rectangle.
struct DLL_LINKAGE MapRect
{
	MapRect();
	MapRect(int3 pos, si32 width, si32 height);
	si32 x, y, z;
	si32 width, height;

	si32 left() const;
	si32 right() const;
	si32 top() const;
	si32 bottom() const;

	int3 topLeft() const; /// Top left corner of this rect.
	int3 topRight() const; /// Top right corner of this rect.
	int3 bottomLeft() const; /// Bottom left corner of this rect.
	int3 bottomRight() const; /// Bottom right corner of this rect.

	/// Returns a MapRect of the intersection of this rectangle and the given one.
	MapRect operator&(const MapRect & rect) const;
};

/// The abstract base class CMapOperation defines an operation that can be executed, undone and redone.
class DLL_LINKAGE CMapOperation : public boost::noncopyable
{
public:
	CMapOperation(CMap * map);
	virtual ~CMapOperation() { };

	virtual void execute() = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual std::string getLabel() const; /// Returns a display-able name of the operation.

protected:
	CMap * map;
};

/// The CMapUndoManager provides the functionality to save operations and undo/redo them.
class DLL_LINKAGE CMapUndoManager : boost::noncopyable
{
public:
	CMapUndoManager();

	void undo();
	void redo();
	void clearAll();

	/// The undo redo limit is a number which says how many undo/redo items can be saved. The default
	/// value is 10. If the value is 0, no undo/redo history will be maintained.
	int getUndoRedoLimit() const;
	void setUndoRedoLimit(int value);

	const CMapOperation * peekRedo() const;
	const CMapOperation * peekUndo() const;

	void addOperation(unique_ptr<CMapOperation> && operation); /// Client code does not need to call this method.

private:
	typedef std::list<unique_ptr<CMapOperation> > TStack;

	void doOperation(TStack & fromStack, TStack & toStack, bool doUndo);
	const CMapOperation * peek(const TStack & stack) const;

	TStack undoStack;
	TStack redoStack;
	int undoRedoLimit;
};

/// The map edit manager provides functionality for drawing terrain and placing
/// objects on the map.
class DLL_LINKAGE CMapEditManager : boost::noncopyable
{
public:
	CMapEditManager(CMap * map);

	/// Clears the terrain. The free level is filled with water and the underground level with rock.
	void clearTerrain(CRandomGenerator * gen);

	void drawTerrain(const MapRect & rect, ETerrainType terType, CRandomGenerator * gen);
	void insertObject(const int3 & pos, CGObjectInstance * obj);

	CMapUndoManager & getUndoManager();
	void undo();
	void redo();

private:
	void execute(unique_ptr<CMapOperation> && operation);

	CMap * map;
	CMapUndoManager undoManager;
};

/* ---------------------------------------------------------------------------- */
/* Implementation/Detail classes, Private API */
/* ---------------------------------------------------------------------------- */

namespace ETerrainGroup
{
	enum ETerrainGroup
	{
		NORMAL,
		DIRT,
		SAND,
		WATER,
		ROCK
	};
}

/// The terrain view pattern describes a specific composition of terrain tiles
/// in a 3x3 matrix and notes which terrain view frame numbers can be used.
struct DLL_LINKAGE TerrainViewPattern
{
	struct WeightedRule
	{
		WeightedRule();
		/// Gets true if this rule is a standard rule which means that it has a value of one of the RULE_* constants.
		bool isStandardRule() const;

		/// The name of the rule. Can be any value of the RULE_* constants or a ID of a another pattern.
		std::string name;
		/// Optional. A rule can have points. Patterns may have a minimum count of points to reach to be successful.
		int points;
	};

	/// Constant for the flip mode different images. Pattern will be flipped and different images will be used(mapping area is divided into 4 parts)
	static const std::string FLIP_MODE_DIFF_IMAGES;
	/// Constant for the rule dirt, meaning a dirty border is required.
	static const std::string RULE_DIRT;
	/// Constant for the rule sand, meaning a sandy border is required.
	static const std::string RULE_SAND;
	/// Constant for the rule transition, meaning a dirty OR sandy border is required.
	static const std::string RULE_TRANSITION;
	/// Constant for the rule native, meaning a native type is required.
	static const std::string RULE_NATIVE;
	/// Constant for the rule any, meaning a native type, dirty OR sandy border is required.
	static const std::string RULE_ANY;

	TerrainViewPattern();

	/// The pattern data can be visualized as a 3x3 matrix:
	/// [ ][ ][ ]
	/// [ ][ ][ ]
	/// [ ][ ][ ]
	///
	/// The box in the center belongs always to the native terrain type and
	/// is the point of origin. Depending on the terrain type different rules
	/// can be used. Their meaning differs also from type to type.
	///
	/// std::vector -> several rules can be used in one cell
	std::array<std::vector<WeightedRule>, 9> data;

	/// The identifier of the pattern, if it's referenced from a another pattern.
	std::string id;

	/// This describes the mapping between this pattern and the corresponding range of frames
	/// which should be used for the ter view.
	///
	/// std::vector -> size=1: typical, size=2: if this pattern should map to two different types of borders
	/// std::pair   -> 1st value: lower range, 2nd value: upper range
	std::vector<std::pair<int, int> > mapping;
	/// If diffImages is true, different images/frames are used to place a rotated terrain view. If it's false
	/// the same frame will be used and rotated.
	bool diffImages;
	/// The rotationTypesCount is only used if diffImages is true and holds the number how many rotation types(horizontal, etc...)
	/// are supported.
	int rotationTypesCount;

	/// The minimum and maximum points to reach to validate the pattern successfully.
	int minPoints, maxPoints;

	ETerrainGroup::ETerrainGroup terGroup;
};

/// The terrain view pattern config loads pattern data from the filesystem.
class DLL_LINKAGE CTerrainViewPatternConfig : public boost::noncopyable
{
public:
	static CTerrainViewPatternConfig & get();

	const std::vector<TerrainViewPattern> & getPatternsForGroup(ETerrainGroup::ETerrainGroup terGroup) const;
	boost::optional<const TerrainViewPattern &> getPatternById(ETerrainGroup::ETerrainGroup terGroup, const std::string & id) const;
	ETerrainGroup::ETerrainGroup getTerrainGroup(const std::string & terGroup) const;

private:
	CTerrainViewPatternConfig();
	~CTerrainViewPatternConfig();

	std::map<ETerrainGroup::ETerrainGroup, std::vector<TerrainViewPattern> > patterns;
	static boost::mutex smx;
};

/// The CDrawTerrainOperation class draws a terrain area on the map.
class CDrawTerrainOperation : public CMapOperation
{
public:
	CDrawTerrainOperation(CMap * map, const MapRect & rect, ETerrainType terType, CRandomGenerator * gen);

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

	void updateTerrainViews(const MapRect & rect);
	ETerrainGroup::ETerrainGroup getTerrainGroup(ETerrainType terType) const;
	/// Validates the terrain view of the given position and with the given pattern. The first method wraps the
	/// second method to validate the terrain view with the given pattern in all four flip directions(horizontal, vertical).
	ValidationResult validateTerrainView(const int3 & pos, const TerrainViewPattern & pattern, int recDepth = 0) const;
	ValidationResult validateTerrainViewInner(const int3 & pos, const TerrainViewPattern & pattern, int recDepth = 0) const;
	/// Tests whether the given terrain type is a sand type. Sand types are: Water, Sand and Rock
	bool isSandType(ETerrainType terType) const;
	TerrainViewPattern getFlippedPattern(const TerrainViewPattern & pattern, int flip) const;

	static const int FLIP_PATTERN_HORIZONTAL = 1;
	static const int FLIP_PATTERN_VERTICAL = 2;
	static const int FLIP_PATTERN_BOTH = 3;

	MapRect rect;
	ETerrainType terType;
	CRandomGenerator * gen;
};

/// The CInsertObjectOperation class inserts an object to the map.
class CInsertObjectOperation : public CMapOperation
{
public:
	CInsertObjectOperation(CMap * map, const int3 & pos, CGObjectInstance * obj);

	void execute() override;
	void undo() override;
	void redo() override;
	std::string getLabel() const override;

private:
	int3 pos;
	CGObjectInstance * obj;
};
