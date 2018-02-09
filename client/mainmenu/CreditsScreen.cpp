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
#include "../mainmenu/CMainMenu.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"

#include "../../lib/filesystem/Filesystem.h"

CreditsScreen::CreditsScreen()
	: positionCounter(0)
{
	addUsedEvents(LCLICK | RCLICK);
	type |= REDRAW_PARENT;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.w = CMM->menu->pos.w;
	pos.h = CMM->menu->pos.h;
	auto textFile = CResourceHandler::get()->load(ResourceID("DATA/CREDITS.TXT"))->readAll();
	std::string text((char *)textFile.first.get(), textFile.second);
	size_t firstQuote = text.find('\"') + 1;
	text = text.substr(firstQuote, text.find('\"', firstQuote) - firstQuote);
	credits = new CMultiLineLabel(Rect(pos.w - 350, 0, 350, 600), FONT_CREDITS, CENTER, Colors::WHITE, text);
	credits->scrollTextTo(-600); // move all text below the screen
}

void CreditsScreen::show(SDL_Surface * to)
{
	CIntObject::show(to);
	positionCounter++;
	if(positionCounter % 2 == 0)
		credits->scrollTextBy(1);

	//end of credits, close this screen
	if(credits->textSize.y + 600 < positionCounter / 2)
		clickRight(false, false);
}

void CreditsScreen::clickLeft(tribool down, bool previousState)
{
	clickRight(down, previousState);
}

void CreditsScreen::clickRight(tribool down, bool previousState)
{
	CTabbedInt * menu = dynamic_cast<CTabbedInt *>(parent);
	assert(menu);
	menu->setActive(0);
}
