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

#include "CSelectionBase.h"

#include "../CGameInfo.h"
#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/NetPacksLobby.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"

OptionsTab::OptionsTab() : humanPlayers(0)
{
	recActions = 0;
	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>("ADVOPTBK", 0, 6);
	pos = background->pos;
	labelTitle = std::make_shared<CLabel>(222, 30, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[515]);
	labelSubTitle = std::make_shared<CMultiLineLabel>(Rect(60, 44, 320, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[516]);

	labelPlayerNameAndHandicap = std::make_shared<CMultiLineLabel>(Rect(58, 86, 100, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[517]);
	labelStartingTown = std::make_shared<CMultiLineLabel>(Rect(163, 86, 70, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[518]);
	labelStartingHero = std::make_shared<CMultiLineLabel>(Rect(239, 86, 70, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[519]);
	labelStartingBonus = std::make_shared<CMultiLineLabel>(Rect(315, 86, 70, (int)graphics->fonts[EFonts::FONT_SMALL]->getLineHeight()*2), EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[520]);
	if(SEL->screenType == ESelectionScreen::newGame || SEL->screenType == ESelectionScreen::loadGame || SEL->screenType == ESelectionScreen::scenarioInfo)
	{
		sliderTurnDuration = std::make_shared<CSlider>(Point(55, 551), 194, std::bind(&IServerAPI::setTurnLength, CSH, _1), 1, (int)GameConstants::POSSIBLE_TURNTIME.size(), (int)GameConstants::POSSIBLE_TURNTIME.size(), Orientation::HORIZONTAL, CSlider::BLUE);
		sliderTurnDuration->setScrollBounds(Rect(-3, -25, 337, 43));
		sliderTurnDuration->setPanningStep(20);
		labelPlayerTurnDuration = std::make_shared<CLabel>(222, 538, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[521]);
		labelTurnDurationValue = std::make_shared<CLabel>(319, 559, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	}
}

void OptionsTab::recreate()
{
	entries.clear();
	humanPlayers = 0;

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	for(auto & pInfo : SEL->getStartInfo()->playerInfos)
	{
		if(pInfo.second.isControlledByHuman())
			humanPlayers++;

		entries.insert(std::make_pair(pInfo.first, std::make_shared<PlayerOptionsEntry>(pInfo.second, * this)));
	}

	if(sliderTurnDuration)
	{
		sliderTurnDuration->scrollTo(vstd::find_pos(GameConstants::POSSIBLE_TURNTIME, SEL->getStartInfo()->turnTime));
		labelTurnDurationValue->setText(CGI->generaltexth->turnDurations[sliderTurnDuration->getValue()]);
	}
}

size_t OptionsTab::CPlayerSettingsHelper::getImageIndex(bool big)
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
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;

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
			return (*CGI->townh)[factionIndex]->town->clientInfo.icons[true][false] + (big ? 0 : 2);
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
			auto index = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;
			return (*CGI->heroh)[index]->imageIndex;
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
			switch((*CGI->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::WOOD_AND_ORE:
				return WOOD_ORE;
			case EGameResID::WOOD:
				return WOOD;
			case EGameResID::MERCURY:
				return MERCURY;
			case EGameResID::ORE:
				return ORE;
			case EGameResID::SULFUR:
				return SULFUR;
			case EGameResID::CRYSTAL:
				return CRYSTAL;
			case EGameResID::GEMS:
				return GEM;
			case EGameResID::GOLD:
				return GOLD;
			case EGameResID::MITHRIL:
				return MITHRIL;
			}
		}
		}
	}
	}
	return 0;
}

std::string OptionsTab::CPlayerSettingsHelper::getImageName(bool big)
{
	switch(type)
	{
	case OptionsTab::TOWN:
		return big ? "ITPt": "ITPA";
	case OptionsTab::HERO:
		return big ? "PortraitsLarge": "PortraitsSmall";
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
		{
			auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;
			return (*CGI->townh)[factionIndex]->getNameTranslated();
		}
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
			auto index = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;
			return (*CGI->heroh)[index]->getNameTranslated();
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
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;
	auto heroIndex = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;

	switch(type)
	{
	case TOWN:
		return getName();
	case HERO:
	{
		if(settings.hero >= 0)
			return getName() + " - " + (*CGI->heroh)[heroIndex]->heroClass->getNameTranslated();
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
			switch((*CGI->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::MERCURY:
				return CGI->generaltexth->allTexts[694];
			case EGameResID::SULFUR:
				return CGI->generaltexth->allTexts[695];
			case EGameResID::CRYSTAL:
				return CGI->generaltexth->allTexts[692];
			case EGameResID::GEMS:
				return CGI->generaltexth->allTexts[693];
			case EGameResID::WOOD_AND_ORE:
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
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;

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
			switch((*CGI->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::MERCURY:
				return CGI->generaltexth->allTexts[690];
			case EGameResID::SULFUR:
				return CGI->generaltexth->allTexts[691];
			case EGameResID::CRYSTAL:
				return CGI->generaltexth->allTexts[688];
			case EGameResID::GEMS:
				return CGI->generaltexth->allTexts[689];
			case EGameResID::WOOD_AND_ORE:
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
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

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
	backgroundTexture = std::make_shared<CFilledTexture>("DIBOXBCK", pos);
	updateShadow();

	labelTitle = std::make_shared<CLabel>(pos.w / 2 + 8, 21, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, getTitle());
	labelSubTitle = std::make_shared<CLabel>(pos.w / 2, 88, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, getSubtitle());
	image = std::make_shared<CAnimImage>(getImageName(), getImageIndex(), 0, pos.w / 2 - 24, 45);
}

void OptionsTab::CPlayerOptionTooltipBox::genTownWindow()
{
	pos = Rect(0, 0, 228, 290);
	genHeader();
	labelAssociatedCreatures = std::make_shared<CLabel>(pos.w / 2 + 8, 122, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[79]);
	auto factionIndex = settings.castle >= CGI->townh->size() ? 0 : settings.castle;
	std::vector<std::shared_ptr<CComponent>> components;
	const CTown * town = (*CGI->townh)[factionIndex]->town;

	for(auto & elem : town->creatures)
	{
		if(!elem.empty())
			components.push_back(std::make_shared<CComponent>(CComponent::creature, elem.front(), 0, CComponent::tiny));
	}
	boxAssociatedCreatures = std::make_shared<CComponentBox>(components, Rect(10, 140, pos.w - 20, 140));
}

void OptionsTab::CPlayerOptionTooltipBox::genHeroWindow()
{
	pos = Rect(0, 0, 292, 226);
	genHeader();
	labelHeroSpeciality = std::make_shared<CLabel>(pos.w / 2 + 4, 117, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	auto heroIndex = settings.hero >= CGI->heroh->size() ? 0 : settings.hero;

	imageSpeciality = std::make_shared<CAnimImage>("UN44", (*CGI->heroh)[heroIndex]->imageIndex, 0, pos.w / 2 - 22, 134);
	labelSpecialityName = std::make_shared<CLabel>(pos.w / 2, 188, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (*CGI->heroh)[heroIndex]->getSpecialtyNameTranslated());
}

void OptionsTab::CPlayerOptionTooltipBox::genBonusWindow()
{
	pos = Rect(0, 0, 228, 162);
	genHeader();

	textBonusDescription = std::make_shared<CTextBox>(getDescription(), Rect(10, 100, pos.w - 20, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
}

OptionsTab::SelectionWindow::SelectionWindow(int color)
	: CWindowObject(BORDERED)
{
	addUsedEvents(LCLICK | SHOW_POPUP);

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	color = color;

	pos = Rect(0, 0, (ELEMENTS_PER_LINE * 2 + 5) * 58, 700);

	backgroundTexture = std::make_shared<CFilledTexture>("DIBOXBCK", pos);
	updateShadow();

	genContentTitle();
	genContentCastles();
	genContentHeroes();
	genContentBonus();

	center();
}

void OptionsTab::SelectionWindow::apply()
{
	if(GH.windows().isTopWindow(this))
	{
		close();
	}
}

void OptionsTab::SelectionWindow::redraw()
{
	components.clear();

	genContentTitle();
	genContentCastles();
	genContentHeroes();
	genContentBonus();
}

void OptionsTab::SelectionWindow::genContentTitle()
{
	components.push_back(std::make_shared<CLabel>((ELEMENTS_PER_LINE - 1) * 58, 40, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, "Town"));
	components.push_back(std::make_shared<CLabel>((ELEMENTS_PER_LINE * 2) * 58, 40, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, "Hero"));
	components.push_back(std::make_shared<CLabel>((ELEMENTS_PER_LINE * 2 + 3) * 58 + 29, 40, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, "Bonus"));
}

void OptionsTab::SelectionWindow::genContentCastles()
{
	factions.clear();

	PlayerSettings set = PlayerSettings();
	set.castle = set.RANDOM;
	CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
	components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, (ELEMENTS_PER_LINE / 2) * 58 + 34, 32 / 2 + 64));



	int i = 0;
	for(auto & elem : SEL->getPlayerInfo(0).allowedFactions)
	{
		int x = i % ELEMENTS_PER_LINE + 1;
		int y = i / ELEMENTS_PER_LINE + 2;

		PlayerSettings set = PlayerSettings();
		set.castle = elem;

		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);

		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * 58, y * 64));
		factions.push_back(elem);

		i++;
	}
}

void OptionsTab::SelectionWindow::genContentHeroes()
{
	heroes.clear();

	PlayerSettings set = PlayerSettings();
	set.castle = set.RANDOM;
	CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
	components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, (ELEMENTS_PER_LINE / 2) * 58 + (ELEMENTS_PER_LINE + 1) * 58 + 34, 32 / 2 + 64));

	std::vector<bool> allowedHeroesFlag = SEL->getMapInfo()->mapHeader->allowedHeroes;

	std::set<HeroTypeID> allowedHeroes;
	for(int i = 0; i < allowedHeroesFlag.size(); i++)
		if(allowedHeroesFlag[i])
			allowedHeroes.insert(HeroTypeID(i));

	int i = 0;
	for(auto & elem : allowedHeroes)
	{
		CHero * type = VLC->heroh->objects[elem];

		if(type->heroClass->faction == SEL->getStartInfo()->playerInfos.find(PlayerColor(color))->second.castle)
		{

			int x = (i % ELEMENTS_PER_LINE) + ELEMENTS_PER_LINE + 2;
			int y = i / ELEMENTS_PER_LINE + 2;

			PlayerSettings set = PlayerSettings();
			set.hero = elem;

			CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);

			components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * 58, y * 64));
			heroes.push_back(elem);

			i++;
		}
	}
}

void OptionsTab::SelectionWindow::genContentBonus()
{
	PlayerSettings set = PlayerSettings();

	int i = 0;
	for(auto elem : {set.RANDOM, set.ARTIFACT, set.GOLD, set.RESOURCE})
	{
		int x = ELEMENTS_PER_LINE * 2 + 3;
		int y = i + 1;

		set.bonus = elem;
		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, x * 58, y * 64 + 32 / 2));

		i++;
	}
}

