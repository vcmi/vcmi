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

CreditsScreen::CreditsScreen(Rect rect)
	: View(LCLICK | RCLICK), positionCounter(0)
{
	pos.w = rect.w;
	pos.h = rect.h;
	type |= REDRAW_PARENT;
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	auto textFile = CResourceHandler::get()->load(ResourceID("DATA/CREDITS.TXT"))->readAll();
	std::string text((char *)textFile.first.get(), textFile.second);
	size_t firstQuote = text.find('\"') + 1;
	text = text.substr(firstQuote, text.find('\"', firstQuote) - firstQuote);
	credits = std::make_shared<CMultiLineLabel>(Rect(pos.w - 350, 0, 350, 600), FONT_CREDITS, CENTER, Colors::WHITE, text);
	credits->scrollTextTo(-600); // move all text below the screen
}

void CreditsScreen::show(SDL_Surface * to)
{
	View::show(to);
	positionCounter++;
	if(positionCounter % 2 == 0)
		credits->scrollTextBy(1);

	//end of credits, close this screen
	if(credits->textSize.y + 600 < positionCounter / 2)
		dynamic_cast<CTabbedInt *>(getParent())->setActive(0);
}

void CreditsScreen::clickLeft(const SDL_Event &event, tribool down)
{
	dynamic_cast<CTabbedInt *>(getParent())->setActive(0);
}

void CreditsScreen::clickRight(const SDL_Event &event, tribool down)
{
	CTabbedInt * menu = dynamic_cast<CTabbedInt *>(getParent());
	assert(menu);
	menu->setActive(0);
}
