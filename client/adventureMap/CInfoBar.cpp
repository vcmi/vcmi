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

#include "AdventureMapInterface.h"

#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../windows/CMessage.h"
#include "../widgets/TextControls.h"
#include "../widgets/MiscWidgets.h"
#include "../windows/InfoWindows.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../media/ISoundPlayer.h"
#include "../render/IScreenHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"

CInfoBar::CVisibleInfo::CVisibleInfo()
	: CIntObject(0, Point(offset_x, offset_y))
{
}

void CInfoBar::CVisibleInfo::show(Canvas & to)
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
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin("ADSTATHR"));

	if(settings["gameTweaks"]["infoBarCreatureManagement"].Bool())
		heroTooltip = std::make_shared<CInteractableHeroTooltip>(Point(0,0), hero);
	else
		heroTooltip = std::make_shared<CHeroTooltip>(Point(0,0), hero);
}

CInfoBar::VisibleTownInfo::VisibleTownInfo(const CGTownInstance * town)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin("ADSTATCS"));

	if(settings["gameTweaks"]["infoBarCreatureManagement"].Bool())
		townTooltip = std::make_shared<CInteractableTownTooltip>(Point(0,0), town);
	else
		townTooltip = std::make_shared<CTownTooltip>(Point(0,0), town);
}

CInfoBar::VisibleDateInfo::VisibleDateInfo()
{
	OBJECT_CONSTRUCTION;

	animation = std::make_shared<CShowableAnim>(1, 0, getNewDayName(), CShowableAnim::PLAY_ONCE, 180);// H3 uses around 175-180 ms per frame
	animation->setDuration(1500);

	std::string labelText;
	if(GAME->interface()->cb->getDate(Date::DAY_OF_WEEK) == 1 && GAME->interface()->cb->getDate(Date::DAY) != 1) // monday of any week but first - show new week info
		labelText = LIBRARY->generaltexth->allTexts[63] + " " + std::to_string(GAME->interface()->cb->getDate(Date::WEEK));
	else
		labelText = LIBRARY->generaltexth->allTexts[64] + " " + std::to_string(GAME->interface()->cb->getDate(Date::DAY_OF_WEEK));

	label = std::make_shared<CLabel>(95, 31, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, labelText);

	forceRefresh.push_back(label);
}

AnimationPath CInfoBar::VisibleDateInfo::getNewDayName()
{
	if(GAME->interface()->cb->getDate(Date::DAY) == 1)
		return AnimationPath::builtin("NEWDAY");

	if(GAME->interface()->cb->getDate(Date::DAY_OF_WEEK) != 1)
		return AnimationPath::builtin("NEWDAY");

	switch(GAME->interface()->cb->getDate(Date::WEEK))
	{
	case 1:
		return AnimationPath::builtin("NEWWEEK1");
	case 2:
		return AnimationPath::builtin("NEWWEEK2");
	case 3:
		return AnimationPath::builtin("NEWWEEK3");
	case 4:
		return AnimationPath::builtin("NEWWEEK4");
	default:
		return AnimationPath();
	}
}

CInfoBar::VisibleEnemyTurnInfo::VisibleEnemyTurnInfo(PlayerColor player)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin("ADSTATNX"));
	banner = std::make_shared<CAnimImage>(AnimationPath::builtin("CREST58"), player.getNum(), 0, 20, 51);
	sand = std::make_shared<CShowableAnim>(99, 51, AnimationPath::builtin("HOURSAND"), 0, 100); // H3 uses around 100 ms per frame
	glass = std::make_shared<CShowableAnim>(99, 51, AnimationPath::builtin("HOURGLAS"), CShowableAnim::PLAY_ONCE, 1000); // H3 scales this nicely for AI turn duration, don't have anything like that in vcmi
}

