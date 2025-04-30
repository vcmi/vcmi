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

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"
#include "../media/ISoundPlayer.h"
#include "../widgets/CComponent.h"
#include "../widgets/ComboBox.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../windows/CHeroOverview.h"
#include "../eventsSDL/InputHandler.h"

#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/GameLibrary.h"

static JsonPath optionsTabConfigLocation()
{
	if(settings["general"]["enableUiEnhancements"].Bool())
		return JsonPath::builtin("config/widgets/playerOptionsTab.json");
	else
		return JsonPath::builtin("config/widgets/advancedOptionsTab.json");
}

OptionsTab::OptionsTab()
	: OptionsTabBase(optionsTabConfigLocation())
	, humanPlayers(0)
{
}

void OptionsTab::recreate()
{
	entries.clear();
	humanPlayers = 0;

	for (auto tooltipWindow : ENGINE->windows().findWindows<CPlayerOptionTooltipBox>())
		tooltipWindow->close();

	for (auto heroOverview : ENGINE->windows().findWindows<CHeroOverview>())
		heroOverview->close();

	for (auto selectionWindow : ENGINE->windows().findWindows<SelectionWindow>())
		selectionWindow->reopen();

	OBJECT_CONSTRUCTION;
	for(auto & pInfo : SEL->getStartInfo()->playerInfos)
	{
		if(pInfo.second.isControlledByHuman())
			humanPlayers++;

		entries.insert(std::make_pair(pInfo.first, std::make_shared<PlayerOptionsEntry>(pInfo.second, * this)));
	}

	OptionsTabBase::recreate();
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
		HERO_RANDOM = 156, HERO_NONE = 157 // Special frames in PortraitsSmall
	};
	auto factionIndex = playerSettings.getCastleValidated();

	switch(selectionType)
	{
	case TOWN:
	{
		if (playerSettings.castle == FactionID::NONE)
			return TOWN_NONE;

		if (playerSettings.castle == FactionID::RANDOM)
			return TOWN_RANDOM;

		return (*LIBRARY->townh)[factionIndex]->town->clientInfo.icons[true][false] + (big ? 0 : 2);
	}

	case HERO:
	{
		if (playerSettings.hero == HeroTypeID::NONE)
			return HERO_NONE;

		if (playerSettings.hero == HeroTypeID::RANDOM)
			return HERO_RANDOM;

		if(playerSettings.heroPortrait != HeroTypeID::NONE)
			return playerSettings.heroPortrait;

		auto index = playerSettings.getHeroValidated();
		return (*LIBRARY->heroh)[index]->imageIndex;
	}

	case BONUS:
	{
		switch(playerSettings.bonus)
		{
		case PlayerStartingBonus::RANDOM:
			return RANDOM;
		case PlayerStartingBonus::ARTIFACT:
			return ARTIFACT;
		case PlayerStartingBonus::GOLD:
			return GOLD;
		case PlayerStartingBonus::RESOURCE:
		{
			switch((*LIBRARY->townh)[factionIndex]->town->primaryRes.toEnum())
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

AnimationPath OptionsTab::CPlayerSettingsHelper::getImageName(bool big)
{
	switch(selectionType)
	{
	case OptionsTab::TOWN:
		return AnimationPath::builtin(big ? "ITPt": "ITPA");
	case OptionsTab::HERO:
		return AnimationPath::builtin(big ? "PortraitsLarge": "PortraitsSmall");
	case OptionsTab::BONUS:
		return AnimationPath::builtin("SCNRSTAR");
	}
	return {};
}

std::string OptionsTab::CPlayerSettingsHelper::getName()
{
	switch(selectionType)
	{
		case TOWN:
		{
			if (playerSettings.castle == FactionID::NONE)
				return LIBRARY->generaltexth->allTexts[523];

			if (playerSettings.castle == FactionID::RANDOM)
				return LIBRARY->generaltexth->allTexts[522];

			auto factionIndex = playerSettings.getCastleValidated();
			return (*LIBRARY->townh)[factionIndex]->getNameTranslated();
		}
		case HERO:
		{
			if (playerSettings.hero == HeroTypeID::NONE)
				return LIBRARY->generaltexth->allTexts[523];

			if (playerSettings.hero == HeroTypeID::RANDOM)
					return LIBRARY->generaltexth->allTexts[522];

			if(!playerSettings.heroNameTextId.empty())
				return LIBRARY->generaltexth->translate(playerSettings.heroNameTextId);
			auto index = playerSettings.getHeroValidated();
			return (*LIBRARY->heroh)[index]->getNameTranslated();
		}
		case BONUS:
		{
			if (playerSettings.bonus == PlayerStartingBonus::RANDOM)
					return LIBRARY->generaltexth->allTexts[522];

			return LIBRARY->generaltexth->arraytxt[214 + static_cast<int>(playerSettings.bonus)];
		}
	}
	return "";
}


std::string OptionsTab::CPlayerSettingsHelper::getTitle()
{
	switch(selectionType)
	{
	case OptionsTab::TOWN:
		return playerSettings.castle.isValid() ? LIBRARY->generaltexth->allTexts[80] : LIBRARY->generaltexth->allTexts[103];
	case OptionsTab::HERO:
		return playerSettings.hero.isValid() ? LIBRARY->generaltexth->allTexts[77] : LIBRARY->generaltexth->allTexts[101];
	case OptionsTab::BONUS:
	{
		switch(playerSettings.bonus)
		{
		case PlayerStartingBonus::RANDOM:
			return LIBRARY->generaltexth->allTexts[86]; //{Random Bonus}
		case PlayerStartingBonus::ARTIFACT:
			return LIBRARY->generaltexth->allTexts[83]; //{Artifact Bonus}
		case PlayerStartingBonus::GOLD:
			return LIBRARY->generaltexth->allTexts[84]; //{Gold Bonus}
		case PlayerStartingBonus::RESOURCE:
			return LIBRARY->generaltexth->allTexts[85]; //{Resource Bonus}
		}
	}
	}
	return "";
}
std::string OptionsTab::CPlayerSettingsHelper::getSubtitle()
{
	auto factionIndex = playerSettings.getCastleValidated();
	auto heroIndex = playerSettings.getHeroValidated();

	switch(selectionType)
	{
	case TOWN:
		return getName();
	case HERO:
	{
		if(playerSettings.hero.isValid())
			return getName() + " - " + (*LIBRARY->heroh)[heroIndex]->heroClass->getNameTranslated();
		return getName();
	}

	case BONUS:
	{
		switch(playerSettings.bonus)
		{
		case PlayerStartingBonus::GOLD:
			return LIBRARY->generaltexth->allTexts[87]; //500-1000
		case PlayerStartingBonus::RESOURCE:
		{
			switch((*LIBRARY->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::MERCURY:
				return LIBRARY->generaltexth->allTexts[694];
			case EGameResID::SULFUR:
				return LIBRARY->generaltexth->allTexts[695];
			case EGameResID::CRYSTAL:
				return LIBRARY->generaltexth->allTexts[692];
			case EGameResID::GEMS:
				return LIBRARY->generaltexth->allTexts[693];
			case EGameResID::WOOD_AND_ORE:
				return LIBRARY->generaltexth->allTexts[89]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
			}
		}
		}
	}
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getDescription()
{
	auto factionIndex = playerSettings.getCastleValidated();

	switch(selectionType)
	{
	case TOWN:
		return LIBRARY->generaltexth->allTexts[104];
	case HERO:
		return LIBRARY->generaltexth->allTexts[102];
	case BONUS:
	{
		switch(playerSettings.bonus)
		{
		case PlayerStartingBonus::RANDOM:
			return LIBRARY->generaltexth->allTexts[94]; //Gold, wood and ore, or an artifact is randomly chosen as your starting bonus
		case PlayerStartingBonus::ARTIFACT:
			return LIBRARY->generaltexth->allTexts[90]; //An artifact is randomly chosen and equipped to your starting hero
		case PlayerStartingBonus::GOLD:
			return LIBRARY->generaltexth->allTexts[92]; //At the start of the game, 500-1000 gold is added to your Kingdom's resource pool
		case PlayerStartingBonus::RESOURCE:
		{
			switch((*LIBRARY->townh)[factionIndex]->town->primaryRes.toEnum())
			{
			case EGameResID::MERCURY:
				return LIBRARY->generaltexth->allTexts[690];
			case EGameResID::SULFUR:
				return LIBRARY->generaltexth->allTexts[691];
			case EGameResID::CRYSTAL:
				return LIBRARY->generaltexth->allTexts[688];
			case EGameResID::GEMS:
				return LIBRARY->generaltexth->allTexts[689];
			case EGameResID::WOOD_AND_ORE:
				return LIBRARY->generaltexth->allTexts[93]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
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
	OBJECT_CONSTRUCTION;

	switch(selectionType)
	{
		case TOWN:
			genTownWindow();
			break;
		case HERO:
			genHeroWindow();
			break;
		case BONUS:
			genBonusWindow();
			break;
	}

	center();
}

void OptionsTab::CPlayerOptionTooltipBox::genHeader()
{
	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	updateShadow();

	labelTitle = std::make_shared<CLabel>(pos.w / 2 + 8, 21, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, getTitle());
	labelSubTitle = std::make_shared<CLabel>(pos.w / 2, 88, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, getSubtitle());
	image = std::make_shared<CAnimImage>(getImageName(), getImageIndex(), 0, pos.w / 2 - 24, 45);
}

void OptionsTab::CPlayerOptionTooltipBox::genTownWindow()
{
	auto factionIndex = playerSettings.getCastleValidated();

	if (playerSettings.castle == FactionID::RANDOM)
		return genBonusWindow();

	pos = Rect(0, 0, 228, 290);
	genHeader();
	labelAssociatedCreatures = std::make_shared<CLabel>(pos.w / 2 + 8, 122, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[79]);
	std::vector<std::shared_ptr<CComponent>> components;
	const CTown * town = (*LIBRARY->townh)[factionIndex]->town.get();

	for(auto & elem : town->creatures)
	{
		if(!elem.empty())
			components.push_back(std::make_shared<CComponent>(ComponentType::CREATURE, elem.front(), std::nullopt, CComponent::tiny));
	}
	boxAssociatedCreatures = std::make_shared<CComponentBox>(components, Rect(10, 140, pos.w - 20, 140), 20, 10, 22, 4);
}

void OptionsTab::CPlayerOptionTooltipBox::genHeroWindow()
{
	auto heroIndex = playerSettings.getHeroValidated();

	if (playerSettings.hero == HeroTypeID::RANDOM)
		return genBonusWindow();

	pos = Rect(0, 0, 292, 226);
	genHeader();
	labelHeroSpeciality = std::make_shared<CLabel>(pos.w / 2 + 4, 117, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[78]);

	imageSpeciality = std::make_shared<CAnimImage>(AnimationPath::builtin("UN44"), (*LIBRARY->heroh)[heroIndex]->imageIndex, 0, pos.w / 2 - 22, 134);
	labelSpecialityName = std::make_shared<CLabel>(pos.w / 2, 188, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, (*LIBRARY->heroh)[heroIndex]->getSpecialtyNameTranslated());
}

void OptionsTab::CPlayerOptionTooltipBox::genBonusWindow()
{
	pos = Rect(0, 0, 228, 162);
	genHeader();

	textBonusDescription = std::make_shared<CTextBox>(getDescription(), Rect(10, 100, pos.w - 20, 70), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
}

OptionsTab::SelectionWindow::SelectionWindow(const PlayerColor & color, SelType _type, int sliderPos)
	: CWindowObject(BORDERED), color(color)
{
	addUsedEvents(LCLICK | SHOW_POPUP);

	type = _type;

	initialFaction = SEL->getStartInfo()->playerInfos.find(color)->second.castle;
	initialHero = SEL->getStartInfo()->playerInfos.find(color)->second.hero;
	initialBonus = SEL->getStartInfo()->playerInfos.find(color)->second.bonus;
	selectedFaction = initialFaction;
	selectedHero = initialHero;
	selectedBonus = initialBonus;
	allowedFactions = SEL->getPlayerInfo(color).allowedFactions;
	allowedHeroes = SEL->getMapInfo()->mapHeader->allowedHeroes;

	for(auto & player : SEL->getStartInfo()->playerInfos)
	{
		if(player.first != color && (int)player.second.hero > HeroTypeID::RANDOM)
			unusableHeroes.insert(player.second.hero);
	}

	for (const auto & disposedHero : SEL->getMapInfo()->mapHeader->disposedHeroes)
	{
		if (!disposedHero.players.count(color))
			allowedHeroes.erase(disposedHero.heroId);
	}

	allowedBonus.push_back(PlayerStartingBonus::RANDOM);

	if(initialHero != HeroTypeID::NONE|| SEL->getPlayerInfo(color).heroesNames.size() > 0)
		allowedBonus.push_back(PlayerStartingBonus::ARTIFACT);

	allowedBonus.push_back(PlayerStartingBonus::GOLD);

	if(initialFaction.isValid())
		allowedBonus.push_back(PlayerStartingBonus::RESOURCE);

	recreate(sliderPos);
}

std::tuple<int, int> OptionsTab::SelectionWindow::calcLines(FactionID faction)
{
	int additionalItems = 1; // random

	if(!faction.isValid())
		return std::make_tuple(
			std::ceil(((double)allowedFactions.size() + additionalItems) / MAX_ELEM_PER_LINES),
			(allowedFactions.size() + additionalItems) % MAX_ELEM_PER_LINES
		);

	int count = 0;
	for(auto & elemh : allowedHeroes)
	{
		const CHero * type = elemh.toHeroType();
		if(type->heroClass->faction != faction)
			continue;

		count++;
	}

	return std::make_tuple(
		std::ceil(((double)count + additionalItems) / MAX_ELEM_PER_LINES),
		(count + additionalItems) % MAX_ELEM_PER_LINES
	);
}

void OptionsTab::SelectionWindow::apply()
{
	if(ENGINE->windows().isTopWindow(this))
	{
		ENGINE->input().hapticFeedback();
		ENGINE->sound().playSound(soundBase::button);

		close();

		setSelection();
	}
}

void OptionsTab::SelectionWindow::setSelection()
{
	if(selectedFaction != initialFaction)
		GAME->server().setPlayerOption(LobbyChangePlayerOption::TOWN_ID, selectedFaction, color);

	if(selectedHero != initialHero)
		GAME->server().setPlayerOption(LobbyChangePlayerOption::HERO_ID, selectedHero, color);

	if(selectedBonus != initialBonus)
		GAME->server().setPlayerOption(LobbyChangePlayerOption::BONUS_ID, static_cast<int>(selectedBonus), color);
}

void OptionsTab::SelectionWindow::reopen()
{
	if(type == SelType::HERO && SEL->getStartInfo()->playerInfos.find(color)->second.castle == FactionID::RANDOM)
		close();
	else{
		auto window = std::make_shared<SelectionWindow>(color, type, slider ? slider->getValue() : 0);
		close();
		if(GAME->server().isMyColor(color) || GAME->server().isHost())
			ENGINE->windows().pushWindow(window);
	}
}

void OptionsTab::SelectionWindow::recreate(int sliderPos)
{
	OBJECT_CONSTRUCTION;

	int amountLines = 1;
	if(type == SelType::BONUS)
		elementsPerLine = allowedBonus.size();
	else
	{
		std::tie(amountLines, elementsPerLine) = calcLines((type > SelType::TOWN) ? selectedFaction : FactionID::RANDOM);
		if(amountLines > 1 || elementsPerLine == 0)
			elementsPerLine = MAX_ELEM_PER_LINES;
	}

	int x = (elementsPerLine) * (ICON_BIG_WIDTH-1);
	int y = (std::min(amountLines, MAX_LINES)) * (ICON_BIG_HEIGHT-1);

	int sliderWidth = ((amountLines > MAX_LINES) ? 16 : 0);

	pos = Rect(pos.x, pos.y, x + sliderWidth, y);
	backgroundTexture = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w - sliderWidth, pos.h));
	backgroundTexture->setPlayerColor(PlayerColor(1));
	updateShadow();

	if(type == SelType::TOWN)
		genContentFactions();
	if(type == SelType::HERO)
		genContentHeroes();
	if(type == SelType::BONUS)
		genContentBonus();
	genContentGrid(std::min(amountLines, MAX_LINES));

	if(!slider && amountLines > MAX_LINES)
	{
		slider = std::make_shared<CSlider>(Point(x, 0), y, std::bind(&OptionsTab::SelectionWindow::sliderMove, this, _1), MAX_LINES, amountLines, 0, Orientation::VERTICAL, CSlider::BLUE);
		slider->setPanningStep(ICON_BIG_HEIGHT);
		slider->setScrollBounds(Rect(-pos.w + slider->pos.w, 0, x + slider->pos.w, y));
		slider->scrollTo(sliderPos);
	}

	center();
}

void OptionsTab::SelectionWindow::drawOutlinedText(int x, int y, ColorRGBA color, std::string text)
{
	components.push_back(std::make_shared<CLabel>(x-1, y, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text, 56));
	components.push_back(std::make_shared<CLabel>(x+1, y, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text, 56));
	components.push_back(std::make_shared<CLabel>(x, y-1, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text, 56));
	components.push_back(std::make_shared<CLabel>(x, y+1, FONT_TINY, ETextAlignment::CENTER, Colors::BLACK, text, 56));
	components.push_back(std::make_shared<CLabel>(x, y, FONT_TINY, ETextAlignment::CENTER, color, text, 56));
}

void OptionsTab::SelectionWindow::genContentGrid(int lines)
{
	for(int y = 0; y < lines; y++)
	{
		for(int x = 0; x < elementsPerLine; x++)
		{
			components.push_back(std::make_shared<CPicture>(ImagePath::builtin("lobby/townBorderBig"), x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
		}
	}
}

void OptionsTab::SelectionWindow::genContentFactions()
{
	int i = 1;

	// random
	PlayerSettings set = PlayerSettings();
	set.castle = FactionID::RANDOM;
	CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
	components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, 6, (ICON_SMALL_HEIGHT/2)));
	drawOutlinedText(TEXT_POS_X, TEXT_POS_Y, (selectedFaction == FactionID::RANDOM) ? Colors::YELLOW : Colors::WHITE, helper.getName());
	if(selectedFaction == FactionID::RANDOM)
		components.push_back(std::make_shared<CPicture>(ImagePath::builtin("lobby/townBorderSmallActivated"), 6, (ICON_SMALL_HEIGHT/2)));

	factions.clear();
	for(auto & elem : allowedFactions)
	{
		int x = i % elementsPerLine;
		int y = (i / elementsPerLine) - (slider ? slider->getValue() : 0);

		PlayerSettings set = PlayerSettings();
		set.castle = elem;

		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);

		factions.push_back(elem);
		i++;

		if(y < 0 || y > MAX_LINES - 1)
			continue;
			
		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
		components.push_back(std::make_shared<CPicture>(ImagePath::builtin(selectedFaction == elem ? "lobby/townBorderBigActivated" : "lobby/townBorderBig"), x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
		drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, (selectedFaction == elem) ? Colors::YELLOW : Colors::WHITE, helper.getName());
	}
}

void OptionsTab::SelectionWindow::genContentHeroes()
{
	int i = 1;

	// random
	PlayerSettings set = PlayerSettings();
	set.hero = HeroTypeID::RANDOM;
	CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
	components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, 6, (ICON_SMALL_HEIGHT/2)));
	drawOutlinedText(TEXT_POS_X, TEXT_POS_Y, (selectedHero == HeroTypeID::RANDOM) ? Colors::YELLOW : Colors::WHITE, helper.getName());
	if(selectedHero == HeroTypeID::RANDOM)
		components.push_back(std::make_shared<CPicture>(ImagePath::builtin("lobby/townBorderSmallActivated"), 6, (ICON_SMALL_HEIGHT/2)));

	heroes.clear();
	for(auto & elem : allowedHeroes)
	{
		const CHero * type = elem.toHeroType();

		if(type->heroClass->faction != selectedFaction)
			continue;

		int x = i % elementsPerLine;
		int y = (i / elementsPerLine) - (slider ? slider->getValue() : 0);

		PlayerSettings set = PlayerSettings();
		set.hero = elem;

		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);

		heroes.push_back(elem);
		i++;

		if(y < 0 || y > MAX_LINES - 1)
			continue;

		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(true), helper.getImageIndex(true), 0, x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
		drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, (selectedHero == elem) ? Colors::YELLOW : Colors::WHITE, helper.getName());
		ImagePath image = ImagePath::builtin("lobby/townBorderBig");
		if(selectedHero == elem)
			image = ImagePath::builtin("lobby/townBorderBigActivated");
		if(unusableHeroes.count(elem))
			image = ImagePath::builtin("lobby/townBorderBigGrayedOut");
		components.push_back(std::make_shared<CPicture>(image, x * (ICON_BIG_WIDTH-1), y * (ICON_BIG_HEIGHT-1)));
	}
}

void OptionsTab::SelectionWindow::genContentBonus()
{
	PlayerSettings set = SEL->getStartInfo()->playerInfos.find(color)->second;

	int i = 0;
	for(auto elem : allowedBonus)
	{
		int x = i;
		int y = 0;

		set.bonus = elem;
		CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
		components.push_back(std::make_shared<CAnimImage>(helper.getImageName(), helper.getImageIndex(), 0, x * (ICON_BIG_WIDTH-1) + 6, y * (ICON_BIG_HEIGHT-1) + (ICON_SMALL_HEIGHT/2)));
		drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, Colors::WHITE , helper.getName());
		if(selectedBonus == elem)
		{
			components.push_back(std::make_shared<CPicture>(ImagePath::builtin("lobby/townBorderSmallActivated"), x * (ICON_BIG_WIDTH-1) + 6, y * (ICON_BIG_HEIGHT-1) + (ICON_SMALL_HEIGHT/2)));
			drawOutlinedText(x * (ICON_BIG_WIDTH-1) + TEXT_POS_X, y * (ICON_BIG_HEIGHT-1) + TEXT_POS_Y, Colors::YELLOW , helper.getName());
		}

		i++;
	}
}

int OptionsTab::SelectionWindow::getElement(const Point & cursorPosition)
{
	int x = (cursorPosition.x - pos.x) / (ICON_BIG_WIDTH-1);
	int y = (cursorPosition.y - pos.y) / (ICON_BIG_HEIGHT-1) + (slider ? slider->getValue() : 0);

	return x + y * elementsPerLine;
}

void OptionsTab::SelectionWindow::setElement(int elem, bool doApply)
{
	PlayerSettings set = PlayerSettings();
	if(type == SelType::TOWN)
	{
		if(elem > 0)
		{
			elem--;
			if(elem >= factions.size())
				return;
			set.castle = factions[elem];
		}
		else
		{
			set.castle = FactionID::RANDOM;
		}
		if(set.castle != FactionID::NONE)
		{
			if(!doApply)
			{
				CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::TOWN);
				ENGINE->windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
			}
			else
				selectedFaction = set.castle;
		}
	}
	if(type == SelType::HERO)
	{
		if(elem > 0)
		{
			elem--;
			if(elem >= heroes.size())
				return;
			set.hero = heroes[elem];
		}
		else
		{
			set.hero = HeroTypeID::RANDOM;
		}

		if(doApply && unusableHeroes.count(heroes[elem]))
			return;

		if(set.hero != HeroTypeID::NONE)
		{
			if(!doApply)
			{
				CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::HERO);
				if(settings["general"]["enableUiEnhancements"].Bool() && helper.playerSettings.hero.isValid() && helper.playerSettings.heroNameTextId.empty())
					ENGINE->windows().createAndPushWindow<CHeroOverview>(helper.playerSettings.hero);
				else
					ENGINE->windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
			}
			else
				selectedHero = set.hero;
		}
	}
	if(type == SelType::BONUS)
	{
		if(elem >= 4)
			return;
		set.bonus = static_cast<PlayerStartingBonus>(allowedBonus[elem]);

		if(!doApply)
		{
			CPlayerSettingsHelper helper = CPlayerSettingsHelper(set, SelType::BONUS);
			ENGINE->windows().createAndPushWindow<CPlayerOptionTooltipBox>(helper);
		}
		else
			selectedBonus = set.bonus;
	}

	if(doApply)
		apply();
}

void OptionsTab::SelectionWindow::sliderMove(int slidPos)
{
	if(!slider)
		return; // ignore spurious call when slider is being created
	recreate();
	redraw();
}

void OptionsTab::SelectionWindow::notFocusedClick()
{
	close();
}

void OptionsTab::SelectionWindow::clickReleased(const Point & cursorPosition)
{
	if(slider && slider->pos.isInside(cursorPosition))
		return;

	int elem = getElement(cursorPosition);

	setElement(elem, true);
}

void OptionsTab::SelectionWindow::showPopupWindow(const Point & cursorPosition)
{
	if(!pos.isInside(cursorPosition) || (slider && slider->pos.isInside(cursorPosition)))
		return;

	int elem = getElement(cursorPosition);

	setElement(elem, false);
}

OptionsTab::HandicapWindow::HandicapWindow()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK);

	pos = Rect(0, 0, 660, 100 + SEL->getStartInfo()->playerInfos.size() * 30);

	backgroundTexture = std::make_shared<FilledTexturePlayerColored>(pos);
	backgroundTexture->setPlayerColor(PlayerColor(1));

	labels.push_back(std::make_shared<CLabel>(pos.w / 2 + 8, 15, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.handicap")));

	enum Columns : int32_t
	{
		INCOME = 1000,
		GROWTH = 2000,
	};
	auto columns = std::vector<int>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS, Columns::INCOME, Columns::GROWTH};

	int i = 0;
	for(auto & pInfo : SEL->getStartInfo()->playerInfos)
	{
		PlayerColor player = pInfo.first;
		anim.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ITGFLAGS"), player.getNum(), 0, 7, 57 + i * 30));
		for(int j = 0; j < columns.size(); j++)
		{
			bool isIncome = int(columns[j]) == Columns::INCOME;
			bool isGrowth = int(columns[j]) == Columns::GROWTH;
			EGameResID resource = columns[j];

			const PlayerSettings &ps = SEL->getStartInfo()->getIthPlayersSettings(player);

			int xPos = 30 + j * 70;
			xPos += j > 0 ? 10 : 0; // Gold field is larger

			if(i == 0)
			{
				if(isIncome)
					labels.push_back(std::make_shared<CLabel>(xPos, 38, FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("core.jktext.32")));
				else if(isGrowth)
					labels.push_back(std::make_shared<CLabel>(xPos, 38, FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("core.genrltxt.194")));
				else
					anim.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("SMALRES"), GameResID(resource).getNum(), 0, 15 + xPos + (j == 0 ? 10 : 0), 35));
			}

			auto area = Rect(xPos, 60 + i * 30, j == 0 ? 60 : 50, 16);
			textinputbackgrounds.push_back(std::make_shared<TransparentFilledRectangle>(area.resize(3), ColorRGBA(0,0,0,128), ColorRGBA(64,64,64,64)));
			textinputs[player][resource] = std::make_shared<CTextInput>(area, FONT_SMALL, ETextAlignment::CENTERLEFT, true);
			textinputs[player][resource]->setText(std::to_string(isIncome ? ps.handicap.percentIncome : (isGrowth ? ps.handicap.percentGrowth : ps.handicap.startBonus[resource])));
			textinputs[player][resource]->setCallback([this, player, resource, isIncome, isGrowth](const std::string & s){
				// text input processing: add/remove sign when pressing "-"; remove non digits; cut length; fill empty field with 0
				std::string tmp = s;
				bool negative = std::count_if( s.begin(), s.end(), []( char c ){ return c == '-'; }) == 1 && !isIncome && !isGrowth;
				tmp.erase(std::remove_if(tmp.begin(), tmp.end(), [](char c) { return !isdigit(c); }), tmp.end());
				int maxLength = isIncome || isGrowth ? 3 : (resource == EGameResID::GOLD ? 6 : 5);
				tmp = tmp.substr(0, maxLength);
				textinputs[player][resource]->setText(tmp.length() == 0 ? "0" : (negative ? "-" : "") + std::to_string(stoi(tmp)));
			});
			textinputs[player][resource]->setPopupCallback([isIncome, isGrowth](){
				// Help for the textinputs
				if(isIncome)
					CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.lobby.handicap.income"));
				else if(isGrowth)
					CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.lobby.handicap.growth"));
				else
					CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.lobby.handicap.resource"));
			});
			if(isIncome || isGrowth)
				labels.push_back(std::make_shared<CLabel>(area.topRight().x, area.center().y, FONT_SMALL, ETextAlignment::CENTERRIGHT, Colors::WHITE, "%"));
		}
		i++;
	}
	
	buttons.push_back(std::make_shared<CButton>(Point(pos.w / 2 - 32, 60 + SEL->getStartInfo()->playerInfos.size() * 30), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this](){
		for (const auto& player : textinputs)
		{
			TResources resources = TResources();
			int income = 100;
			int growth = 100;
			for (const auto& resource : player.second)
			{
				bool isIncome = int(resource.first) == Columns::INCOME;
				bool isGrowth = int(resource.first) == Columns::GROWTH;
				if(isIncome)
					income = std::stoi(resource.second->getText());
				else if(isGrowth)
					growth = std::stoi(resource.second->getText());
				else
					resources[resource.first] = std::stoi(resource.second->getText());
			}
			GAME->server().setPlayerHandicap(player.first, Handicap{resources, income, growth});
		}
	    		
		close();
	}, EShortcut::GLOBAL_RETURN));

	updateShadow();
	center();
}

