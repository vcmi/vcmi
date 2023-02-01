/*
 * CResDataBar.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAdvmapInterface.h"

#include "CCastleInterface.h"
#include "CHeroWindow.h"
#include "CKingdomInterface.h"
#include "CSpellWindow.h"
#include "CTradeWindow.h"
#include "GUIClasses.h"
#include "InfoWindows.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../mainmenu/CMainMenu.h"
#include "../lobby/CSelectionBase.h"
#include "../lobby/CCampaignInfoScreen.h"
#include "../lobby/CSavingScreen.h"
#include "../lobby/CScenarioInfoScreen.h"
#include "../Graphics.h"
#include "../mapHandler.h"

#include "../gui/CAnimation.h"
#include "../gui/CursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/MiscWidgets.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CSoundBase.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/TerrainHandler.h"

#include <SDL_surface.h>
#include <SDL_events.h>

#define ADVOPT (conf.go()->ac)
using namespace CSDL_Ext;

std::shared_ptr<CAdvMapInt> adventureInt;

static void setScrollingCursor(ui8 direction)
{
	if(direction & CAdvMapInt::RIGHT)
	{
		if(direction & CAdvMapInt::UP)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHEAST);
		else if(direction & CAdvMapInt::DOWN)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHEAST);
		else
			CCS->curh->set(Cursor::Map::SCROLL_EAST);
	}
	else if(direction & CAdvMapInt::LEFT)
	{
		if(direction & CAdvMapInt::UP)
			CCS->curh->set(Cursor::Map::SCROLL_NORTHWEST);
		else if(direction & CAdvMapInt::DOWN)
			CCS->curh->set(Cursor::Map::SCROLL_SOUTHWEST);
		else
			CCS->curh->set(Cursor::Map::SCROLL_WEST);
	}
	else if(direction & CAdvMapInt::UP)
		CCS->curh->set(Cursor::Map::SCROLL_NORTH);
	else if(direction & CAdvMapInt::DOWN)
		CCS->curh->set(Cursor::Map::SCROLL_SOUTH);
}

void CResDataBar::clickRight(tribool down, bool previousState)
{
}

CResDataBar::CResDataBar(const std::string & defname, int x, int y, int offx, int offy, int resdist, int datedist)
{
	pos.x += x;
	pos.y += y;
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>(defname, 0, 0);
	background->colorize(LOCPLINT->playerID);

	pos.w = background->pos.w;
	pos.h = background->pos.h;

	txtpos.resize(8);
	for (int i = 0; i < 8 ; i++)
	{
		txtpos[i].first = pos.x + offx + resdist*i;
		txtpos[i].second = pos.y + offy;
	}
	txtpos[7].first = txtpos[6].first + datedist;
	datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63]
	+ ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";
	addUsedEvents(RCLICK);
}

CResDataBar::CResDataBar()
{
	pos.x += ADVOPT.resdatabarX;
	pos.y += ADVOPT.resdatabarY;

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>(ADVOPT.resdatabarG, 0, 0);
	background->colorize(LOCPLINT->playerID);

	pos.w = background->pos.w;
	pos.h = background->pos.h;

	txtpos.resize(8);
	for (int i = 0; i < 8 ; i++)
	{
		txtpos[i].first = pos.x + ADVOPT.resOffsetX + ADVOPT.resDist*i;
		txtpos[i].second = pos.y + ADVOPT.resOffsetY;
	}
	txtpos[7].first = txtpos[6].first + ADVOPT.resDateDist;
	datetext =  CGI->generaltexth->allTexts[62]+": %s, " + CGI->generaltexth->allTexts[63]
				+ ": %s, " + CGI->generaltexth->allTexts[64] + ": %s";
}

CResDataBar::~CResDataBar() = default;

void CResDataBar::draw(SDL_Surface * to)
{
	//TODO: all this should be labels, but they require proper text update on change
	for (auto i=Res::WOOD; i<=Res::GOLD; vstd::advance(i, 1))
	{
		std::string text = boost::lexical_cast<std::string>(LOCPLINT->cb->getResourceAmount(i));

		graphics->fonts[FONT_SMALL]->renderTextLeft(to, text, Colors::WHITE, Point(txtpos[i].first,txtpos[i].second));
	}
	std::vector<std::string> temp;

	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::MONTH)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::WEEK)));
	temp.push_back(boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK)));

	graphics->fonts[FONT_SMALL]->renderTextLeft(to, processStr(datetext,temp), Colors::WHITE, Point(txtpos[7].first,txtpos[7].second));
}

void CResDataBar::show(SDL_Surface * to)
{

}

void CResDataBar::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	draw(to);
}