CInfoBar::VisibleGameStatusInfo::VisibleGameStatusInfo()
{
	OBJECT_CONSTRUCTION;
	//get amount of halls of each level
	std::vector<int> halls(4, 0);
	for(auto town : GAME->interface()->localState->getOwnedTowns())
	{
		int hallLevel = town->hallLevel();
		//negative value means no village hall, unlikely but possible
		if(hallLevel >= 0)
			halls.at(hallLevel)++;
	}

	std::vector<PlayerColor> allies;
	std::vector<PlayerColor> enemies;

	//generate list of allies and enemies
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
	{
		if(GAME->interface()->cb->getPlayerStatus(PlayerColor(i), false) == EPlayerStatus::INGAME)
		{
			if(GAME->interface()->cb->getPlayerRelations(GAME->interface()->playerID, PlayerColor(i)) != PlayerRelations::ENEMIES)
				allies.push_back(PlayerColor(i));
			else
				enemies.push_back(PlayerColor(i));
		}
	}

	//generate widgets
	background = std::make_shared<CPicture>(ImagePath::builtin("ADSTATIN"));
	allyLabel = std::make_shared<CLabel>(10, 106, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[390] + ":");
	enemyLabel = std::make_shared<CLabel>(10, 136, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[391] + ":");

	int posx = allyLabel->pos.w + allyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : allies)
	{
		auto image = std::make_shared<CAnimImage>(AnimationPath::builtin("ITGFLAGS"), player.getNum(), 0, posx, 102);
		posx += image->pos.w;
		flags.push_back(image);
	}

	posx = enemyLabel->pos.w + enemyLabel->pos.x - pos.x + 4;
	for(PlayerColor & player : enemies)
	{
		auto image = std::make_shared<CAnimImage>(AnimationPath::builtin("ITGFLAGS"), player.getNum(), 0, posx, 132);
		posx += image->pos.w;
		flags.push_back(image);
	}

	for(size_t i=0; i<halls.size(); i++)
	{
		hallIcons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("itmtl"), i, 0, 6 + 42 * (int)i , 11));
		if(halls[i])
			hallLabels.push_back(std::make_shared<CLabel>( 26 + 42 * (int)i, 64, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, std::to_string(halls[i])));
	}
}

CInfoBar::VisibleComponentInfo::VisibleComponentInfo(const std::vector<Component> & compsToDisplay, std::string message, int textH, bool tiny)
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CPicture>(ImagePath::builtin("ADSTATOT"), 1, 0);
	auto fullRect = Rect(CInfoBar::offset, CInfoBar::offset, data_width - 2 * CInfoBar::offset, data_height - 2 * CInfoBar::offset);
	auto textRect = fullRect;
	auto imageRect = fullRect;
	auto font = tiny ? FONT_TINY : FONT_SMALL;
	auto maxComponents = 2; 

	if(!compsToDisplay.empty())
	{
		auto size = CComponent::large;
		if(compsToDisplay.size() > 2)
		{
			size = CComponent::medium;
			font = FONT_TINY;
		}
		if(!message.empty())
		{
			textRect = Rect(CInfoBar::offset,
							CInfoBar::offset,
							data_width - 2 * CInfoBar::offset,
							textH);
			imageRect = Rect(CInfoBar::offset,
							 textH,
							 data_width - 2 * CInfoBar::offset,
							 CInfoBar::data_height - 2* CInfoBar::offset - textH);
		}
		
		if(compsToDisplay.size() > 4) {
			maxComponents = 3; 
			size = CComponent::small;
		}
		if(compsToDisplay.size() > 6)
			maxComponents = 4;

		std::vector<std::shared_ptr<CComponent>> vect;

		for(const auto & c : compsToDisplay)
			vect.emplace_back(std::make_shared<CComponent>(c, size, font));

		comps = std::make_shared<CComponentBox>(vect, imageRect, 4, 4, 1, maxComponents);
	}

	if(!message.empty())
		text = std::make_shared<CMultiLineLabel>(textRect, font, ETextAlignment::CENTER, Colors::WHITE, message);
}

void CInfoBar::playNewDaySound()
{
	int volume = ENGINE->sound().getVolume();
	int handle = -1;
	if(volume == 0)
		ENGINE->sound().setVolume(settings["general"]["sound"].Integer());

	if(GAME->interface()->cb->getDate(Date::DAY_OF_WEEK) != 1) // not first day of the week
		handle = ENGINE->sound().playSound(soundBase::newDay);
	else if(GAME->interface()->cb->getDate(Date::WEEK) != 1) // not first week in month
		handle = ENGINE->sound().playSound(soundBase::newWeek);
	else if(GAME->interface()->cb->getDate(Date::MONTH) != 1) // not first month
		handle = ENGINE->sound().playSound(soundBase::newMonth);
	else
		handle = ENGINE->sound().playSound(soundBase::newDay);

	if(volume == 0)
		ENGINE->sound().setCallback(handle, [&]() { if(!ENGINE->screenHandler().hasFocus()) ENGINE->sound().setVolume(0); });
}

void CInfoBar::reset()
{
	OBJECT_CONSTRUCTION;
	state = EState::EMPTY;
	visibleInfo = std::make_shared<EmptyVisibleInfo>();
}

