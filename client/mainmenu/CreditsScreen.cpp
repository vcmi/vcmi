/*
 * CreditsScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CreditsScreen.h"

#include "CMainMenu.h"

#include "../gui/CGuiHandler.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"

#include "../../lib/filesystem/Filesystem.h"

CreditsScreen::CreditsScreen(Rect rect)
	: CIntObject(LCLICK), positionCounter(0)
{
	pos.w = rect.w;
	pos.h = rect.h;
	setRedrawParent(true);
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	auto textFile = CResourceHandler::get()->load(ResourceID("DATA/CREDITS.TXT"))->readAll();
	std::string text((char *)textFile.first.get(), textFile.second);
	size_t firstQuote = text.find('\"') + 1;
	text = text.substr(firstQuote, text.find('\"', firstQuote) - firstQuote);
	credits = std::make_shared<CMultiLineLabel>(Rect(pos.w - 350, 0, 350, 600), FONT_CREDITS, ETextAlignment::CENTER, Colors::WHITE, text);
	credits->scrollTextTo(-600); // move all text below the screen
}

void CreditsScreen::show(Canvas & to)
{
	CIntObject::show(to);
	positionCounter++;
	if(positionCounter % 2 == 0)
		credits->scrollTextBy(1);

	//end of credits, close this screen
	if(credits->textSize.y + 600 < positionCounter / 2)
		clickLeft(false, false);
}

void CreditsScreen::clickLeft(tribool down, bool previousState)
{
	CTabbedInt * menu = dynamic_cast<CTabbedInt *>(parent);
	assert(menu);
	menu->setActive(0);
}
