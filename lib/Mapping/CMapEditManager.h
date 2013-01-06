
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

namespace ETerrainGroup
{
	/**
	 * This enumeration lists terrain groups which differ in the terrain view frames alignment.
	 */
	enum ETerrainGroup
	{
		NORMAL,
		DIRT,
		SAND,
		WATER,
		ROCK
	};
}

/**
 * The terrain view pattern describes a specific composition of terrain tiles
 * in a 3x3 matrix and notes which terrain view frame numbers can be used.
 */
struct TerrainViewPattern
{
	/** Constant for the flip mode same image. Pattern will be flipped and the same image will be used(which is given in the mapping). */
	static const std::string FLIP_MODE_SAME_IMAGE;

	/** Constant for the flip mode different images. Pattern will be flipped and different images will be used(mapping area is divided into 4 parts) */
	static const std::string FLIP_MODE_DIFF_IMAGES;

	/** Constant for the rule dirt, meaning a dirty border is required. */
	static const std::string RULE_DIRT;

	/** Constant for the rule sand, meaning a sandy border is required. */
	static const std::string RULE_SAND;

	/** Constant for the rule transition, meaning a dirty OR sandy border is required. */
	static const std::string RULE_TRANSITION;

	/** Constant for the rule native, meaning a native type is required. */
	static const std::string RULE_NATIVE;

	/** Constant for the rule any, meaning a native type, dirty OR sandy border is required. */
	static const std::string RULE_ANY;

	/**
	 * Default constructor.
	 */
	TerrainViewPattern();

	/**
	 * The pattern data.
	 *
	 * It can be visualized as a 3x3 matrix:
	 * [ ][ ][ ]
	 * [ ][ ][ ]
	 * [ ][ ][ ]
	 *
	 * The box in the center belongs always to the native terrain type and
	 * is the point of origin. Depending on the terrain type different rules
	 * can be used. Their meaning differs also from type to type.
	 *
	 * std::vector -> several rules can be used in one cell
	 * std::pair   -> combination of the name of the rule and a optional number of points
	 */
	std::array<std::vector<std::pair<std::string, int> >, 9> data;

	/** The identifier of the pattern, if it's referenced from a another pattern. */
	std::string id;

	/**
	 * This describes the mapping between this pattern and the corresponding range of frames
	 * which should be used for the ter view.
	 *
	 * std::vector -> size=1: typical, size=2: if this pattern should map to two different types of borders
	 * std::pair   -> 1st value: lower range, 2nd value: upper range
	 */
	std::vector<std::pair<int, int> > mapping;

	/** The minimum points to reach to to validate the pattern successfully. */
	int minPoints;

	/** Describes if flipping is required and which mapping should be used. */
	std::string flipMode;

	/** The terrain group to which the pattern belongs to. */
	ETerrainGroup::ETerrainGroup terGroup;
};

/**
 * The terrain view pattern config loads pattern data from the filesystem.
 */
class CTerrainViewPatternConfig
{
public:
	/**
	 * Constructor. Initializes the patterns data.
	 */
	CTerrainViewPatternConfig();

	/**
	 * Gets the patterns for a specific group of terrain.
	 *
	 * @param terGroup the terrain group e.g. normal for grass, lava,... OR dirt OR sand,...
	 * @return a vector containing patterns
	 */
	const std::vector<TerrainViewPattern> & getPatternsForGroup(ETerrainGroup::ETerrainGroup terGroup) const;

private:
	/** The patterns data. */
	std::map<ETerrainGroup::ETerrainGroup, std::vector<TerrainViewPattern> > patterns;
};

/**
 * The map edit manager provides functionality for drawing terrain and placing
 * objects on the map.
 *
 * TODO add undo / selection functionality for the map editor
 */
class CMapEditManager
{
public:
	/**
	 * Constructor. The map object / terrain data has to be initialized.
	 *
	 * @param terViewPatternConfig the terrain view pattern config
	 * @param map the map object which should be edited
	 * @param randomSeed optional. the seed which is used for generating randomly terrain views
	 */
	CMapEditManager(const CTerrainViewPatternConfig * terViewPatternConfig, CMap * map, int randomSeed = std::time(nullptr));