void OptionsTab::HandicapWindow::notFocusedClick()
{
	close();
}

OptionsTab::SelectedBox::SelectedBox(Point position, PlayerSettings & playerSettings, SelType type)
	: Scrollable(LCLICK | SHOW_POPUP, position, Orientation::HORIZONTAL)
	, CPlayerSettingsHelper(playerSettings, type)
{
	OBJECT_CONSTRUCTION;

	image = std::make_shared<CAnimImage>(getImageName(), getImageIndex());
	subtitle = std::make_shared<CLabel>(24, 39, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, getName(), 71);

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
	if(playerSettings.castle == FactionID::NONE && CPlayerSettingsHelper::selectionType == TOWN)
		return;
	if(playerSettings.hero == HeroTypeID::NONE && !SEL->getPlayerInfo(playerSettings.color).hasCustomMainHero() && CPlayerSettingsHelper::selectionType == HERO)
		return;

	if(settings["general"]["enableUiEnhancements"].Bool() && CPlayerSettingsHelper::selectionType == HERO && playerSettings.hero.isValid() && playerSettings.heroNameTextId.empty())
		ENGINE->windows().createAndPushWindow<CHeroOverview>(playerSettings.hero);
	else
		ENGINE->windows().createAndPushWindow<CPlayerOptionTooltipBox>(*this);
}

