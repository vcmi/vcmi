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
#include "../eventsSDL/InputHandler.h"

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

OptionsTab::SelectionWindow::SelectionWindow(PlayerColor _color, SelType _type)
	: CWindowObject(BORDERED)
{
	addUsedEvents(LCLICK | SHOW_POPUP);

	color = _color;
	type = _type;

	initialFraction = SEL->getStartInfo()->playerInfos.find(color)->second.castle;
	initialHero = SEL->getStartInfo()->playerInfos.find(color)->second.hero;
	initialBonus = SEL->getStartInfo()->playerInfos.find(color)->second.bonus;
	selectedFraction = initialFraction;
	selectedHero = initialHero;
	selectedBonus = initialBonus;
	allowedFactions = SEL->getPlayerInfo(color.getNum()).allowedFactions;
	std::vector<bool> allowedHeroesFlag = SEL->getMapInfo()->mapHeader->allowedHeroes;
	for(int i = 0; i < allowedHeroesFlag.size(); i++)
		if(allowedHeroesFlag[i])
			allowedHeroes.insert(HeroTypeID(i));

	allowedBonus.push_back(-1); // random
	if(initialHero >= -1)
		allowedBonus.push_back(0); // artifact
	allowedBonus.push_back(1); // gold
	if(initialFraction >= 0)
		allowedBonus.push_back(2); // resource

	recreate();
}

int OptionsTab::SelectionWindow::calcLines(FactionID faction)
{
	if(faction < 0)
		return std::ceil((double)allowedFactions.size() / ELEMENTS_PER_LINE);

	int count = 0;
	for(auto & elemh : allowedHeroes)
	{
		CHero * type = VLC->heroh->objects[elemh];
		if(type->heroClass->faction == faction)
			count++;
	}

	return std::ceil(std::max((double)count, (double)allowedFactions.size()) / (double)ELEMENTS_PER_LINE);
}

void OptionsTab::SelectionWindow::apply()
{
	if(GH.windows().isTopWindow(this))
	{
		GH.input().hapticFeedback();

		close();

		setSelection();
	}
}

void OptionsTab::SelectionWindow::setSelection()
{
	// fraction
	int selectedFractionPos = -1;
	for(int i = 0; i<factions.size(); i++)
		if(factions[i] == selectedFraction)
			selectedFractionPos = i;

	int initialFractionPos = -1;
	for(int i = 0; i<factions.size(); i++)
		if(factions[i] == initialFraction)
			initialFractionPos = i;

	int deltaFraction = selectedFractionPos - initialFractionPos;

	if(deltaFraction != 0)
		for(int i = 0; i<abs(deltaFraction); i++)
			CSH->setPlayerOption(LobbyChangePlayerOption::TOWN, deltaFraction > 0 ? 1 : -1, color);
	else
	{
		if(type == SelType::TOWN)
		{
			CSH->setPlayerOption(LobbyChangePlayerOption::TOWN, 1, color);
			CSH->setPlayerOption(LobbyChangePlayerOption::TOWN, -1, color);
		}
	}

	// hero
	int selectedHeroPos = -1;
	for(int i = 0; i<heroes.size(); i++)
		if(heroes[i] == selectedHero)
			selectedHeroPos = i;

	int initialHeroPos = -1;
	if(deltaFraction == 0)
		for(int i = 0; i<heroes.size(); i++)
			if(heroes[i] == initialHero)
				initialHeroPos = i;

	int deltaHero = selectedHeroPos - initialHeroPos;

	if(deltaHero != 0)
		for(int i = 0; i<abs(deltaHero); i++)
			CSH->setPlayerOption(LobbyChangePlayerOption::HERO, deltaHero > 0 ? 1 : -1, color);

	// bonus
	int deltaBonus = selectedBonus - initialBonus;

	if(initialHero < -1 && ((selectedBonus > 0 && initialBonus < 0) || (selectedBonus < 0 && initialBonus > 0))) // no artifact
	{
		if(deltaBonus > 0)
			deltaBonus--;
		else if(deltaBonus < 0)
			deltaBonus++;
	}

	if(deltaBonus != 0)
		for(int i = 0; i<abs(deltaBonus); i++)
			CSH->setPlayerOption(LobbyChangePlayerOption::BONUS, deltaBonus > 0 ? 1 : -1, color);
}

void OptionsTab::SelectionWindow::recreate()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	
	int amountLines = calcLines((type > SelType::TOWN) ? selectedFraction : static_cast<FactionID>(-1));
	if(type == SelType::BONUS)
		amountLines = 1;

	int x = (ELEMENTS_PER_LINE) * 57;
	int y = (amountLines) * 63;

	pos = Rect(0, 0, x, y);

	backgroundTexture = std::make_shared<CFilledTexture>("DlgBluBk", pos);
	updateShadow();

	components.clear();

	GH.windows().totalRedraw();

	if(type == SelType::TOWN)
		genContentCastles();
	if(type == SelType::HERO)
		genContentHeroes();
	if(type == SelType::BONUS)
		genContentBonus();
	genContentGrid(amountLines);

	center();
}

