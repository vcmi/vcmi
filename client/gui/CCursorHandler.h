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

class CAnimImage;
struct SDL_Surface;

namespace ECursor
{
	enum ECursorTypes { ADVENTURE, COMBAT, DEFAULT, SPELLBOOK };

	enum EBattleCursors { COMBAT_BLOCKED, COMBAT_MOVE, COMBAT_FLY, COMBAT_SHOOT,
						COMBAT_HERO, COMBAT_QUERY, COMBAT_POINTER,
						//various attack frames
						COMBAT_SHOOT_PENALTY = 15, COMBAT_SHOOT_CATAPULT, COMBAT_HEAL,
						COMBAT_SACRIFICE, COMBAT_TELEPORT};
}

/// handles mouse cursor
class CCursorHandler final
{
	SDL_Surface * help;
	CAnimImage * currentCursor;

	std::unique_ptr<CAnimImage> dndObject; //if set, overrides currentCursor

	std::array<std::unique_ptr<CAnimImage>, 4> cursors;

	bool showing;

	/// Draw cursor preserving original image below cursor
	void drawWithScreenRestore();
	/// Restore original image below cursor
	void drawRestored();
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

	void shiftPos( int &x, int &y );
	void hide() { showing=0; };
	void show() { showing=1; };

	/// change cursor's positions to (x, y)
	void cursorMove(const int & x, const int & y);
	/// Move cursor to screen center
	void centerCursor();

	CCursorHandler();
	~CCursorHandler();
};
