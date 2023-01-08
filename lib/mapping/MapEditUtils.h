/*
 * MapEditUtils.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../int3.h"
#include "../CRandomGenerator.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CMap;

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
	MapRect operator&(const MapRect& rect) const;

	template<typename Func>
	void forEach(Func f) const
	{
		for(int j = y; j < bottom(); ++j)
		{
			for(int i = x; i < right(); ++i)
			{
				f(int3(i, j, z));
			}
		}
	}
};

/// Generic selection class to select any type
template<typename T>
class DLL_LINKAGE CMapSelection
{
public:
	explicit CMapSelection(CMap* map) : map(map) { }
	virtual ~CMapSelection() = default;
	void select(const T & item)
	{
		selectedItems.insert(item);
	}
	void deselect(const T & item)
	{
		selectedItems.erase(item);
	}
	std::set<T> getSelectedItems()
	{
		return selectedItems;
	}
	CMap* getMap() { return map; }
	virtual void selectRange(const MapRect & rect) { }
	virtual void deselectRange(const MapRect & rect) { }
	virtual void selectAll() { }
	virtual void clearSelection() { }

private:
	std::set<T> selectedItems;
	CMap* map;
};

/// Selection class to select terrain.
class DLL_LINKAGE CTerrainSelection : public CMapSelection<int3>
{
public:
	explicit CTerrainSelection(CMap * map);
	void selectRange(const MapRect & rect) override;
	void deselectRange(const MapRect & rect) override;
	void selectAll() override;
	void clearSelection() override;
	void setSelection(const std::vector<int3> & vec);
};

/// Selection class to select objects.
class DLL_LINKAGE CObjectSelection : public CMapSelection<CGObjectInstance *>
{
public:
	explicit CObjectSelection(CMap * map);
};

/// The terrain view pattern describes a specific composition of terrain tiles
/// in a 3x3 matrix and notes which terrain view frame numbers can be used.
struct DLL_LINKAGE TerrainViewPattern
{
	struct WeightedRule
	{
		WeightedRule(std::string& Name);
		/// Gets true if this rule is a standard rule which means that it has a value of one of the RULE_* constants.
		inline bool isStandardRule() const
		{
			return standardRule;
		}
		inline bool isAnyRule() const
		{
			return anyRule;
		}
		inline bool isDirtRule() const
		{
			return dirtRule;
		}
		inline bool isSandRule() const
		{
			return sandRule;
		}
		inline bool isTransition() const
		{
			return transitionRule;
		}
		inline bool isNativeStrong() const
		{
			return nativeStrongRule;
		}
		inline bool isNativeRule() const
		{
			return nativeRule;
		}
		void setNative();

		/// The name of the rule. Can be any value of the RULE_* constants or a ID of a another pattern.
		//FIXME: remove string variable altogether, use only in constructor
		std::string name;
		/// Optional. A rule can have points. Patterns may have a minimum count of points to reach to be successful.
		int points;

	private:
		bool standardRule;
		bool anyRule;
		bool dirtRule;
		bool sandRule;
		bool transitionRule;
		bool nativeStrongRule;
		bool nativeRule;

		WeightedRule(); //only allow string constructor
	};

	static const int PATTERN_DATA_SIZE = 9;
	/// Constant for the flip mode different images. Pattern will be flipped and different images will be used(mapping area is divided into 4 parts)
	static const std::string FLIP_MODE_DIFF_IMAGES;
	/// Constant for the rule dirt, meaning a dirty border is required.
	static const std::string RULE_DIRT;
	/// Constant for the rule sand, meaning a sandy border is required.
	static const std::string RULE_SAND;
	/// Constant for the rule transition, meaning a dirty OR sandy border is required.
	static const std::string RULE_TRANSITION;
	/// Constant for the rule native, meaning a native border is required.
	static const std::string RULE_NATIVE;
	/// Constant for the rule native strong, meaning a native type is required.
	static const std::string RULE_NATIVE_STRONG;
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
	std::array<std::vector<WeightedRule>, PATTERN_DATA_SIZE> data;

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
};

/// The terrain view pattern config loads pattern data from the filesystem.
class DLL_LINKAGE CTerrainViewPatternConfig : public boost::noncopyable
{
public:
	typedef std::vector<TerrainViewPattern> TVPVector;

	CTerrainViewPatternConfig();
	~CTerrainViewPatternConfig();

	const std::vector<TVPVector> & getTerrainViewPatterns(TerrainId terrain) const;
	boost::optional<const TerrainViewPattern &> getTerrainViewPatternById(std::string patternId, const std::string & id) const;
	boost::optional<const TVPVector &> getTerrainViewPatternsById(TerrainId terrain, const std::string & id) const;
	const TVPVector * getTerrainTypePatternById(const std::string & id) const;
	void flipPattern(TerrainViewPattern & pattern, int flip) const;

private:
	std::map<std::string, std::vector<TVPVector> > terrainViewPatterns;
	std::map<std::string, TVPVector> terrainTypePatterns;
};

class DLL_LINKAGE CTerrainViewPatternUtils
{
public:
	static void printDebuggingInfoAboutTile(const CMap * map, int3 pos);
};

VCMI_LIB_NAMESPACE_END
