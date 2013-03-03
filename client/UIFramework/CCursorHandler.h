#pragma once

#include "../Gfx/Manager.h"

class CAnimImage;

/*
 * CCursorhandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
class CCursorHandler 
{
	Gfx::PImage help;
	Gfx::PAnimation currentCursor;
	CAnimImage* dndObject; // if set, overrides currentCursor
	bool showing;

public:
	/// position of cursor
	int xpos, ypos;

	/// Current cursor
	ECursor::ECursorTypes type;
	size_t frame;

	/// inits cursorHandler - run only once, it's not memleak-proof (rev 1333)
	CCursorHandler();

	/// changes cursor graphic for type type (0 - adventure, 1 - combat, 2 - default, 3 - spellbook) and frame index (not used for type 3)
	void changeGraphic(ECursor::ECursorTypes type, int index);

	/**
	 * Replaces the cursor with a custom image.
	 *
	 * @param image Image to replace cursor with or NULL to use the normal
	 * cursor. CursorHandler takes ownership of object
	 */
	void dragAndDropCursor (CAnimImage * image);

	/// Draw cursor preserving original image below cursor
	void drawWithScreenRestore();
	/// Restore original image below cursor
	void drawRestored();
	/// Simple draw cursor
	void draw();

	void shiftPos( int &x, int &y );
	void hide() { showing=0; };
	void show() { showing=1; };

	/// change cursor's positions to (x, y)
	void cursorMove(int x, int y);
	/// Move cursor to screen center
	void centerCursor();

	~CCursorHandler();
};
