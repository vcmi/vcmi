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

struct QuestInfo;
class CAdvmapInterface;

void CQuestLabel::clickLeft(tribool down, bool previousState)
{
	if (down)
		callback();
}

void CQuestMinimap::clickLeft(tribool down, bool previousState)
{
	if (down)
	{
		moveAdvMapSelection();
		update();
	}
}

void CQuestMinimap::update()
{
	CMinimap::update();
	if (currentQuest)
		addQuestMarks (currentQuest);
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
	description = new CTextBox ("", Rect(240, 33, 355, 355), 1, FONT_SMALL, TOPLEFT, Colors::Cornsilk);
	ok = new CAdventureMapButton("",CGI->generaltexth->zelp[445].second, boost::bind(&CQuestLog::close,this), 547, 401, "IOKAY.DEF", SDLK_RETURN);

	if (quests.size() > QUEST_COUNT)
		slider = new CSlider(203, 199, 230, boost::bind (&CQuestLog::sliderMoved, this, _1), quests.size(), quests.size(), false, 0);

	auto map = LOCPLINT->cb->getVisibilityMap(); //TODO: another function to get all tiles?

	for (int g = 0; g < map.size(); ++g)
		for (int h = 0; h < map[g].size(); ++h)
			for (int y = 0; y < map[g][h].size(); ++y)
				minimap->showTile (int3 (g, h, y));

	for (int i = 0; i < quests.size(); ++i)
	{
		CQuestLabel * label = new CQuestLabel (28, 199 + i * 24, FONT_SMALL, TOPLEFT, Colors::Cornsilk, quests[i].quest.firstVisitText);
		label->callback = boost::bind(&CQuestLog::selectQuest, this, i);
		labels.push_back(label);
	}

	recreateQuestList (0); //truncate invisible and too wide labels
	showAll (screen2);
}

void CQuestLog::showAll(SDL_Surface * to)
{
	CIntObject::showAll (to);
	recreateQuestList (0);
	BOOST_FOREACH (auto label, labels)
	{
		if (label->active)
			label->show(to);
	}
	if (labels.size())
		CSDL_Ext::drawBorder(to, Rect::around(labels[questIndex]->pos), int3(Colors::MetallicGold.r, Colors::MetallicGold.g, Colors::MetallicGold.b));
	description->show(to);
	minimap->update();
}

void CQuestLog::recreateQuestList (int newpos)
{
	for (int i = 0; i < labels.size(); ++i)
	{
		if (i >= newpos && i < newpos + QUEST_COUNT)
		{
			labels[i]->pos = Rect (pos.x + 28, pos.y + 207 + (i-newpos) * 24, 172, 30); //TODO: limit label width?
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
	if (currentQuest->obj)
	{
		//minimap->setLevel (currentQuest->obj->pos.z);
		adventureInt->centerOn (currentQuest->obj->pos);
	}
	description->text = currentQuest->quest.firstVisitText; //TODO: use special log entry text
	redraw();
}

void CQuestLog::sliderMoved (int newpos)
{
	recreateQuestList (newpos); //move components
	redraw();
}