FactionID OptionsTab::SelectionWindow::getElementCastle(const Point & cursorPosition)
{
	Point loc = getElement(cursorPosition, 0);

	FactionID faction;
	faction = PlayerSettings().NONE;
	if ((loc.x == ELEMENTS_PER_LINE / 2 || loc.x == ELEMENTS_PER_LINE / 2 - 1) && loc.y == 0)
		faction = PlayerSettings().RANDOM;
	else if(loc.y > 0 && loc.x < ELEMENTS_PER_LINE)
	{
		int index = loc.x + (loc.y - 1) * ELEMENTS_PER_LINE;
		if (index < factions.size())
			faction = factions[loc.x + (loc.y - 1) * ELEMENTS_PER_LINE];
	}

	return faction;
}

HeroTypeID OptionsTab::SelectionWindow::getElementHero(const Point & cursorPosition)
{
	Point loc = getElement(cursorPosition, 1);

	HeroTypeID hero;
	hero = PlayerSettings().NONE;

	if(loc.x < 0)
		return hero;

	if ((loc.x == ELEMENTS_PER_LINE / 2 || loc.x == ELEMENTS_PER_LINE / 2 - 1) && loc.y == 0)
		hero = PlayerSettings().RANDOM;
	else if(loc.y > 0 && loc.x < ELEMENTS_PER_LINE)
	{
		int index = loc.x + (loc.y - 1) * ELEMENTS_PER_LINE;
		if (index < heroes.size())
			hero = heroes[loc.x + (loc.y - 1) * ELEMENTS_PER_LINE];
	}

	return hero;
}