void OptionsTab::SelectedBox::clickReleased(const Point & cursorPosition)
{
	if (!SEL)
		return;

	if(SEL->screenType != ESelectionScreen::newGame)
		return;
	
	PlayerInfo pi = SEL->getPlayerInfo(playerSettings.color);
	const bool foreignPlayer = GAME->server().isGuest() && !GAME->server().isMyColor(playerSettings.color);

	if(selectionType == SelType::TOWN && ((pi.allowedFactions.size() < 2 && !pi.isFactionRandom) || foreignPlayer))
		return;

	if(selectionType == SelType::HERO && ((pi.defaultHero() == HeroTypeID::NONE || !playerSettings.castle.isValid() || foreignPlayer)))
		return;

	if(selectionType == SelType::BONUS && foreignPlayer)
		return;

	ENGINE->input().hapticFeedback();
	ENGINE->windows().createAndPushWindow<SelectionWindow>(playerSettings.color, selectionType);
}

void OptionsTab::SelectedBox::scrollBy(int distance)
{
	// FIXME: currently options tab is completely recreacted from scratch whenever we receive any information from server
	// because of that, panning event gets interrupted (due to destruction of element)
	// so, currently, gesture will always move selection only by 1, and then wait for recreation from server info
	distance = std::clamp(distance, -1, 1);

	switch(CPlayerSettingsHelper::selectionType)
	{
		case TOWN:
			GAME->server().setPlayerOption(LobbyChangePlayerOption::TOWN, distance, playerSettings.color);
			break;
		case HERO:
			GAME->server().setPlayerOption(LobbyChangePlayerOption::HERO, distance, playerSettings.color);
			break;
		case BONUS:
			GAME->server().setPlayerOption(LobbyChangePlayerOption::BONUS, distance, playerSettings.color);
			break;
	}

	setScrollingEnabled(false);
}

