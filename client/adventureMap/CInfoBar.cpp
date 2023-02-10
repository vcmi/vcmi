/*
 * CInfoBar.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CInfoBar.h"

#include "CAdvMapInt.h"

#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/MiscWidgets.h"
#include "../windows/InfoWindows.h"
#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"

#include "../../CCallback.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

CInfoBar::CVisibleInfo::CVisibleInfo()
	: CIntObject(0, Point(8, 12))
{
}

void CInfoBar::CVisibleInfo::show(SDL_Surface * to)
{
	CIntObject::show(to);
	for(auto object : forceRefresh)
		object->showAll(to);
}

CInfoBar::EmptyVisibleInfo::EmptyVisibleInfo()
{
}

CInfoBar::VisibleHeroInfo::VisibleHeroInfo(const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>("ADSTATHR");
	heroTooltip = std::make_shared<CHeroTooltip>(Point(0,0), hero);
}

CInfoBar::VisibleTownInfo::VisibleTownInfo(const CGTownInstance * town)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>("ADSTATCS");
	townTooltip = std::make_shared<CTownTooltip>(Point(0,0), town);
}

CInfoBar::VisibleDateInfo::VisibleDateInfo()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	animation = std::make_shared<CShowableAnim>(1, 0, getNewDayName(), CShowableAnim::PLAY_ONCE, 180);// H3 uses around 175-180 ms per frame

	std::string labelText;
	if(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK) == 1 && LOCPLINT->cb->getDate(Date::DAY) != 1) // monday of any week but first - show new week info
		labelText = CGI->generaltexth->allTexts[63] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::WEEK));
	else
		labelText = CGI->generaltexth->allTexts[64] + " " + boost::lexical_cast<std::string>(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK));

	label = std::make_shared<CLabel>(95, 31, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, labelText);

	forceRefresh.push_back(label);
}

std::string CInfoBar::VisibleDateInfo::getNewDayName()
{
	if(LOCPLINT->cb->getDate(Date::DAY) == 1)
		return "NEWDAY";

	if(LOCPLINT->cb->getDate(Date::DAY) != 1)
		return "NEWDAY";

	switch(LOCPLINT->cb->getDate(Date::WEEK))
	{
	case 1:
		return "NEWWEEK1";
	case 2:
		return "NEWWEEK2";
	case 3:
		return "NEWWEEK3";
	case 4:
		return "NEWWEEK4";
	default:
		return "";
	}
}

CInfoBar::VisibleEnemyTurnInfo::VisibleEnemyTurnInfo(PlayerColor player)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	background = std::make_shared<CPicture>("ADSTATNX");
	banner = std::make_shared<CAnimImage>("CREST58", player.getNum(), 0, 20, 51);
	sand = std::make_shared<CShowableAnim>(99, 51, "HOURSAND", 0, 100); // H3 uses around 100 ms per frame
	glass = std::make_shared<CShowableAnim>(99, 51, "HOURGLAS", CShowableAnim::PLAY_ONCE, 1000); // H3 scales this nicely for AI turn duration, don't have anything like that in vcmi
}

CInfoBar::VisibleGameStatusInfo::VisibleGameStatusInfo()
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	//get amount of halls of each level
	std::vector<int> halls(4, 0);
	for(auto town : LOCPLINT->towns)
	{
		int hallLevel = town->hallLevel();
		//negative value means no village hall, unlikely but possible
		if(hallLevel >= 0)
			halls.at(hallLevel)++;
	}

	std::vector<PlayerColor> allies, enemies;

	//generate list of allies and enemies
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
	{
		if(LOCPLINT->cb->getPlayerStatus(PlayerColor(i), false) == EPlayerStatus::INGAME)
		{
			if(LOCPLINT->cb->getPlayerRelations(LOCPLINT->playerID, PlayerColor(i)) != PlayerRelations::ENEMIES)
				allies.push_back(PlayerColor(i));
			else
				enemies.push_back(PlayerColor(i));
		}
	}

	//generate widgets
	background = std::make_shared<CPicture>("ADSTATIN");
	allyLabel = std::make_shared<CLabel>(10, 106, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[390] + ":");
	enemyLabel = std::make_shared<CLabel>(10, 136, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->allTexts[391] + ":");

	int posx = allyLabel->pos.w + allyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : allies)
	{
		auto image = std::make_shared<CAnimImage>("ITGFLAGS", player.getNum(), 0, posx, 102);
		posx += image->pos.w;
		flags.push_back(image);
	}

	posx = enemyLabel->pos.w + enemyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : enemies)
	{
		auto image = std::make_shared<CAnimImage>("ITGFLAGS", player.getNum(), 0, posx, 132);
		posx += image->pos.w;
		flags.push_back(image);
	}

	for(size_t i=0; i<halls.size(); i++)
	{
		hallIcons.push_back(std::make_shared<CAnimImage>("itmtl", i, 0, 6 + 42 * (int)i , 11));
		if(halls[i])
			hallLabels.push_back(std::make_shared<CLabel>( 26 + 42 * (int)i, 64, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, boost::lexical_cast<std::string>(halls[i])));
	}
}

CInfoBar::VisibleComponentInfo::VisibleComponentInfo(const Component & compToDisplay, std::string message)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	background = std::make_shared<CPicture>("ADSTATOT", 1, 0);

	comp = std::make_shared<CComponent>(compToDisplay);
	comp->moveTo(Point(pos.x+47, pos.y+50));

	text = std::make_shared<CTextBox>(message, Rect(10, 4, 160, 50), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
}

void CInfoBar::playNewDaySound()
{
	if(LOCPLINT->cb->getDate(Date::DAY_OF_WEEK) != 1) // not first day of the week
		CCS->soundh->playSound(soundBase::newDay);
	else if(LOCPLINT->cb->getDate(Date::WEEK) != 1) // not first week in month
		CCS->soundh->playSound(soundBase::newWeek);
	else if(LOCPLINT->cb->getDate(Date::MONTH) != 1) // not first month
		CCS->soundh->playSound(soundBase::newMonth);
	else
		CCS->soundh->playSound(soundBase::newDay);
}

void CInfoBar::reset()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = EMPTY;
	visibleInfo = std::make_shared<EmptyVisibleInfo>();
}

void CInfoBar::showSelection()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	if(adventureInt->selection)
	{
		if(auto hero = dynamic_cast<const CGHeroInstance *>(adventureInt->selection))
		{
			showHeroSelection(hero);
			return;
		}
		else if(auto town = dynamic_cast<const CGTownInstance *>(adventureInt->selection))
		{
			showTownSelection(town);
			return;
		}
	}
	showGameStatus();//FIXME: may be incorrect but shouldn't happen in general
}

void CInfoBar::tick()
{
	removeUsedEvents(TIME);
	if(GH.topInt() == adventureInt)
		showSelection();
}

void CInfoBar::clickLeft(tribool down, bool previousState)
{
	if(down)
	{
		if(state == HERO || state == TOWN)
			showGameStatus();
		else if(state == GAME)
			showDate();
		else
			showSelection();
	}
}

void CInfoBar::clickRight(tribool down, bool previousState)
{
	if (down)
		CRClickPopup::createAndPush(CGI->generaltexth->allTexts[109]);
}

void CInfoBar::hover(bool on)
{
	if(on)
		GH.statusbar->write(CGI->generaltexth->zelp[292].first);
	else
		GH.statusbar->clear();
}

CInfoBar::CInfoBar(const Rect & position)
	: CIntObject(LCLICK | RCLICK | HOVER, position.topLeft()),
	state(EMPTY)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	pos.w = position.w;
	pos.h = position.h;
	reset();
}

void CInfoBar::showDate()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	playNewDaySound();
	state = DATE;
	visibleInfo = std::make_shared<VisibleDateInfo>();
	setTimer(3000); // confirmed to match H3
	redraw();
}

void CInfoBar::showComponent(const Component & comp, std::string message)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = COMPONENT;
	visibleInfo = std::make_shared<VisibleComponentInfo>(comp, message);
	setTimer(3000);
	redraw();
}

void CInfoBar::startEnemyTurn(PlayerColor color)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = AITURN;
	visibleInfo = std::make_shared<VisibleEnemyTurnInfo>(color);
	redraw();
}

void CInfoBar::showHeroSelection(const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	if(!hero)
	{
		reset();
	}
	else
	{
		state = HERO;
		visibleInfo = std::make_shared<VisibleHeroInfo>(hero);
	}
	redraw();
}

void CInfoBar::showTownSelection(const CGTownInstance * town)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	if(!town)
	{
		reset();
	}
	else
	{
		state = TOWN;
		visibleInfo = std::make_shared<VisibleTownInfo>(town);
	}
	redraw();
}

void CInfoBar::showGameStatus()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	state = GAME;
	visibleInfo = std::make_shared<VisibleGameStatusInfo>();
	setTimer(3000);
	redraw();
}