int OptionsTab::SelectionWindow::getElementBonus(const Point & cursorPosition)
{
	Point loc = getElement(cursorPosition, 2);

	if(loc.x != 0 || loc.y < 0 || loc.y > 3)
		return -2;

	return loc.y - 1;
}

Point OptionsTab::SelectionWindow::getElement(const Point & cursorPosition, int area)
{
	int x = (cursorPosition.x - pos.x - area * (ELEMENTS_PER_LINE + 1) * 58) / 58;
	int y = (cursorPosition.y - pos.y) / 64;

	return Point(x - 1, y - 1);
}

void OptionsTab::SelectionWindow::clickReleased(const Point & cursorPosition) {
	FactionID faction = getElementCastle(cursorPosition);
	HeroTypeID hero = getElementHero(cursorPosition);
	int bonus = getElementBonus(cursorPosition);

	PlayerSettings set = PlayerSettings();
	set.castle = faction;
	set.hero = hero;
	set.bonus = static_cast<PlayerSettings::Ebonus>(bonus);

	if(set.castle != -2)
	{
		//SEL->getStartInfo()->playerInfos.find(PlayerColor(color))->second.castle = faction;

		CSH->setPlayerOption(LobbyChangePlayerOption::TOWN, 1, PlayerColor(color));

		redraw();
		//CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
		
	}
	else if(set.hero != -2)
	{
		//CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
		apply();
	}
	else if(set.bonus != -2)
	{
		//CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
		apply();
	}
}

