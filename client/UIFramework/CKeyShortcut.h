#pragma once

#include "CIntObject.h"

/*
 * CKeyShortcut.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class for binding keys to left mouse button clicks
/// Classes wanting use it should have it as one of their base classes
class CKeyShortcut : public virtual CIntObject
{
public:
	std::set<int> assignedKeys;
	CKeyShortcut(){}; //c-tor
	CKeyShortcut(int key){assignedKeys.insert(key);}; //c-tor
	CKeyShortcut(std::set<int> Keys):assignedKeys(Keys){}; //c-tor
	virtual void keyPressed(const SDL_KeyboardEvent & key); //call-in
};