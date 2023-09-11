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

	// update contributors with bash:   curl -s "https://api.github.com/repos/vcmi/vcmi/contributors" | jq '.[].login' | tr '\n' '|' | sed 's/|/, /g' | sed 's/..$//'
	std::vector<std::string> contributors = { "IvanSavenko", "alexvins", "mwu-tow", "Nordsoft91", "mateuszbaran", "ArseniyShestakov", "nullkiller", "dydzio0614", "kambala-decapitator", "rilian-la-te", "DjWarmonger", "Laserlicht", "beegee1", "henningkoehlernz", "SoundSSGood", "vmarkovtsev", "krs0", "Mixaill", "Fayth", "ShubusCorporation", "rwesterlund", "Macron1Robot", "Chocimier", "Zyx-2000", "bwrsandman", "stopiccot", "viciious", "karol57", "heroesiiifan", "Adriankhl" };

public:
	CreditsScreen(Rect rect);
	void show(Canvas & to) override;
	void clickPressed(const Point & cursorPosition) override;
};
