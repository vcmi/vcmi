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
class CIntObject;
class CAnimImage;
struct SDL_Surface;
struct SDL_Texture;

namespace ECursor
{
	enum ECursorTypes {
		ADVENTURE, // set of various cursors for adventure map
		COMBAT,    // set of various cursors for combat
		DEFAULT,   // default arror and hourglass cursors
		SPELLBOOK  // animated cursor for spellcasting
	};

	enum EBattleCursors {
		COMBAT_BLOCKED        = 0,
		COMBAT_MOVE           = 1,
		COMBAT_FLY            = 2,
		COMBAT_SHOOT          = 3,
		COMBAT_HERO           = 4,
		COMBAT_QUERY          = 5,
		COMBAT_POINTER        = 6,
		COMBAT_HIT_NORTHEAST  = 7,
		COMBAT_HIT_EAST       = 8,
		COMBAT_HIT_SOUTHEAST  = 9,
		COMBAT_HIT_SOUTHWEST  = 10,
		COMBAT_HIT_WEST       = 11,
		COMBAT_HIT_NORTHWEST  = 12,
		COMBAT_HIT_NORTH      = 13,
		COMBAT_HIT_SOUTH      = 14,
		COMBAT_SHOOT_PENALTY  = 15,
		COMBAT_SHOOT_CATAPULT = 16,
		COMBAT_HEAL           = 17,
		COMBAT_SACRIFICE      = 18,
		COMBAT_TELEPORT       = 19
	};
}

/// handles mouse cursor
class CCursorHandler final
{
	bool needUpdate;
	SDL_Texture * cursorLayer;

	SDL_Surface * buffer;
	CAnimImage * currentCursor;

	std::unique_ptr<CAnimImage> dndObject; //if set, overrides currentCursor

	std::array<std::unique_ptr<CAnimImage>, 4> cursors;

	bool showing;

	void clearBuffer();
	void updateBuffer(CIntObject * payload);
	void replaceBuffer(CIntObject * payload);
	void shiftPos( int &x, int &y );

	void updateTexture();
public:
	/// position of cursor
	int xpos, ypos;

	/// Current cursor
	ECursor::ECursorTypes type;
	size_t frame;

	/// inits cursorHandler - run only once, it's not memleak-proof (rev 1333)
	void initCursor();

	/// changes cursor graphic for type type (0 - adventure, 1 - combat, 2 - default, 3 - spellbook) and frame index (not used for type 3)
	void changeGraphic(ECursor::ECursorTypes type, int index);

	/**
	 * Replaces the cursor with a custom image.
	 *
	 * @param image Image to replace cursor with or nullptr to use the normal
	 * cursor. CursorHandler takes ownership of object
	 */
	void dragAndDropCursor (std::unique_ptr<CAnimImage> image);

	void render();

	void hide() { showing=false; };
	void show() { showing=true; };

	/// change cursor's positions to (x, y)
	void cursorMove(const int & x, const int & y);
	/// Move cursor to screen center
	void centerCursor();

	CCursorHandler();
	~CCursorHandler();
};