void CInfoBar::showSelection()
{
	OBJECT_CONSTRUCTION;
	if(GAME->interface()->localState->getCurrentHero())
	{
		showHeroSelection(GAME->interface()->localState->getCurrentHero());
		return;
	}

	if(GAME->interface()->localState->getCurrentTown())
	{
		showTownSelection(GAME->interface()->localState->getCurrentTown());
		return;
	}

	showGameStatus();//FIXME: may be incorrect but shouldn't happen in general
}

void CInfoBar::tick(uint32_t msPassed)
{
	assert(timerCounter > 0);

	if (msPassed >= timerCounter)
	{
		timerCounter = 0;
		removeUsedEvents(TIME);
		if(ENGINE->windows().isTopWindow(adventureInt))
			popComponents(true);
	}
	else
	{
		timerCounter -= msPassed;
	}
}

void CInfoBar::clickReleased(const Point & cursorPosition, bool lastActivated)
{
	timerCounter = 0;
	removeUsedEvents(TIME); //expiration trigger from just clicked element is not valid anymore

	if(state == EState::HERO || state == EState::TOWN)
	{
		if(lastActivated)
			showGameStatus();
	}
	else if(state == EState::GAME)
		showDate();
	else
		popComponents(true);
}

void CInfoBar::showPopupWindow(const Point & cursorPosition)
{
	CRClickPopup::createAndPush(LIBRARY->generaltexth->allTexts[109]);
}

void CInfoBar::hover(bool on)
{
	if(on)
		ENGINE->statusbar()->write(LIBRARY->generaltexth->zelp[292].first);
	else
		ENGINE->statusbar()->clear();
}

CInfoBar::CInfoBar(const Rect & position)
	: CIntObject(LCLICK | SHOW_POPUP | HOVER, position.topLeft()),
	timerCounter(0),
	state(EState::EMPTY),
	listener(settings.listen["gameTweaks"]["infoBarCreatureManagement"])
{
	OBJECT_CONSTRUCTION;
	pos.w = position.w;
	pos.h = position.h;
	listener(std::bind(&CInfoBar::OnInfoBarCreatureManagementChanged, this));
	reset();
}

CInfoBar::CInfoBar(const Point & position): CInfoBar(Rect(position.x, position.y, width, height))
{
}

void CInfoBar::OnInfoBarCreatureManagementChanged()
{
	showSelection();
}

void CInfoBar::setTimer(uint32_t msToTrigger)
{
	addUsedEvents(TIME);
	timerCounter = msToTrigger;
}

void CInfoBar::showDate()
{
	OBJECT_CONSTRUCTION;
	playNewDaySound();
	state = EState::DATE;
	visibleInfo = std::make_shared<VisibleDateInfo>();
	setTimer(3000); // confirmed to match H3
	redraw();
}

void CInfoBar::pushComponents(const std::vector<Component> & components, std::string message, int timer)
{
	auto actualPush = [&](const std::vector<Component> & components, std::string message, int timer, size_t max){
		std::vector<Component> vect = components; //I do not know currently how to avoid copy here
		while(!vect.empty())
		{
			std::vector<Component> sender =  {vect.begin(), vect.begin() + std::min(vect.size(), max)};
			prepareComponents(sender, message, timer);
			vect.erase(vect.begin(), vect.begin() + std::min(vect.size(), max));
		};
	};
	if(shouldPopAll)
		popAll();
	if(components.empty())
		prepareComponents(components, message, timer);
	else
	{
		std::array<std::pair<std::vector<Component>, int>, 10> reward_map;
		for(const auto & c : components)
		{
			switch(c.type)
			{
				case ComponentType::PRIM_SKILL:
				case ComponentType::EXPERIENCE:
				case ComponentType::LEVEL:
				case ComponentType::MANA:
					reward_map.at(0).first.push_back(c);
					reward_map.at(0).second = 8; //At most 8, cannot be more
					break;
				case ComponentType::NONE:
				case ComponentType::SEC_SKILL:
					reward_map.at(1).first.push_back(c);
					reward_map.at(1).second = 4; //At most 4
					break;
				case ComponentType::SPELL:
					reward_map.at(2).first.push_back(c);
					reward_map.at(2).second = 4; //At most 4
					break;
				case ComponentType::ARTIFACT:
				case ComponentType::SPELL_SCROLL:
					reward_map.at(3).first.push_back(c);
					reward_map.at(3).second = 4; //At most 4, too long names
					break;
				case ComponentType::CREATURE:
					reward_map.at(4).first.push_back(c);
					reward_map.at(4).second = 4; //At most 4, too long names
					break;
				case ComponentType::RESOURCE:
				case ComponentType::RESOURCE_PER_DAY:
					reward_map.at(5).first.push_back(c);
					reward_map.at(5).second = 7; //At most 7
					break;
				case ComponentType::MORALE:
				case ComponentType::LUCK:
					reward_map.at(6).first.push_back(c);
					reward_map.at(6).second = 2; //At most 2 - 1 for morale + 1 for luck
					break;
				case ComponentType::BUILDING:
					reward_map.at(7).first.push_back(c);
					reward_map.at(7).second = 1; //At most 1 - only large icons available AFAIK
					break;
				case ComponentType::HERO_PORTRAIT:
					reward_map.at(8).first.push_back(c);
					reward_map.at(8).second = 1; //I do not think than we even can get more than 1 hero
					break;
				case ComponentType::FLAG:
					reward_map.at(9).first.push_back(c);
					reward_map.at(9).second = 1; //I do not think than we even can get more than 1 player in notification
					break;
				default:
					logGlobal->warn("Invalid component received!");
			}
		}
		
		for(const auto & kv : reward_map)
			if(!kv.first.empty())
				actualPush(kv.first, message, timer, kv.second);
	}
	popComponents();
}

