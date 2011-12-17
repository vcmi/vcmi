#pragma once

#include "IShowable.h"
#include "IActivatable.h"

/*
 * IShowActivatable.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class IShowActivatable : public IShowable, public IActivatable
{
public:
	//redraw parent flag - this int may be semi-transparent and require redraw of parent window
	enum {WITH_GARRISON = 1, BLOCK_ADV_HOTKEYS = 2, WITH_ARTIFACTS = 4, REDRAW_PARENT=8};
	int type; //bin flags using etype
	IShowActivatable();
	virtual ~IShowActivatable(){}; //d-tor
};