void OptionsTab::SelectionWindow::showPopupWindow(const Point & cursorPosition)
{
	FactionID faction = getElementCastle(cursorPosition);
	HeroTypeID hero = getElementHero(cursorPosition);
	int bonus = getElementBonus(cursorPosition);

	PlayerSettings set = PlayerSettings();
	set.castle = faction;
	set.hero = hero;
	set.bonus = static_cast<PlayerSettings::Ebonus>(bonus);

	if(set.castle != -2)
	{
		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
		GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
	}
	else if(set.hero != -2)
	{
		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
		GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
	}
	else if(set.bonus != -2)
	{
		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
		GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
	}
}

OptionsTab::SelectedBox::SelectedBox(Point position, PlayerSettings & settings, SelType type)
	: Scrollable(LCLICK | SHOW_POPUP, position, Orientation::HORIZONTAL)
	, CPlayerSettingsHelper(settings, type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	image = std::make_shared<CAnimImage>(getImageName(), getImageIndex());
	subtitle = std::make_shared<CLabel>(23, 39, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, getName());

	pos = image->pos;

	setPanningStep(pos.w);
}

void OptionsTab::SelectedBox::update()
{
	image->setFrame(getImageIndex());
	subtitle->setText(getName());
}

void OptionsTab::SelectedBox::showPopupWindow(const Point & cursorPosition)
{
	// cases when we do not need to display a message
	if(settings.castle == -2 && CPlayerSettingsHelper::type == TOWN)
		return;
	if(settings.hero == -2 && !SEL->getPlayerInfo(settings.color.getNum()).hasCustomMainHero() && CPlayerSettingsHelper::type == HERO)
		return;

	GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(*this);
}

void OptionsTab::SelectedBox::clickReleased(const Point & cursorPosition)
{
	GH.windows().createAndPushWindow<SelectionWindow>(settings.color.getNum());
}

void OptionsTab::SelectedBox::scrollBy(int distance)
{
	// FIXME: currently options tab is completely recreacted from scratch whenever we receive any information from server
	// because of that, panning event gets interrupted (due to destruction of element)
	// so, currently, gesture will always move selection only by 1, and then wait for recreation from server info
	distance = std::clamp(distance, -1, 1);

	switch(CPlayerSettingsHelper::type)
	{
		case TOWN:
			CSH->setPlayerOption(LobbyChangePlayerOption::TOWN, distance, settings.color);
			break;
		case HERO:
			CSH->setPlayerOption(LobbyChangePlayerOption::HERO, distance, settings.color);
			break;
		case BONUS:
			CSH->setPlayerOption(LobbyChangePlayerOption::BONUS, distance, settings.color);
			break;
	}

	setScrollingEnabled(false);
}