void CInfoBar::prepareComponents(const std::vector<Component> & components, std::string message, int timer)
{
	auto imageH = CMessage::getEstimatedComponentHeight(components.size()) + (components.empty() ? 0 : 2 * CInfoBar::offset);
	auto textH = CMessage::guessHeight(message,CInfoBar::data_width - 2 * CInfoBar::offset, FONT_SMALL);
	auto tinyH = CMessage::guessHeight(message,CInfoBar::data_width - 2 * CInfoBar::offset, FONT_TINY);
	auto header = CMessage::guessHeader(message);
	auto headerH = CMessage::guessHeight(header, CInfoBar::data_width - 2 * CInfoBar::offset, FONT_SMALL);
	auto headerTinyH = CMessage::guessHeight(header, CInfoBar::data_width - 2 * CInfoBar::offset, FONT_TINY);

	// Order matters - priority form should be chosen first
	if(imageH + textH < CInfoBar::data_height)
		pushComponents(components, message, textH, false, timer);
	else if(imageH + tinyH < CInfoBar::data_height)
		pushComponents(components, message, tinyH, true, timer);
	else if(imageH + headerH < CInfoBar::data_height)
		pushComponents(components, header, headerH, false, timer);
	else if(imageH + headerTinyH < CInfoBar::data_height)
		pushComponents(components, header, headerTinyH, true, timer);
	else
		pushComponents(components, "", 0, false, timer);

	return;
}

void CInfoBar::requestPopAll()
{
	shouldPopAll = true;
}

void CInfoBar::popAll()
{
	componentsQueue = {};
	shouldPopAll = false;
}

void CInfoBar::popComponents(bool remove)
{
	OBJECT_CONSTRUCTION;
	if(remove && !componentsQueue.empty())
		componentsQueue.pop();
	if(!componentsQueue.empty())
	{
		state = EState::COMPONENT;
		const auto & extracted = componentsQueue.front();
		visibleInfo = std::make_shared<VisibleComponentInfo>(extracted.first);
		setTimer(extracted.second);
		redraw();
		return;
	}
	showSelection();
}

void CInfoBar::pushComponents(const std::vector<Component> & comps, std::string message, int textH, bool tiny, int timer)
{
	OBJECT_CONSTRUCTION;
	componentsQueue.emplace(VisibleComponentInfo::Cache(comps, message, textH, tiny), timer);
}

bool CInfoBar::showingComponents()
{
	return state == EState::COMPONENT;
}

void CInfoBar::startEnemyTurn(PlayerColor color)
{
	OBJECT_CONSTRUCTION;
	state = EState::AITURN;
	visibleInfo = std::make_shared<VisibleEnemyTurnInfo>(color);
	redraw();
}

void CInfoBar::showHeroSelection(const CGHeroInstance * hero)
{
	OBJECT_CONSTRUCTION;
	if(!hero)
	{
		reset();
	}
	else
	{
		state = EState::HERO;
		visibleInfo = std::make_shared<VisibleHeroInfo>(hero);
	}
	redraw();
}

void CInfoBar::showTownSelection(const CGTownInstance * town)
{
	OBJECT_CONSTRUCTION;
	if(!town)
	{
		reset();
	}
	else
	{
		state = EState::TOWN;
		visibleInfo = std::make_shared<VisibleTownInfo>(town);
	}
	redraw();
}

void CInfoBar::showGameStatus()
{
	OBJECT_CONSTRUCTION;
	state = EState::GAME;
	visibleInfo = std::make_shared<VisibleGameStatusInfo>();
	setTimer(3000);
	redraw();
}

