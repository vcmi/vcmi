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
#include "CResDataBar.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../render/Colors.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/Images.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"

#define ADVOPT (conf.go()->ac)

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

}

CResDataBar::~CResDataBar() = default;

std::string CResDataBar::buildDateString()
{
	std::string pattern = "%s: %d, %s: %d, %s: %d";

	auto formatted = boost::format(pattern)
		% CGI->generaltexth->allTexts[62] % LOCPLINT->cb->getDate(Date::MONTH)
		% CGI->generaltexth->allTexts[63] % LOCPLINT->cb->getDate(Date::WEEK)
		% CGI->generaltexth->allTexts[64] % LOCPLINT->cb->getDate(Date::DAY_OF_WEEK);

	return boost::str(formatted);
}

void CResDataBar::draw(SDL_Surface * to)
{
	//TODO: all this should be labels, but they require proper text update on change
	for (auto i=Res::WOOD; i<=Res::GOLD; vstd::advance(i, 1))
	{
		std::string text = boost::lexical_cast<std::string>(LOCPLINT->cb->getResourceAmount(i));

		graphics->fonts[FONT_SMALL]->renderTextLeft(to, text, Colors::WHITE, Point(txtpos[i].first,txtpos[i].second));
	}
	graphics->fonts[FONT_SMALL]->renderTextLeft(to, buildDateString(), Colors::WHITE, Point(txtpos[7].first,txtpos[7].second));
}

void CResDataBar::show(SDL_Surface * to)
{

}

void CResDataBar::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	draw(to);
}