OptionsTab::PlayerOptionsEntry::PlayerOptionsEntry(const PlayerSettings & S, const OptionsTab & parent)
	: CIntObject(LCLICK | KEYBOARD | TEXTINPUT)
	, pi(std::make_unique<PlayerInfo>(SEL->getPlayerInfo(S.color)))
	, s(std::make_unique<PlayerSettings>(S))
	, parentTab(parent)
	, name(S.name)
{
	OBJECT_CONSTRUCTION;

	int serial = 0;
	for(PlayerColor g = PlayerColor(0); g < s->color; ++g)
	{
		auto itred = SEL->getPlayerInfo(g);
		if(itred.canComputerPlay || itred.canHumanPlay)
			serial++;
	}

	pos.x += 54;
	pos.y += 128 + serial * 50;

	assert(GAME->server().mi && GAME->server().mi->mapHeader);
	const PlayerInfo & p = SEL->getPlayerInfo(s->color);
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

	background = std::make_shared<CPicture>(ImagePath::builtin(bgs[s->color]), 0, 0);
	if(s->isControlledByAI() || GAME->server().isGuest())
		labelPlayerName = std::make_shared<CLabel>(55, 10, EFonts::FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, name, 95);
	else
	{
		labelPlayerNameEdit = std::make_shared<CTextInput>(Rect(6, 3, 95, 15), EFonts::FONT_SMALL, ETextAlignment::CENTER, false);
		labelPlayerNameEdit->setText(name);
	}

	labelWhoCanPlay = std::make_shared<CMultiLineLabel>(Rect(6, 21, 45, 26), EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->arraytxt[206 + whoCanPlay]);

	auto hasHandicap = [this](){ return s->handicap.startBonus.empty() && s->handicap.percentIncome == 100 && s->handicap.percentGrowth == 100; };
	std::string labelHandicapText = hasHandicap() ? LIBRARY->generaltexth->arraytxt[210] : MetaString::createFromTextID("vcmi.lobby.handicap").toString();
	labelHandicap = std::make_shared<CMultiLineLabel>(Rect(55, 23, 46, 24), EFonts::FONT_TINY, ETextAlignment::CENTER, Colors::WHITE, labelHandicapText);
	handicap = std::make_shared<LRClickableArea>(Rect(53, 23, 50, 24), [](){
		if(!GAME->server().isHost())
			return;
		
		ENGINE->windows().createAndPushWindow<HandicapWindow>();
	}, [this, hasHandicap](){
		if(hasHandicap())
			CRClickPopup::createAndPush(MetaString::createFromTextID("core.help.124.help").toString());
		else
		{
			auto str = MetaString::createFromTextID("vcmi.lobby.handicap");
			str.appendRawString(":\n");
			for(auto & res : EGameResID::ALL_RESOURCES())
				if(s->handicap.startBonus[res] != 0)
				{
					str.appendRawString("\n");
					str.appendName(res);
					str.appendRawString(": ");
					str.appendRawString(std::to_string(s->handicap.startBonus[res]));
				}
			if(s->handicap.percentIncome != 100)
			{
				str.appendRawString("\n");
				str.appendTextID("core.jktext.32");
				str.appendRawString(": ");
				str.appendRawString(std::to_string(s->handicap.percentIncome) + "%");
			}
			if(s->handicap.percentGrowth != 100)
			{
				str.appendRawString("\n");
				str.appendTextID("core.genrltxt.194");
				str.appendRawString(": ");
				str.appendRawString(std::to_string(s->handicap.percentGrowth) + "%");
			}
			CRClickPopup::createAndPush(str.toString());
		}
	});

	if(SEL->screenType == ESelectionScreen::newGame)
	{
		buttonTownLeft   = std::make_shared<CButton>(Point(107, 5), AnimationPath::builtin("ADOPLFA.DEF"), LIBRARY->generaltexth->zelp[132], std::bind(&IServerAPI::setPlayerOption, &GAME->server(), LobbyChangePlayerOption::TOWN, -1, s->color));
		buttonTownRight  = std::make_shared<CButton>(Point(168, 5), AnimationPath::builtin("ADOPRTA.DEF"), LIBRARY->generaltexth->zelp[133], std::bind(&IServerAPI::setPlayerOption, &GAME->server(), LobbyChangePlayerOption::TOWN, +1, s->color));
		buttonHeroLeft   = std::make_shared<CButton>(Point(183, 5), AnimationPath::builtin("ADOPLFA.DEF"), LIBRARY->generaltexth->zelp[148], std::bind(&IServerAPI::setPlayerOption, &GAME->server(), LobbyChangePlayerOption::HERO, -1, s->color));
		buttonHeroRight  = std::make_shared<CButton>(Point(244, 5), AnimationPath::builtin("ADOPRTA.DEF"), LIBRARY->generaltexth->zelp[149], std::bind(&IServerAPI::setPlayerOption, &GAME->server(), LobbyChangePlayerOption::HERO, +1, s->color));
		buttonBonusLeft  = std::make_shared<CButton>(Point(259, 5), AnimationPath::builtin("ADOPLFA.DEF"), LIBRARY->generaltexth->zelp[164], std::bind(&IServerAPI::setPlayerOption, &GAME->server(), LobbyChangePlayerOption::BONUS, -1, s->color));
		buttonBonusRight = std::make_shared<CButton>(Point(320, 5), AnimationPath::builtin("ADOPRTA.DEF"), LIBRARY->generaltexth->zelp[165], std::bind(&IServerAPI::setPlayerOption, &GAME->server(), LobbyChangePlayerOption::BONUS, +1, s->color));
	}

	hideUnavailableButtons();

	if(SEL->screenType != ESelectionScreen::scenarioInfo && SEL->getPlayerInfo(s->color).canHumanPlay)
	{
		flag = std::make_shared<CButton>(
			Point(-43, 2),
			AnimationPath::builtin(flags[s->color.getNum()]),
			LIBRARY->generaltexth->zelp[180],
			std::bind(&OptionsTab::onSetPlayerClicked, &parentTab, *s)
		);
		flag->setHoverable(true);
		flag->block(GAME->server().isGuest());
	}
	else
		flag = nullptr;

	town = std::make_shared<SelectedBox>(Point(119, 2), *s, TOWN);
	hero = std::make_shared<SelectedBox>(Point(195, 2), *s, HERO);
	bonus = std::make_shared<SelectedBox>(Point(271, 2), *s, BONUS);
}

