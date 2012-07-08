#include "StdInc.h"
#include "CQuestLog.h"

#include "CGameInfo.h"
#include "../lib/CGeneralTextHandler.h"
#include "../CCallback.h"

#include <SDL.h>
#include "UIFramework/SDL_Extensions.h"
#include "CBitmapHandler.h"
#include "CDefHandler.h"
#include "Graphics.h"
#include "CPlayerInterface.h"
#include "CConfigHandler.h"

#include "../lib/CGameState.h"
#include "../lib/CArtHandler.h"
#include "../lib/NetPacks.h"
#include "../lib/CObjectHandler.h"

#include "UIFramework/CGuiHandler.h"
#include "UIFramework/CIntObjectClasses.h"

/*
 * CQuestLog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct QuestInfo;
class CAdvmapInterface;

void CQuestLabel::clickLeft(tribool down, bool previousState)
{
	if (down)
		callback();
}

void CQuestLabel::showAll(SDL_Surface * to)
{
	if (active)
		CBoundedLabel::showAll (to);
}

CQuestIcon::CQuestIcon (const std::string &bmpname, int x, int y) :
	CPicture (bmpname, x, y)
{
	addUsedEvents(LCLICK);
}

void CQuestIcon::clickLeft(tribool down, bool previousState)
{
	if (down)
		callback();
}

void CQuestIcon::showAll(SDL_Surface * to)
{
	if(bg)
	{
		if(srcRect)
		{
			SDL_Rect srcRectCpy = *srcRect;
			SDL_Rect dstRect = srcRectCpy;
			dstRect.x = pos.x;
			dstRect.y = pos.y;

			CSDL_Ext::blitSurface(bg, &srcRectCpy, to, &dstRect);
		}
		else //TODO: allow blitting with offset correction (center of picture on the center of pos)
		{
			SDL_Rect dstRect = pos;
			dstRect.x -= pos.w + 2;
			dstRect.y -= pos.h + 2;
			blitAt(bg, dstRect, to);
		}
	}
}

CQuestMinimap::CQuestMinimap (const Rect & position) :
CMinimap (position),
	currentQuest (NULL)
{
}

void CQuestMinimap::addQuestMarks (const QuestInfo * q)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	icons.clear();

	int3 tile;
	if (q->obj)
	{
		tile = q->obj->pos;
	}
	else
	{
		tile = q->tile;
	}
	CQuestIcon * pic = new CQuestIcon ("", 0, 0);
	CDefHandler * def = CDefHandler::giveDef("VwSymbol.def");
	CSDL_Ext::alphaTransform(def->ourImages[3].bitmap);
	pic->bg = def->ourImages[3].bitmap;
	pic->pos.w = 8;
	pic->pos.h = 8;

	int x, y;
	minimap->tileToPixels (tile, x, y);
	pic->moveTo (Point (minimap->pos.x, minimap->pos.y), true);
	pic->pos.x += x - pic->pos.w / 2 - 1;
	pic->pos.y += y - pic->pos.h / 2 - 1;

	pic->callback = boost::bind (&CQuestMinimap::iconClicked, this);
	icons.push_back(pic);
}

void CQuestMinimap::update()
{
	CMinimap::update();
	if (currentQuest)
		addQuestMarks (currentQuest);
}

void CQuestMinimap::iconClicked()
{
	if (currentQuest->obj)
		adventureInt->centerOn (currentQuest->obj->pos);
	moveAdvMapSelection();
}

void CQuestMinimap::showAll(SDL_Surface * to)
{
	CMinimap::showAll(to);
	BOOST_FOREACH (auto pic, icons)
		pic->showAll(to);
}

CQuestLog::CQuestLog (const std::vector<QuestInfo> & Quests) :
	CWindowObject(PLAYER_COLORED, "QuestLog.pcx"),
	quests (Quests), slider (NULL),
	questIndex(0), currentQuest(NULL)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init();
}

void CQuestLog::init()
{
	minimap = new CQuestMinimap (Rect (47, 33, 144, 144));
	description = new CTextBox ("", Rect(245, 33, 350, 355), 1, FONT_MEDIUM, TOPLEFT, Colors::Cornsilk);
	ok = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, boost::bind(&CQuestLog::close,this), 547, 401, "IOKAY.DEF", SDLK_RETURN);

	if (quests.size() > QUEST_COUNT)
		slider = new CSlider(203, 199, 230, boost::bind (&CQuestLog::sliderMoved, this, _1), QUEST_COUNT, quests.size(), false, 0);

	auto map = LOCPLINT->cb->getVisibilityMap(); //TODO: another function to get all tiles?

	for (int g = 0; g < map.size(); ++g)
		for (int h = 0; h < map[g].size(); ++h)
			for (int y = 0; y < map[g][h].size(); ++y)
				minimap->showTile (int3 (g, h, y));

	for (int i = 0; i < quests.size(); ++i)
	{
		MetaString text;
		quests[i].quest->getRolloverText (text, false);
		if (quests[i].obj)
			text.addReplacement (quests[i].obj->getHoverText()); //get name of the object
		CQuestLabel * label = new CQuestLabel (28, 199 + i * 24, FONT_SMALL, TOPLEFT, Colors::Cornsilk, text.toString());
		label->callback = boost::bind(&CQuestLog::selectQuest, this, i);
		label->setBounds (172, 30);
		labels.push_back(label);
	}

	recreateQuestList (0);
	showAll (screen2);
}

void CQuestLog::showAll(SDL_Surface * to)
{
	CIntObject::showAll (to);
	BOOST_FOREACH (auto label, labels)
	{
		label->show(to); //shows only if active
	}
	if (labels.size() && labels[questIndex]->active)
	{
		CSDL_Ext::drawBorder(to, Rect::around(labels[questIndex]->pos), int3(Colors::MetallicGold.r, Colors::MetallicGold.g, Colors::MetallicGold.b));
	}
	description->show(to);
	minimap->update();
	minimap->show(to);
}

void CQuestLog::recreateQuestList (int newpos)
{
	for (int i = 0; i < labels.size(); ++i)
	{
		labels[i]->pos = Rect (pos.x + 28, pos.y + 207 + (i-newpos) * 25, 173, 23);
		if (i >= newpos && i < newpos + QUEST_COUNT)
		{
			labels[i]->activate();
		}
		else
		{
			labels[i]->deactivate();
		}
	}
}

void CQuestLog::selectQuest (int which)
{
	questIndex = which;
	currentQuest = &quests[which];
	minimap->currentQuest = currentQuest;

	MetaString text;
	std::vector<Component> components; //TODO: display them
	currentQuest->quest->getVisitText (text, components , currentQuest->quest->isCustomFirst, true);
	description->setTxt (text.toString()); //TODO: use special log entry text
	redraw();
}

void CQuestLog::sliderMoved (int newpos)
{
	recreateQuestList (newpos); //move components
	redraw();
}