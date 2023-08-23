/*
 * CCursorHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/Point.h"
#include "../../lib/filesystem/ResourcePath.h"

class ICursor;
class IImage;
class CAnimation;

namespace Cursor
{
	enum class Type {
		ADVENTURE, // set of various cursors for adventure map
		COMBAT,    // set of various cursors for combat
		DEFAULT,   // default arrow and hourglass cursors
		SPELLBOOK  // animated cursor for spellcasting
	};

	enum class Default {
		POINTER      = 0,
		//ARROW_COPY = 1, // probably unused
		HOURGLASS  = 2,
	};

	enum class Combat {
		BLOCKED        = 0,
		MOVE           = 1,
		FLY            = 2,
		SHOOT          = 3,
		HERO           = 4,
		QUERY          = 5,
		POINTER        = 6,
		HIT_NORTHEAST  = 7,
		HIT_EAST       = 8,
		HIT_SOUTHEAST  = 9,
		HIT_SOUTHWEST  = 10,
		HIT_WEST       = 11,
		HIT_NORTHWEST  = 12,
		HIT_NORTH      = 13,
		HIT_SOUTH      = 14,
		SHOOT_PENALTY  = 15,
		SHOOT_CATAPULT = 16,
		HEAL           = 17,
		SACRIFICE      = 18,
		TELEPORT       = 19,

		COUNT
	};

	enum class Map {
		POINTER          =  0,
		HOURGLASS        =  1,
		HERO             =  2,
		TOWN             =  3,
		T1_MOVE          =  4,
		T1_ATTACK        =  5,
		T1_SAIL          =  6,
		T1_DISEMBARK     =  7,
		T1_EXCHANGE      =  8,
		T1_VISIT         =  9,
		T2_MOVE          = 10,
		T2_ATTACK        = 11,
		T2_SAIL          = 12,
		T2_DISEMBARK     = 13,
		T2_EXCHANGE      = 14,
		T2_VISIT         = 15,
		T3_MOVE          = 16,
		T3_ATTACK        = 17,
		T3_SAIL          = 18,
		T3_DISEMBARK     = 19,
		T3_EXCHANGE      = 20,
		T3_VISIT         = 21,
		T4_MOVE          = 22,
		T4_ATTACK        = 23,
		T4_SAIL          = 24,
		T4_DISEMBARK     = 25,
		T4_EXCHANGE      = 26,
		T4_VISIT         = 27,
		T1_SAIL_VISIT    = 28,
		T2_SAIL_VISIT    = 29,
		T3_SAIL_VISIT    = 30,
		T4_SAIL_VISIT    = 31,
		SCROLL_NORTH     = 32,
		SCROLL_NORTHEAST = 33,
		SCROLL_EAST      = 34,
		SCROLL_SOUTHEAST = 35,
		SCROLL_SOUTH     = 36,
		SCROLL_SOUTHWEST = 37,
		SCROLL_WEST      = 38,
		SCROLL_NORTHWEST = 39,
		//POINTER_COPY       = 40, // probably unused
		TELEPORT         = 41,
		SCUTTLE_BOAT     = 42,

		COUNT
	};

	enum class Spellcast {
		SPELL = 0,
	};
}

/// handles mouse cursor
class CursorHandler final
{
	std::shared_ptr<IImage> dndObject; //if set, overrides currentCursor

	std::array<std::unique_ptr<CAnimation>, 4> cursors;

	bool showing;

	/// Current cursor
	Cursor::Type type;
	size_t frame;
	float frameTime;
	Point pos;

	void changeGraphic(Cursor::Type type, size_t index);

	Point getPivotOffsetSpellcast();
	Point getPivotOffset();

	void updateSpellcastCursor();

	std::shared_ptr<IImage> getCurrentImage();

	std::unique_ptr<ICursor> cursor;

	static std::unique_ptr<ICursor> createCursor();
public:
	CursorHandler();
	~CursorHandler();

	/// Replaces the cursor with a custom image.
	/// @param image Image to replace cursor with or nullptr to use the normal cursor.
	void dragAndDropCursor(std::shared_ptr<IImage> image);

	void dragAndDropCursor(const AnimationPath & path, size_t index);

	/// Changes cursor to specified index
	void set(Cursor::Default index);
	void set(Cursor::Map index);
	void set(Cursor::Combat index);
	void set(Cursor::Spellcast index);

	/// Returns current index of cursor
	template<typename Index>
	Index get()
	{
		bool typeValid = true;

		typeValid &= (std::is_same<Index, Cursor::Default>::value   )|| type != Cursor::Type::DEFAULT;
		typeValid &= (std::is_same<Index, Cursor::Map>::value       )|| type != Cursor::Type::ADVENTURE;
		typeValid &= (std::is_same<Index, Cursor::Combat>::value    )|| type != Cursor::Type::COMBAT;
		typeValid &= (std::is_same<Index, Cursor::Spellcast>::value )|| type != Cursor::Type::SPELLBOOK;

		if (typeValid)
			return static_cast<Index>(frame);
		return Index::POINTER;
	}

	Point getPivotOffsetDefault(size_t index);
	Point getPivotOffsetMap(size_t index);
	Point getPivotOffsetCombat(size_t index);

	void render();

	void hide();
	void show();

	/// change cursor's positions to (x, y)
	void cursorMove(const int & x, const int & y);
};