	/**
	 * Clears the terrain. The free level is filled with water and the
	 * underground level with rock.
	 */
	void clearTerrain();

	/**
	 * Draws terrain.
	 *
	 * @param terType the type of the terrain to draw
	 * @param posx the x coordinate
	 * @param posy the y coordinate
	 * @param width the height of the terrain to draw
	 * @param height the width of the terrain to draw
	 * @param underground true if you want to draw at the underground, false if open
	 */
	void drawTerrain(ETerrainType::ETerrainType terType, int posx, int posy, int width, int height, bool underground);

	/**
	 * Inserts an object.
	 *
	 * @param obj the object to insert
	 * @param posx the x coordinate
	 * @param posy the y coordinate
	 * @param underground true if you want to draw at the underground, false if open
	 */
	void insertObject(CGObjectInstance * obj, int posx, int posy, bool underground);

private:
	/**
	 * The validation result struct represents the result of a pattern validation.
	 */
	struct ValidationResult
	{
		/**
		 * Constructor.
		 *
		 * @param result the result of the validation either true or false
		 * @param points optional. the points which were achieved with that pattern
		 * @param transitionReplacement optional. the replacement of a T rule, either D or S
		 */
		ValidationResult(bool result, int points = 0, const std::string & transitionReplacement = "");

		/** The result of the validation. */
		bool result;

		/** The points which were achieved with that pattern. */
		int points;

		/** The replacement of a T rule, either D or S. */
		std::string transitionReplacement;
	};

	/**
	 * Updates the terrain view ids in the specified area.
	 *
	 * @param posx the x coordinate
	 * @param posy the y coordinate
	 * @param width the height of the terrain to update
	 * @param height the width of the terrain to update
	 * @param mapLevel the map level, 0 for open and 1 for underground
	 */
	void updateTerrainViews(int posx, int posy, int width, int height, int mapLevel);

	/**
	 * Gets the terrain group by the terrain type number.
	 *
	 * @param terType the terrain type
	 * @return the terrain group
	 */
	ETerrainGroup::ETerrainGroup getTerrainGroup(ETerrainType::ETerrainType terType) const;

	/**
	 * Validates the terrain view of the given position and with the given pattern.
	 *
	 * @param posx the x position
	 * @param posy the y position
	 * @param mapLevel the map level, 0 for open and 1 for underground
	 * @param pattern the pattern to validate the terrain view with
	 * @return a validation result struct
	 */
	ValidationResult validateTerrainView(int posx, int posy, int mapLevel, const TerrainViewPattern & pattern) const;

	/**
	 * Tests whether the given terrain type is a sand type. Sand types are: Water, Sand and Rock
	 *
	 * @param terType the terrain type to test
	 * @return true if the terrain type is a sand type, otherwise false
	 */
	bool isSandType(ETerrainType::ETerrainType terType) const;

	/**
	 * Gets a flipped pattern.
	 *
	 * @param pattern the original pattern to flip
	 * @param flip the flip mode value, see FLIP_PATTERN_* constants for details
	 * @return the flipped pattern
	 */
	TerrainViewPattern getFlippedPattern(const TerrainViewPattern & pattern, int flip) const;

	/** Constant for flipping a pattern horizontally. */
	static const int FLIP_PATTERN_HORIZONTAL = 1;

	/** Constant for flipping a pattern vertically. */
	static const int FLIP_PATTERN_VERTICAL = 2;

	/** Constant for flipping a pattern horizontally and vertically. */
	static const int FLIP_PATTERN_BOTH = 3;

	/** The map object to edit. */
	CMap * map;

	/** The random number generator. */
	CRandomGenerator gen;

	/** The terrain view pattern config. */
	const CTerrainViewPatternConfig * terViewPatternConfig;
};
