#include "StdInc.h"
#include "CQuestLog.h"

#include "CAdvmapInterface.h"

#include "../CBitmapHandler.h"
#include "../CDefHandler.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../Graphics.h"

#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"

#include "../../CCallback.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/NetPacksBase.h"

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
		CMultiLineLabel::showAll (to);
}

CQuestIcon::CQuestIcon (const std::string &defname, int index, int x, int y) :
	CAnimImage(defname, index, 0, x, y)
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
	CSDL_Ext::CClipRectGuard guard(to, parent->pos);
	CAnimImage::showAll(to);
}

CQuestMinimap::CQuestMinimap (const Rect & position) :
CMinimap (position),
	currentQuest (nullptr)
{
}

void CQuestMinimap::addQuestMarks (const QuestInfo * q)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	for (auto icon : icons)
		delete icon;
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
	int x,y;
	minimap->tileToPixels (tile, x, y);

	CQuestIcon * pic = new CQuestIcon ("VwSymbol.def", 3, x, y);

	pic->moveBy (Point ( -pic->pos.w/2, -pic->pos.h/2));
	pic->callback = std::bind (&CQuestMinimap::iconClicked, this);

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
	//moveAdvMapSelection();
}

void CQuestMinimap::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to); // blitting IntObject directly to hide radar
	for (auto pic : icons)
		pic->showAll(to);
}

CQuestLog::CQuestLog (const std::vector<QuestInfo> & Quests) :
	CWindowObject(PLAYER_COLORED | BORDERED, "questDialog.pcx"),
	questIndex(0),
	currentQuest(nullptr),
	quests (Quests),
	slider(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init();
}

void CQuestLog::init()
{
	minimap = new CQuestMinimap (Rect (33, 18, 144, 144));
	description = new CTextBox ("", Rect(221, 18, 350, 355), 1, FONT_MEDIUM, TOPLEFT, Colors::WHITE);
	ok = new CButton(Point(533, 386), "IOKAY.DEF", CGI->generaltexth->zelp[445], boost::bind(&CQuestLog::close,this), SDLK_RETURN);

	if (quests.size() > QUEST_COUNT)
		slider = new CSlider(Point(189, 184), 230, std::bind (&CQuestLog::sliderMoved, this, _1), QUEST_COUNT, quests.size(), false, CSlider::BROWN);

	for (int i = 0; i < quests.size(); ++i)
	{
		MetaString text;
		quests[i].quest->getRolloverText (text, false);
		if (quests[i].obj)
			text.addReplacement (quests[i].obj->getObjectName()); //get name of the object
		CQuestLabel * label = new CQuestLabel (Rect(14, 184 + i * 24, 172,30), FONT_SMALL, TOPLEFT, Colors::WHITE, text.toString());
		label->callback = boost::bind(&CQuestLog::selectQuest, this, i);
		labels.push_back(label);
	}

	recreateQuestList (0);
}

void CQuestLog::showAll(SDL_Surface * to)
{
	CWindowObject::showAll (to);
	for (auto label : labels)
	{
		label->show(to); //shows only if active
	}
	if (labels.size() && labels[questIndex]->active)
	{
		CSDL_Ext::drawBorder(to, Rect::around(labels[questIndex]->pos), int3(Colors::METALLIC_GOLD.r, Colors::METALLIC_GOLD.g, Colors::METALLIC_GOLD.b));
	}
	description->show(to);
	minimap->show(to);
}

void CQuestLog::recreateQuestList (int newpos)
{
	for (int i = 0; i < labels.size(); ++i)
	{
		labels[i]->pos = Rect (pos.x + 14, pos.y + 192 + (i-newpos) * 25, 173, 23);
		if (i >= newpos && i < newpos + QUEST_COUNT)
		{
			labels[i]->activate();
		}
		else
		{
			labels[i]->deactivate();
		}
	}
	minimap->update();
}

void CQuestLog::selectQuest (int which)
{
	questIndex = which;
	currentQuest = &quests[which];
	minimap->currentQuest = currentQuest;

	MetaString text;
	std::vector<Component> components; //TODO: display them
	currentQuest->quest->getVisitText (text, components , currentQuest->quest->isCustomFirst, true);
	description->setText (text.toString()); //TODO: use special log entry text
	minimap->update();
	redraw();
}

void CQuestLog::sliderMoved (int newpos)
{
	recreateQuestList (newpos); //move components
	redraw();
}
