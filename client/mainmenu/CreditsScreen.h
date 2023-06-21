/*
 * CreditsScreen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class CMultiLineLabel;

class CreditsScreen : public CIntObject
{
	int positionCounter;
	std::shared_ptr<CMultiLineLabel> credits;

public:
	CreditsScreen(Rect rect);
	void show(Canvas & to) override;
	void clickLeft(tribool down, bool previousState) override;
};
