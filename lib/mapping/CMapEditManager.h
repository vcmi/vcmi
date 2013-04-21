
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

/// Represents a map rectangle.
struct DLL_LINKAGE MapRect
{
	MapRect(int3 pos, si32 width, si32 height) : pos(pos), width(width), height(height) { };
	int3 pos;
	si32 width, height;
};

/// The abstract base class CMapOperation defines an operation that can be executed, undone and redone.
class CMapOperation
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
class CMapUndoManager : boost::noncopyable
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

	inline void doOperation(TStack & fromStack, TStack & toStack, bool doUndo);
	inline const CMapOperation * peek(const TStack & stack) const;

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

/// The terrain view pattern describes a specific composition of terrain tiles
/// in a 3x3 matrix and notes which terrain view frame numbers can be used.
struct TerrainViewPattern
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

	/// Constant for the flip mode same image. Pattern will be flipped and the same image will be used(which is given in the mapping).
	static const std::string FLIP_MODE_SAME_IMAGE;

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

	/// The minimum points to reach to to validate the pattern successfully.
	int minPoints;

	/// Describes if flipping is required and which mapping should be used.
	std::string flipMode;

	ETerrainGroup::ETerrainGroup terGroup;
};

/// The terrain view pattern config loads pattern data from the filesystem.
class CTerrainViewPatternConfig : public boost::noncopyable
{
public:
	static CTerrainViewPatternConfig & get();

	const std::vector<TerrainViewPattern> & getPatternsForGroup(ETerrainGroup::ETerrainGroup terGroup) const;
	const TerrainViewPattern & getPatternById(ETerrainGroup::ETerrainGroup terGroup, const std::string & id) const;

private:
	CTerrainViewPatternConfig();
	~CTerrainViewPatternConfig();

	std::map<ETerrainGroup::ETerrainGroup, std::vector<TerrainViewPattern> > patterns;
	static boost::mutex smx;
};

/// The DrawTerrainOperation class draws a terrain area on the map.
class DrawTerrainOperation : public CMapOperation
{
public:
	DrawTerrainOperation(CMap * map, const MapRect & rect, ETerrainType terType, CRandomGenerator * gen);

	void execute();
	void undo();
	void redo();
	std::string getLabel() const;

private:
	struct ValidationResult
	{
		ValidationResult(bool result, const std::string & transitionReplacement = "");

		bool result;
		/// The replacement of a T rule, either D or S.
		std::string transitionReplacement;
	};

	void updateTerrainViews(const MapRect & rect);
	ETerrainGroup::ETerrainGroup getTerrainGroup(ETerrainType terType) const;
	/// Validates the terrain view of the given position and with the given pattern.
	ValidationResult validateTerrainView(const int3 & pos, const TerrainViewPattern & pattern, int recDepth = 0) const;
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

/// The InsertObjectOperation class inserts an object to the map.
class InsertObjectOperation : public CMapOperation
{
public:
	InsertObjectOperation(CMap * map, const int3 & pos, CGObjectInstance * obj);

	void execute();
	void undo();
	void redo();
	std::string getLabel() const;

private:
	int3 pos;
	CGObjectInstance * obj;
};
