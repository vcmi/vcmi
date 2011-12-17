#pragma once

#include "UIFramework/CIntObject.h"

/*
 * CSimpleWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// Window GUI class
class CSimpleWindow : public CIntObject
{
public:
	SDL_Surface * bitmap; //background
	virtual void show(SDL_Surface * to);
	CSimpleWindow():bitmap(NULL){}; //c-tor
	virtual ~CSimpleWindow(); //d-tor
};