OptionsTab::PlayerOptionsEntry::PlayerOptionsEntry(const PlayerSettings & S, const OptionsTab & parent)
	: pi(std::make_unique<PlayerInfo>(SEL->getPlayerInfo(S.color.getNum())))
	, s(std::make_unique<PlayerSettings>(S))
	, parentTab(parent)
{
	OBJ_CONSTRUCTION;
	defActions |= SHARE_POS;

	int serial = 0;
	for(int g = 0; g < s->color.getNum(); ++g)
	{
		auto itred = SEL->getPlayerInfo(g);
		if(itred.canComputerPlay || itred.canHumanPlay)
			serial++;
	}

	pos.x += 54;
	pos.y += 122 + serial * 50;

	assert(CSH->mi && CSH->mi->mapHeader);
	const PlayerInfo & p = SEL->getPlayerInfo(s->color.getNum());
	assert(p.canComputerPlay || p.canHumanPlay); //someone must be able to control this player
	if(p.canHumanPlay && p.canComputerPlay)
		whoCanPlay = HUMAN_OR_CPU;
	else if(p.canComputerPlay)
		whoCanPlay = CPU;
	else
		whoCanPlay = HUMAN;

	static const std::array<std::string, PlayerColor::PLAYER_LIMIT_I> flags =
	{{
		"AOFLGBR.DEF", "AOFLGBB.DEF", "AOFLGBY.DEF", "AOFLGBG.DEF",
		"AOFLGBO.DEF", "AOFLGBP.DEF", "AOFLGBT.DEF", "AOFLGBS.DEF"
	}};
	static const std::array<std::string, PlayerColor::PLAYER_LIMIT_I> bgs =
	{{
		"ADOPRPNL.bmp", "ADOPBPNL.bmp", "ADOPYPNL.bmp", "ADOPGPNL.bmp",
		"ADOPOPNL.bmp", "ADOPPPNL.bmp", "ADOPTPNL.bmp", "ADOPSPNL.bmp"
	}};

	background = std::make_shared<CPicture>(bgs[s->color.getNum()], 0, 0);
	labelPlayerName = std::make_shared<CLabel>(55, 10, EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, s->name);
	labelWhoCanPlay = std::make_shared<CMultiLineLabel>(Rect(6, 23, 45, (int)graphics->fonts[EFonts::FONT_TINY]->getLineHeight()*2), EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->arraytxt[206 + whoCanPlay]);

	if(SEL->screenType == ESelectionScreen::newGame)
	{
		buttonTownLeft = std::make_shared<CButton>(Point(107, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[132], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::TOWN, -1, s->color));
		buttonTownRight = std::make_shared<CButton>(Point(168, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[133], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::TOWN, +1, s->color));
		buttonHeroLeft = std::make_shared<CButton>(Point(183, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[148], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::HERO, -1, s->color));
		buttonHeroRight = std::make_shared<CButton>(Point(244, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[149], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::HERO, +1, s->color));
		buttonBonusLeft = std::make_shared<CButton>(Point(259, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[164], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::BONUS, -1, s->color));
		buttonBonusRight = std::make_shared<CButton>(Point(320, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[165], std::bind(&IServerAPI::setPlayerOption, CSH, LobbyChangePlayerOption::BONUS, +1, s->color));
	}

	hideUnavailableButtons();

	if(SEL->screenType != ESelectionScreen::scenarioInfo && SEL->getPlayerInfo(s->color.getNum()).canHumanPlay)
	{
		flag = std::make_shared<CButton>(
			Point(-43, 2),
			flags[s->color.getNum()],
			CGI->generaltexth->zelp[180],
			std::bind(&OptionsTab::onSetPlayerClicked, &parentTab, *s)
		);
		flag->hoverable = true;
		flag->block(CSH->isGuest());
	}
	else
		flag = nullptr;

	town = std::make_shared<SelectedBox>(Point(119, 2), *s, TOWN);
	hero = std::make_shared<SelectedBox>(Point(195, 2), *s, HERO);
	bonus = std::make_shared<SelectedBox>(Point(271, 2), *s, BONUS);
}

void OptionsTab::onSetPlayerClicked(const PlayerSettings & ps) const
{
	if(ps.isControlledByAI() || humanPlayers > 0)
		CSH->setPlayer(ps.color);
}

void OptionsTab::PlayerOptionsEntry::hideUnavailableButtons()
{
	if(!buttonTownLeft)
		return;

	const bool foreignPlayer = CSH->isGuest() && !CSH->isMyColor(s->color);

	if((pi->allowedFactions.size() < 2 && !pi->isFactionRandom) || foreignPlayer)
	{
		buttonTownLeft->disable();
		buttonTownRight->disable();
	}
	else
	{
		buttonTownLeft->enable();
		buttonTownRight->enable();
	}

	if((pi->defaultHero() != -1 || s->castle < 0) //fixed hero
		|| foreignPlayer) //or not our player
	{
		buttonHeroLeft->disable();
		buttonHeroRight->disable();
	}
	else
	{
		buttonHeroLeft->enable();
		buttonHeroRight->enable();
	}

	if(foreignPlayer)
	{
		buttonBonusLeft->disable();
		buttonBonusRight->disable();
	}
	else
	{
		buttonBonusLeft->enable();
		buttonBonusRight->enable();
	}
}
