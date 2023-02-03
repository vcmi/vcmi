/*
 * CResDataBar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

/// Resources bar which shows information about how many gold, crystals,... you have
/// Current date is displayed too
class CResDataBar : public CIntObject
{
	std::string buildDateString();

public:
	std::shared_ptr<CPicture> background;

	std::vector<std::pair<int,int> > txtpos;

	void clickRight(tribool down, bool previousState) override;
	CResDataBar();
	CResDataBar(const std::string &defname, int x, int y, int offx, int offy, int resdist, int datedist);
	~CResDataBar();

	void draw(SDL_Surface * to);
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};