void OptionsTab::SelectionWindow::genContentGrid(int lines)
{
	for(int y = 0; y<lines; y++)
	{
		for(int x = 0; x<ELEMENTS_PER_LINE; x++)
		{
			components.push_back(std::make_shared<CPicture>("lobby/townBorderBig", x * 57, y * 63));
		}
	}
}

void OptionsTab::SelectionWindow::genContentCastles()
{
	int i = 0;

	for(auto & elem : allowedFactions)
	{
		int x = i % ELEMENTS_PER_LINE;
		int y = i / ELEMENTS_PER_LINE;

		PlayerSettings set = PlayerSettings();
		set.castle = elem;

		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);

		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * 57, y * 63));
		components.push_back(std::make_shared<CPicture>(selectedFraction == elem ? "lobby/townBorderBigActivated" : "lobby/townBorderBig", x * 57, y * 63));
		factions.push_back(elem);

		i++;
	}
}

void OptionsTab::SelectionWindow::genContentHeroes()
{
	int i = 0;

	for(auto & elem : allowedHeroes)
	{
		CHero * type = VLC->heroh->objects[elem];

		if(type->heroClass->faction == selectedFraction)
		{

			int x = i % ELEMENTS_PER_LINE;
			int y = i / ELEMENTS_PER_LINE;

			PlayerSettings set = PlayerSettings();
			set.hero = elem;

			CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);

			components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * 57, y * 63));
			components.push_back(std::make_shared<CPicture>(selectedHero == elem ? "lobby/townBorderBigActivated" : "lobby/townBorderBig", x * 57, y * 63));
			heroes.push_back(elem);

			i++;
		}
	}
}

void OptionsTab::SelectionWindow::genContentBonus()
{
	PlayerSettings set = PlayerSettings();

	int i = 0;
	for(auto elem : allowedBonus)
	{
		int x = i;
		int y = 0;

		set.bonus = static_cast<PlayerSettings::Ebonus>(elem);
		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, x * 57 + 6, y * 63 + 32 / 2));
		if(selectedBonus == elem)
			if(elem == set.RESOURCE && selectedFraction >= 0)
				components.push_back(std::make_shared<CPicture>("lobby/townBorderSmallActivated", x * 57 + 6, y * 63 + 32 / 2));

		i++;
	}
}

int OptionsTab::SelectionWindow::getElement(const Point & cursorPosition)
{
	int x = (cursorPosition.x - pos.x) / 57;
	int y = (cursorPosition.y - pos.y) / 63;

	return x + y * ELEMENTS_PER_LINE;
}

void OptionsTab::SelectionWindow::clickReleased(const Point & cursorPosition) {
	int elem = getElement(cursorPosition);

	PlayerSettings set = PlayerSettings();
	if(type == SelType::TOWN)
	{
		if(elem >= factions.size())
			return;
		set.castle = factions[elem];
		if(set.castle != -2)
			selectedFraction = set.castle;
	}
	if(type == SelType::HERO)
	{
		if(elem >= heroes.size())
			return;
		set.hero = heroes[elem];
		if(set.hero != -2)
			selectedHero = set.hero;
	}
	if(type == SelType::BONUS)
	{
		if(elem >= allowedBonus.size())
			return;
		set.bonus = static_cast<PlayerSettings::Ebonus>(allowedBonus[elem]);
		if(set.bonus != -2)
			selectedBonus = set.bonus;
	}
	apply();
}

void OptionsTab::SelectionWindow::showPopupWindow(const Point & cursorPosition)
{
	int elem = getElement(cursorPosition);

	PlayerSettings set = PlayerSettings();
	if(type == SelType::TOWN)
	{
		if(elem >= factions.size())
			return;
		set.castle = factions[elem];
		if(set.castle != -2)
		{
			CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
			GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
		}
	}
	if(type == SelType::HERO)
	{
		if(elem >= heroes.size())
			return;
		set.hero = heroes[elem];
		if(set.hero != -2)
		{
			CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
			GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
		}
	}
	if(type == SelType::BONUS)
	{
		if(elem >= 4)
			return;
		set.bonus = static_cast<PlayerSettings::Ebonus>(elem-1);
		if(set.bonus != -2)
		{
			CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
			GH.windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
		}
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
	PlayerInfo pi = SEL->getPlayerInfo(settings.color.getNum());
	const bool foreignPlayer = CSH->isGuest() && !CSH->isMyColor(settings.color);

	if(type == SelType::TOWN && ((pi.allowedFactions.size() < 2 && !pi.isFactionRandom) || foreignPlayer))
		return;

	if(type == SelType::HERO && ((pi.defaultHero() != -1 || settings.castle < 0) || foreignPlayer))
		return;

	if(type == SelType::BONUS && foreignPlayer)
		return;

	GH.input().hapticFeedback();
	GH.windows().createAndPushWindow<SelectionWindow>(settings.color, type);
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