bool OptionsTab::PlayerOptionsEntry::captureThisKey(EShortcut key)
{
	return labelPlayerNameEdit && labelPlayerNameEdit->hasFocus() && key == EShortcut::GLOBAL_ACCEPT;
}

void OptionsTab::PlayerOptionsEntry::keyPressed(EShortcut key)
{
	if(labelPlayerNameEdit && key == EShortcut::GLOBAL_ACCEPT)
		updateName();
}

bool OptionsTab::PlayerOptionsEntry::receiveEvent(const Point & position, int eventType) const
{
	return eventType == AEventsReceiver::LCLICK; // capture all left clicks (not only within control)
}

void OptionsTab::PlayerOptionsEntry::clickReleased(const Point & cursorPosition)
{
	if(labelPlayerNameEdit && !labelPlayerNameEdit->pos.isInside(cursorPosition))
		updateName();
}

void OptionsTab::PlayerOptionsEntry::updateName() {
	if(labelPlayerNameEdit->getText() != name)
	{
		GAME->server().setPlayerName(s->color, labelPlayerNameEdit->getText());
		if(GAME->server().isMyColor(s->color))
		{
			Settings set = settings.write["general"]["playerName"];
			set->String() = labelPlayerNameEdit->getText();
		}
	}

	labelPlayerNameEdit->removeFocus();
	name = labelPlayerNameEdit->getText();
}

void OptionsTab::onSetPlayerClicked(const PlayerSettings & ps) const
{
	if(ps.isControlledByAI() || humanPlayers > 1)
		GAME->server().setPlayer(ps.color);
}

void OptionsTab::PlayerOptionsEntry::hideUnavailableButtons()
{
	if(!buttonTownLeft)
		return;

	const bool foreignPlayer = GAME->server().isGuest() && !GAME->server().isMyColor(s->color);

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

	if((pi->defaultHero() != HeroTypeID::RANDOM || !s->castle.isValid()) //fixed hero
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
