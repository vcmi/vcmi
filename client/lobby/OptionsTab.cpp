/*
 * OptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "OptionsTab.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CServerHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/NetPacks.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CHeroHandler.h"

OptionsTab::OptionsTab()
	: turnDuration(nullptr)
{
	OBJ_CONSTRUCTION;
	bg = new CPicture("ADVOPTBK", 0, 6);
	pos = bg->pos;

	if(SEL->screenType == ESelectionScreen::newGame || SEL->screenType == ESelectionScreen::loadGame || SEL->screenType == ESelectionScreen::scenarioInfo)
		turnDuration = new CSlider(Point(55, 551), 194, std::bind(&IServerAPI::setTurnLength, CSH, _1), 1, ARRAY_COUNT(GameConstants::POSSIBLE_TURNTIME), ARRAY_COUNT(GameConstants::POSSIBLE_TURNTIME), true, CSlider::BLUE);
}

void OptionsTab::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printAtMiddleLoc(CGI->generaltexth->allTexts[515], 222, 30, FONT_BIG, Colors::YELLOW, to);
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[516], 222, 68, FONT_SMALL, 300, Colors::WHITE, to); //Select starting options, handicap, and name for each player in the game.
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[517], 107, 110, FONT_SMALL, 100, Colors::YELLOW, to); //Player Name Handicap Type
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[518], 197, 110, FONT_SMALL, 70, Colors::YELLOW, to); //Starting Town
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[519], 273, 110, FONT_SMALL, 70, Colors::YELLOW, to); //Starting Hero
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[520], 349, 110, FONT_SMALL, 70, Colors::YELLOW, to); //Starting Bonus
	printAtMiddleLoc(CGI->generaltexth->allTexts[521], 222, 538, FONT_SMALL, Colors::YELLOW, to); // Player Turn Duration
	if(turnDuration)
		printAtMiddleLoc(CGI->generaltexth->turnDurations[turnDuration->getValue()], 319, 559, FONT_SMALL, Colors::WHITE, to); //Turn duration value
}

void OptionsTab::recreate()
{
	for(auto & elem : entries)
	{
		delete elem.second;
	}
	entries.clear();

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	for(auto it = SEL->getStartInfo()->playerInfos.begin(); it != SEL->getStartInfo()->playerInfos.end(); ++it)
	{
		entries.insert(std::make_pair(it->first, new PlayerOptionsEntry(this, it->second)));
	}

	if(turnDuration)
		turnDuration->moveTo(std::distance(GameConstants::POSSIBLE_TURNTIME, std::find(GameConstants::POSSIBLE_TURNTIME, GameConstants::POSSIBLE_TURNTIME + ARRAY_COUNT(GameConstants::POSSIBLE_TURNTIME), SEL->getStartInfo()->turnTime)));
}

size_t OptionsTab::CPlayerSettingsHelper::getImageIndex()
{
	enum EBonusSelection //frames of bonuses file
	{
		WOOD_ORE = 0,   CRYSTAL = 1,    GEM  = 2,
		MERCURY  = 3,   SULFUR  = 5,    GOLD = 8,
		ARTIFACT = 9,   RANDOM  = 10,
		WOOD = 0,       ORE     = 0,    MITHRIL = 10, // resources unavailable in bonuses file

		TOWN_RANDOM = 38,  TOWN_NONE = 39, // Special frames in ITPA
		HERO_RANDOM = 163, HERO_NONE = 164 // Special frames in PortraitsSmall
	};

	switch(type)
	{
	case TOWN:
		switch(settings.castle)
		{
		case PlayerSettings::NONE:
			return TOWN_NONE;
		case PlayerSettings::RANDOM:
			return TOWN_RANDOM;
		default:
			return CGI->townh->factions[settings.castle]->town->clientInfo.icons[true][false] + 2;
		}

	case HERO:
		switch(settings.hero)
		{
		case PlayerSettings::NONE:
			return HERO_NONE;
		case PlayerSettings::RANDOM:
			return HERO_RANDOM;
		default:
		{
			if(settings.heroPortrait >= 0)
				return settings.heroPortrait;
			return CGI->heroh->heroes[settings.hero]->imageIndex;
		}
		}

	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return RANDOM;
		case PlayerSettings::ARTIFACT:
			return ARTIFACT;
		case PlayerSettings::GOLD:
			return GOLD;
		case PlayerSettings::RESOURCE:
		{
			switch(CGI->townh->factions[settings.castle]->town->primaryRes)
			{
			case Res::WOOD_AND_ORE:
				return WOOD_ORE;
			case Res::WOOD:
				return WOOD;
			case Res::MERCURY:
				return MERCURY;
			case Res::ORE:
				return ORE;
			case Res::SULFUR:
				return SULFUR;
			case Res::CRYSTAL:
				return CRYSTAL;
			case Res::GEMS:
				return GEM;
			case Res::GOLD:
				return GOLD;
			case Res::MITHRIL:
				return MITHRIL;
			}
		}
		}
	}
	}
	return 0;
}

std::string OptionsTab::CPlayerSettingsHelper::getImageName()
{
	switch(type)
	{
	case OptionsTab::TOWN:
		return "ITPA";
	case OptionsTab::HERO:
		return "PortraitsSmall";
	case OptionsTab::BONUS:
		return "SCNRSTAR";
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getName()
{
	switch(type)
	{
	case TOWN:
	{
		switch(settings.castle)
		{
		case PlayerSettings::NONE:
			return CGI->generaltexth->allTexts[523];
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[522];
		default:
			return CGI->townh->factions[settings.castle]->name;
		}
	}
	case HERO:
	{
		switch(settings.hero)
		{
		case PlayerSettings::NONE:
			return CGI->generaltexth->allTexts[523];
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[522];
		default:
		{
			if(!settings.heroName.empty())
				return settings.heroName;
			return CGI->heroh->heroes[settings.hero]->name;
		}
		}
	}
	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[522];
		default:
			return CGI->generaltexth->arraytxt[214 + settings.bonus];
		}
	}
	}
	return "";
}


std::string OptionsTab::CPlayerSettingsHelper::getTitle()
{
	switch(type)
	{
	case OptionsTab::TOWN:
		return (settings.castle < 0) ? CGI->generaltexth->allTexts[103] : CGI->generaltexth->allTexts[80];
	case OptionsTab::HERO:
		return (settings.hero < 0) ? CGI->generaltexth->allTexts[101] : CGI->generaltexth->allTexts[77];
	case OptionsTab::BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[86]; //{Random Bonus}
		case PlayerSettings::ARTIFACT:
			return CGI->generaltexth->allTexts[83]; //{Artifact Bonus}
		case PlayerSettings::GOLD:
			return CGI->generaltexth->allTexts[84]; //{Gold Bonus}
		case PlayerSettings::RESOURCE:
			return CGI->generaltexth->allTexts[85]; //{Resource Bonus}
		}
	}
	}
	return "";
}
std::string OptionsTab::CPlayerSettingsHelper::getSubtitle()
{
	switch(type)
	{
	case TOWN:
		return getName();
	case HERO:
	{
		if(settings.hero >= 0)
			return getName() + " - " + CGI->heroh->heroes[settings.hero]->heroClass->name;
		return getName();
	}

	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::GOLD:
			return CGI->generaltexth->allTexts[87]; //500-1000
		case PlayerSettings::RESOURCE:
		{
			switch(CGI->townh->factions[settings.castle]->town->primaryRes)
			{
			case Res::MERCURY:
				return CGI->generaltexth->allTexts[694];
			case Res::SULFUR:
				return CGI->generaltexth->allTexts[695];
			case Res::CRYSTAL:
				return CGI->generaltexth->allTexts[692];
			case Res::GEMS:
				return CGI->generaltexth->allTexts[693];
			case Res::WOOD_AND_ORE:
				return CGI->generaltexth->allTexts[89]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
			}
		}
		}
	}
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getDescription()
{
	switch(type)
	{
	case TOWN:
		return CGI->generaltexth->allTexts[104];
	case HERO:
		return CGI->generaltexth->allTexts[102];
	case BONUS:
	{
		switch(settings.bonus)
		{
		case PlayerSettings::RANDOM:
			return CGI->generaltexth->allTexts[94]; //Gold, wood and ore, or an artifact is randomly chosen as your starting bonus
		case PlayerSettings::ARTIFACT:
			return CGI->generaltexth->allTexts[90]; //An artifact is randomly chosen and equipped to your starting hero
		case PlayerSettings::GOLD:
			return CGI->generaltexth->allTexts[92]; //At the start of the game, 500-1000 gold is added to your Kingdom's resource pool
		case PlayerSettings::RESOURCE:
		{
			switch(CGI->townh->factions[settings.castle]->town->primaryRes)
			{
			case Res::MERCURY:
				return CGI->generaltexth->allTexts[690];
			case Res::SULFUR:
				return CGI->generaltexth->allTexts[691];
			case Res::CRYSTAL:
				return CGI->generaltexth->allTexts[688];
			case Res::GEMS:
				return CGI->generaltexth->allTexts[689];
			case Res::WOOD_AND_ORE:
				return CGI->generaltexth->allTexts[93]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
			}
		}
		}
	}
	}
	return "";
}

OptionsTab::CPlayerOptionTooltipBox::CPlayerOptionTooltipBox(CPlayerSettingsHelper & helper)
	: CWindowObject(BORDERED | RCLICK_POPUP), CPlayerSettingsHelper(helper)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	int value = PlayerSettings::NONE;

	switch(CPlayerSettingsHelper::type)
	{
		break;
	case TOWN:
		value = settings.castle;
		break;
	case HERO:
		value = settings.hero;
		break;
	case BONUS:
		value = settings.bonus;
	}

	if(value == PlayerSettings::RANDOM)
		genBonusWindow();
	else if(CPlayerSettingsHelper::type == BONUS)
		genBonusWindow();
	else if(CPlayerSettingsHelper::type == HERO)
		genHeroWindow();
	else if(CPlayerSettingsHelper::type == TOWN)
		genTownWindow();

	center();
}

void OptionsTab::CPlayerOptionTooltipBox::genHeader()
{
	new CFilledTexture("DIBOXBCK", pos);
	updateShadow();

	new CLabel(pos.w / 2 + 8, 21, FONT_MEDIUM, CENTER, Colors::YELLOW, getTitle());

	new CLabel(pos.w / 2, 88, FONT_SMALL, CENTER, Colors::WHITE, getSubtitle());

	new CAnimImage(getImageName(), getImageIndex(), 0, pos.w / 2 - 24, 45);
}

void OptionsTab::CPlayerOptionTooltipBox::genTownWindow()
{
	pos = Rect(0, 0, 228, 290);
	genHeader();

	new CLabel(pos.w / 2 + 8, 122, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[79]);

	std::vector<CComponent *> components;
	const CTown * town = CGI->townh->factions[settings.castle]->town;

	for(auto & elem : town->creatures)
	{
		if(!elem.empty())
			components.push_back(new CComponent(CComponent::creature, elem.front(), 0, CComponent::tiny));
	}

	new CComponentBox(components, Rect(10, 140, pos.w - 20, 140));
}

void OptionsTab::CPlayerOptionTooltipBox::genHeroWindow()
{
	pos = Rect(0, 0, 292, 226);
	genHeader();

	// specialty
	new CAnimImage("UN44", CGI->heroh->heroes[settings.hero]->imageIndex, 0, pos.w / 2 - 22, 134);

	new CLabel(pos.w / 2 + 4, 117, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	new CLabel(pos.w / 2, 188, FONT_SMALL, CENTER, Colors::WHITE, CGI->heroh->heroes[settings.hero]->specName);
}

void OptionsTab::CPlayerOptionTooltipBox::genBonusWindow()
{
	pos = Rect(0, 0, 228, 162);
	genHeader();

	new CTextBox(getDescription(), Rect(10, 100, pos.w - 20, 70), 0, FONT_SMALL, CENTER, Colors::WHITE);
}

OptionsTab::SelectedBox::SelectedBox(Point position, PlayerSettings & settings, SelType type)
	: CIntObject(RCLICK, position), CPlayerSettingsHelper(settings, type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	image = new CAnimImage(getImageName(), getImageIndex());
	subtitle = new CLabel(23, 39, FONT_TINY, CENTER, Colors::WHITE, getName());

	pos = image->pos;
}

void OptionsTab::SelectedBox::update()
{
	image->setFrame(getImageIndex());
	subtitle->setText(getName());
}

void OptionsTab::SelectedBox::clickRight(tribool down, bool previousState)
{
	if(down)
	{
		// cases when we do not need to display a message
		if(settings.castle == -2 && CPlayerSettingsHelper::type == TOWN)
			return;
		if(settings.hero == -2 && !SEL->getPlayerInfo(settings.color.getNum()).hasCustomMainHero() && CPlayerSettingsHelper::type == HERO)
			return;

		GH.pushInt(new CPlayerOptionTooltipBox(*this));
	}
}

OptionsTab::PlayerOptionsEntry::PlayerOptionsEntry(OptionsTab * owner, const PlayerSettings & S)
	: pi(SEL->getPlayerInfo(S.color.getNum())), s(S)
{
	OBJ_CONSTRUCTION;
	defActions |= SHARE_POS;

	int serial = 0;
	for(int g = 0; g < s.color.getNum(); ++g)
	{
		auto itred = SEL->getPlayerInfo(g);
		if(itred.canComputerPlay || itred.canHumanPlay)
			serial++;
	}

	pos.x += 54;
	pos.y += 122 + serial * 50;

	static const char * flags[] =
	{
		"AOFLGBR.DEF", "AOFLGBB.DEF", "AOFLGBY.DEF", "AOFLGBG.DEF",
		"AOFLGBO.DEF", "AOFLGBP.DEF", "AOFLGBT.DEF", "AOFLGBS.DEF"
	};
	static const char * bgs[] =
	{
		"ADOPRPNL.bmp", "ADOPBPNL.bmp", "ADOPYPNL.bmp", "ADOPGPNL.bmp",
		"ADOPOPNL.bmp", "ADOPPPNL.bmp", "ADOPTPNL.bmp", "ADOPSPNL.bmp"
	};

	bg = new CPicture(BitmapHandler::loadBitmap(bgs[s.color.getNum()]), 0, 0, true);
	if(SEL->screenType == ESelectionScreen::newGame)
	{
		btns[0] = new CButton(Point(107, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[132], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::TOWN, -1, s.color));
		btns[1] = new CButton(Point(168, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[133], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::TOWN, +1, s.color));
		btns[2] = new CButton(Point(183, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[148], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::HERO, -1, s.color));
		btns[3] = new CButton(Point(244, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[149], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::HERO, +1, s.color));
		btns[4] = new CButton(Point(259, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[164], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::BONUS, -1, s.color));
		btns[5] = new CButton(Point(320, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[165], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::BONUS, +1, s.color));
	}
	else
		for(auto & elem : btns)
			elem = nullptr;

	hideUnavailableButtons();

	assert(CSH->mi && CSH->mi->mapHeader);
	const PlayerInfo & p = SEL->getPlayerInfo(s.color.getNum());
	assert(p.canComputerPlay || p.canHumanPlay); //someone must be able to control this player
	if(p.canHumanPlay && p.canComputerPlay)
		whoCanPlay = HUMAN_OR_CPU;
	else if(p.canComputerPlay)
		whoCanPlay = CPU;
	else
		whoCanPlay = HUMAN;

	if(SEL->screenType != ESelectionScreen::scenarioInfo && SEL->getPlayerInfo(s.color.getNum()).canHumanPlay)
	{
		flag = new CButton(Point(-43, 2), flags[s.color.getNum()], CGI->generaltexth->zelp[180], std::bind(&IServerAPI::setPlayer, CSH, s.color));
		flag->hoverable = true;
		flag->block(CSH->isGuest());
	}
	else
		flag = nullptr;

	town = new SelectedBox(Point(119, 2), s, TOWN);
	hero = new SelectedBox(Point(195, 2), s, HERO);
	bonus = new SelectedBox(Point(271, 2), s, BONUS);
}

void OptionsTab::PlayerOptionsEntry::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printAtMiddleLoc(s.name, 55, 10, FONT_SMALL, Colors::WHITE, to);
	printAtMiddleWBLoc(CGI->generaltexth->arraytxt[206 + whoCanPlay], 28, 39, FONT_TINY, 50, Colors::WHITE, to);
}

void OptionsTab::PlayerOptionsEntry::hideUnavailableButtons()
{
	if(!btns[0])
		return;

	const bool foreignPlayer = CSH->isGuest() && !CSH->isMyColor(s.color);

	if((pi.allowedFactions.size() < 2 && !pi.isFactionRandom) || foreignPlayer)
	{
		btns[0]->disable();
		btns[1]->disable();
	}
	else
	{
		btns[0]->enable();
		btns[1]->enable();
	}

	if((pi.defaultHero() != -1 || s.castle < 0) //fixed hero
		|| foreignPlayer) //or not our player
	{
		btns[2]->disable();
		btns[3]->disable();
	}
	else
	{
		btns[2]->enable();
		btns[3]->enable();
	}

	if(foreignPlayer)
	{
		btns[4]->disable();
		btns[5]->disable();
	}
	else
	{
		btns[4]->enable();
		btns[5]->enable();
	}
}
