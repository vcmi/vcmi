/*
 * CSelectionBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CSelectionBase.h"
#include "CBonusSelection.h"
#include "CLobbyScreen.h"
#include "OptionsTab.h"
#include "RandomMapTab.h"
#include "SelectionTab.h"

#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../globalLobby/GlobalLobbyClient.h"
#include "../mainmenu/CMainMenu.h"
#include "../media/ISoundPlayer.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/CTextInput.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../render/CAnimation.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/GameLibrary.h"

ISelectionScreenInfo::ISelectionScreenInfo(ESelectionScreen ScreenType)
	: screenType(ScreenType)
{
	assert(!SEL);
	SEL = this;
}

ISelectionScreenInfo::~ISelectionScreenInfo()
{
	assert(SEL == this);
	SEL = nullptr;
}

int ISelectionScreenInfo::getCurrentDifficulty()
{
	return getStartInfo()->difficulty;
}

PlayerInfo ISelectionScreenInfo::getPlayerInfo(PlayerColor color)
{
	auto mapInfo = getMapInfo();
	if (!mapInfo)
		throw std::runtime_error("Attempt to get player info for invalid map!");

	if (!mapInfo->mapHeader)
		throw std::runtime_error("Attempt to get player info for invalid map header!");

	return mapInfo->mapHeader->players.at(color.getNum());
}

CSelectionBase::CSelectionBase(ESelectionScreen type)
	: CWindowObject(BORDERED | SHADOW_DISABLED), ISelectionScreenInfo(type)
{
	OBJECT_CONSTRUCTION;
	pos.w = 762;
	pos.h = 584;
	if(screenType == ESelectionScreen::campaignList)
	{
		setBackground(ImagePath::builtin("CamCust.bmp"));
		pos = background->center();
	}
	else
	{

		const JsonNode& gameSelectConfig = CMainMenuConfig::get().getConfig()["scenario-selection"];
		const JsonVector* bgNames = nullptr;

		if (!gameSelectConfig.isStruct()) 
			bgNames = &CMainMenuConfig::get().getConfig()["game-select"].Vector();  // Fallback for 1.6 mods
		else
			bgNames = &gameSelectConfig["background"].Vector();
		

		setBackground(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(*bgNames, CRandomGenerator::getDefault())));
		pos = background->center();

		for (const JsonNode& node : gameSelectConfig["images"].Vector())
		{
			auto image = std::make_shared<CPicture>(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(node["name"].Vector(), CRandomGenerator::getDefault())), Point(node["x"].Integer(), node["y"].Integer()));
			images.push_back(image);
		}
	}
	
	card = std::make_shared<InfoCard>();
	buttonBack = std::make_shared<CButton>(Point(581, 535), AnimationPath::builtin("SCNRBACK.DEF"), LIBRARY->generaltexth->zelp[105], [this](){ close();}, EShortcut::GLOBAL_CANCEL);
}

void CSelectionBase::toggleTab(std::shared_ptr<CIntObject> tab)
{
	if(curTab && curTab->isActive())
	{
		curTab->disable();
	}

	if(curTab != tab)
	{
		tab->enable();
		curTab = tab;
	}
	else
	{
		curTab.reset();
	}

	if(tabSel->showRandom && tab != tabOpt)
	{
		tabSel->curFolder = "";
		tabSel->showRandom = false;
		tabSel->filter(0, true);
	}

	ENGINE->windows().totalRedraw();
}

InfoCard::InfoCard()
	: showChat(true)
{
	OBJECT_CONSTRUCTION;
	setRedrawParent(true);
	pos.x += 393;
	pos.y += 6;

	labelSaveDate = std::make_shared<CLabel>(305, 38, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE);
	labelMapSize = std::make_shared<CLabel>(327, 57, FONT_TINY, ETextAlignment::CENTER, Colors::WHITE);
	mapName = std::make_shared<CLabel>(26, 39, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, "", SEL->screenType == ESelectionScreen::campaignList ? 325 : 280);
	Rect descriptionRect(26, 149, 320, 115);
	mapDescription = std::make_shared<CTextBox>("", descriptionRect, 1);
	playerListBg = std::make_shared<CPicture>(ImagePath::builtin("CHATPLUG.bmp"), 16, 276);
	chat = std::make_shared<CChatBox>(Rect(18, 126, 335, 143));
	pvpBox = std::make_shared<PvPBox>(Rect(17, 396, 338, 105));

	buttonInvitePlayers = std::make_shared<CButton>(Point(20, 365), AnimationPath::builtin("pregameInvitePlayers"), LIBRARY->generaltexth->zelp[105], [](){ GAME->server().getGlobalLobby().activateRoomInviteInterface(); }, EShortcut::LOBBY_INVITE_PLAYERS );
	buttonOpenGlobalLobby = std::make_shared<CButton>(Point(188, 365), AnimationPath::builtin("pregameReturnToLobby"), LIBRARY->generaltexth->zelp[105], [](){ GAME->server().getGlobalLobby().activateInterface(); }, EShortcut::MAIN_MENU_LOBBY );

	buttonInvitePlayers->setTextOverlay  (MetaString::createFromTextID("vcmi.lobby.invite.header").toString(), EFonts::FONT_SMALL, Colors::WHITE);
	buttonOpenGlobalLobby->setTextOverlay(MetaString::createFromTextID("vcmi.lobby.backToLobby").toString(), EFonts::FONT_SMALL, Colors::WHITE);

	if(SEL->screenType == ESelectionScreen::campaignList)
	{
		labelCampaignDescription = std::make_shared<CLabel>(26, 132, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[38]);
	}
	else
	{
		background = std::make_shared<CPicture>(ImagePath::builtin("GSELPOP1.bmp"), 0, 0);
		parent->addChild(background.get());
		auto it = vstd::find(parent->children, this); //our position among parent children
		parent->children.insert(it, background.get()); //put BG before us
		parent->children.pop_back();
		pos.w = background->pos.w;
		pos.h = background->pos.h;
		iconsMapSizes = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRMPSZ"), 4, 0, 313, 25); //let it be custom size (frame 4) by default

		iconDifficulty = std::make_shared<CToggleGroup>(0);
		{
			constexpr std::array difButns = {"GSPBUT3.DEF", "GSPBUT4.DEF", "GSPBUT5.DEF", "GSPBUT6.DEF", "GSPBUT7.DEF"};

			for(int i = 0; i < 5; i++)
			{
				auto button = std::make_shared<CToggleButton>(Point(110 + i * 32, 450), AnimationPath::builtin(difButns[i]), LIBRARY->generaltexth->zelp[24 + i]);

				iconDifficulty->addToggle(i, button);
				if(SEL->screenType != ESelectionScreen::newGame)
					button->block(true);
			}
		}

		flagbox = std::make_shared<CFlagBox>(Rect(24, 400, 335, 23));
		labelMapDiff = std::make_shared<CLabel>(33, 430, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[494]);
		labelPlayerDifficulty = std::make_shared<CLabel>(133, 430, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[492] + ":");
		labelRating = std::make_shared<CLabel>(290, 430, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[218] + ":");
		labelScenarioName = std::make_shared<CLabel>(26, 22, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[495]);
		labelScenarioDescription = std::make_shared<CLabel>(26, 132, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[496]);
		labelVictoryCondition = std::make_shared<CLabel>(26, 283, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[497]);
		labelLossCondition = std::make_shared<CLabel>(26, 339, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::YELLOW, LIBRARY->generaltexth->allTexts[498]);
		iconsVictoryCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRVICT"), 0, 0, 24, 302);
		iconsLossCondition = std::make_shared<CAnimImage>(AnimationPath::builtin("SCNRLOSS"), 0, 0, 24, 359);

		labelVictoryConditionText = std::make_shared<CLabel>(60, 307, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, "", 290);
		labelLossConditionText = std::make_shared<CLabel>(60, 366, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, "", 290);

		labelDifficulty = std::make_shared<CLabel>(62, 472, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
		labelDifficultyPercent = std::make_shared<CLabel>(311, 472, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

		labelGroupPlayers = std::make_shared<CLabelGroup>(FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
		disableLabelRedraws();
	}
	setChat(false);
	if (GAME->server().inLobbyRoom())
		setChat(true); // FIXME: less ugly version?
}

void InfoCard::disableLabelRedraws()
{
	labelSaveDate->setAutoRedraw(false);
	labelMapSize->setAutoRedraw(false);
	mapName->setAutoRedraw(false);
	mapDescription->label->setAutoRedraw(false);
	labelVictoryConditionText->setAutoRedraw(false);
	labelLossConditionText->setAutoRedraw(false);
	labelDifficulty->setAutoRedraw(false);
	labelDifficultyPercent->setAutoRedraw(false);
}

void InfoCard::changeSelection()
{
	auto mapInfo = SEL->getMapInfo();
	if(!mapInfo)
		return;

	labelSaveDate->setText(mapInfo->date);
	mapName->setText(mapInfo->getNameTranslated());
	mapDescription->setText(mapInfo->getDescriptionTranslated());

	mapDescription->label->scrollTextTo(0, false);
	if(mapDescription->slider)
		mapDescription->slider->scrollToMin();

	if(SEL->screenType == ESelectionScreen::campaignList)
		return;

	const CMapHeader * header = mapInfo->mapHeader.get();

	labelMapSize->setText(std::to_string(header->width) + "x" + std::to_string(header->height) + "x" + std::to_string(header->mapLevels));
	iconsMapSizes->setFrame(mapInfo->getMapSizeIconId());

	iconsVictoryCondition->setFrame(header->victoryIconIndex);
	labelVictoryConditionText->setText(header->victoryMessage.toString());
	iconsLossCondition->setFrame(header->defeatIconIndex);
	labelLossConditionText->setText(header->defeatMessage.toString());
	flagbox->recreate();
	labelDifficulty->setText(LIBRARY->generaltexth->arraytxt[142 + vstd::to_underlying(mapInfo->mapHeader->difficulty)]);
	iconDifficulty->activate();
	iconDifficulty->setSelected(SEL->getCurrentDifficulty());
	if(SEL->screenType == ESelectionScreen::loadGame || SEL->screenType == ESelectionScreen::saveGame)
		for(auto & button : iconDifficulty->buttons)
			button.second->setEnabled(button.first == SEL->getCurrentDifficulty());

	const std::array<std::string, 5> difficultyPercent = {"80%", "100%", "130%", "160%", "200%"};
	labelDifficultyPercent->setText(difficultyPercent[SEL->getCurrentDifficulty()]);

	OBJECT_CONSTRUCTION;
	// FIXME: We recreate them each time because CLabelGroup don't use smart pointers
	labelGroupPlayers = std::make_shared<CLabelGroup>(FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
	if(!showChat)
		labelGroupPlayers->disable();

	const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);

	for(const auto & p : GAME->server().playerNames)
	{
		int slotsUsed = labelGroupPlayers->currentSize();
		Point labelPosition;

		if(slotsUsed < 4)
			labelPosition = Point(24, 285 + slotsUsed * font->getLineHeight()); // left column
		else
			labelPosition = Point(193, 285 + (slotsUsed - 4) * font->getLineHeight()); // right column

		labelGroupPlayers->add(labelPosition.x, labelPosition.y, p.second.name);
	}
}

void InfoCard::clearSelection()
{
	labelSaveDate->setText("");
	mapName->setText("");
	mapDescription->setText("");

	if(SEL->screenType == ESelectionScreen::campaignList)
		return;

	labelMapSize->setText("");

	labelVictoryConditionText->setText("");
	labelLossConditionText->setText("");
	iconDifficulty->deactivate();
	labelDifficulty->setText("");
	labelDifficultyPercent->setText("");
}

void InfoCard::toggleChat()
{
	setChat(!showChat);
}

void InfoCard::setChat(bool activateChat)
{
	if(showChat == activateChat)
		return;

	if(activateChat)
	{
		if(SEL->screenType == ESelectionScreen::campaignList)
		{
			labelCampaignDescription->disable();
		}
		else
		{
			labelScenarioDescription->disable();
			labelVictoryCondition->disable();
			labelLossCondition->disable();
			iconsVictoryCondition->disable();
			labelVictoryConditionText->disable();
			iconsLossCondition->disable();
			labelLossConditionText->disable();
			labelGroupPlayers->enable();
			labelMapDiff->disable();
			labelPlayerDifficulty->disable();
			labelRating->disable();
			labelDifficulty->disable();
			labelDifficultyPercent->disable();
			flagbox->disable();
			iconDifficulty->disable();
			mapDescription->disable();
			chat->enable();
			pvpBox->enable();
			playerListBg->enable();
		}
		if (GAME->server().inLobbyRoom())
		{
			buttonInvitePlayers->enable();
			buttonOpenGlobalLobby->enable();
		}
	}
	else
	{
		buttonInvitePlayers->disable();
		buttonOpenGlobalLobby->disable();
		mapDescription->enable();
		chat->disable();
		pvpBox->disable();
		playerListBg->disable();

		if(SEL->screenType == ESelectionScreen::campaignList)
		{
			labelCampaignDescription->enable();
		}
		else
		{
			labelScenarioDescription->enable();
			labelVictoryCondition->enable();
			labelLossCondition->enable();
			iconsVictoryCondition->enable();
			iconsLossCondition->enable();
			labelVictoryConditionText->enable();
			labelLossConditionText->enable();
			labelGroupPlayers->disable();
			labelMapDiff->enable();
			labelPlayerDifficulty->enable();
			labelRating->enable();
			labelDifficulty->enable();
			labelDifficultyPercent->enable();
			flagbox->enable();
			iconDifficulty->enable();
		}
	}

	showChat = activateChat;
	ENGINE->windows().totalRedraw();
}

CChatBox::CChatBox(const Rect & rect)
	: CIntObject(KEYBOARD | TEXTINPUT)
{
	OBJECT_CONSTRUCTION;
	pos += rect.topLeft();
	setRedrawParent(true);

	const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
	const int height = font->getLineHeight();
	Rect textInputArea(1, rect.h - height, rect.w - 1, height);
	Rect chatHistoryArea(3, 1, rect.w - 3, rect.h - height - 1);
	inputBackground = std::make_shared<TransparentFilledRectangle>(textInputArea, ColorRGBA(0,0,0,192));
	inputBox = std::make_shared<CTextInput>(textInputArea, EFonts::FONT_SMALL, ETextAlignment::CENTERLEFT, true);
	inputBox->removeUsedEvents(KEYBOARD);
	chatHistory = std::make_shared<CTextBox>("", chatHistoryArea, 1);

	chatHistory->label->color = Colors::GREEN;
}

bool CChatBox::captureThisKey(EShortcut key)
{
	return !inputBox->getText().empty() && key == EShortcut::GLOBAL_ACCEPT;
}

void CChatBox::keyPressed(EShortcut key)
{
	if(key == EShortcut::GLOBAL_ACCEPT && inputBox->getText().size())
	{
		GAME->server().sendMessage(inputBox->getText());
		inputBox->setText("");
	}
	else
		inputBox->keyPressed(key);
}

void CChatBox::addNewMessage(const std::string & text)
{
	ENGINE->sound().playSound(AudioPath::builtin("CHAT"));
	chatHistory->setText(chatHistory->label->getText() + text + "\n");
	if(chatHistory->slider)
		chatHistory->slider->scrollToMax();
}

PvPBox::PvPBox(const Rect & rect)
{
	OBJECT_CONSTRUCTION;
	pos += rect.topLeft();
	setRedrawParent(true);

	backgroundTexture = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, rect.w, rect.h));
	backgroundTexture->setPlayerColor(PlayerColor(1));
	backgroundBorder = std::make_shared<TransparentFilledRectangle>(Rect(0, 0, rect.w, rect.h), ColorRGBA(0, 0, 0, 64), ColorRGBA(96, 96, 96, 255), 1);

	townSelector = std::make_shared<TownSelector>(Point(5, 3));

	auto getBannedTowns = [this](){
		std::vector<FactionID> bannedTowns;
		for(const auto & town : townSelector->townsEnabled)
			if(!town.second)
				bannedTowns.push_back(town.first);
		return bannedTowns;
	};

	buttonFlipCoin = std::make_shared<CButton>(Point(190, 6), AnimationPath::builtin("GSPBUT2.DEF"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.pvp.coin.help")), [](){
		LobbyPvPAction lpa;
		lpa.action = LobbyPvPAction::COIN;
		GAME->server().sendLobbyPack(lpa);
	}, EShortcut::LOBBY_FLIP_COIN);
	buttonFlipCoin->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.pvp.coin.hover"), EFonts::FONT_SMALL, Colors::WHITE);

	buttonRandomTown = std::make_shared<CButton>(Point(190, 31), AnimationPath::builtin("GSPBUT2.DEF"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.pvp.randomTown.help")), [getBannedTowns](){
		LobbyPvPAction lpa;
		lpa.action = LobbyPvPAction::RANDOM_TOWN;
		lpa.bannedTowns = getBannedTowns();
		GAME->server().sendLobbyPack(lpa);
	}, EShortcut::LOBBY_RANDOM_TOWN);
	buttonRandomTown->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.pvp.randomTown.hover"), EFonts::FONT_SMALL, Colors::WHITE);

	buttonRandomTownVs = std::make_shared<CButton>(Point(190, 56), AnimationPath::builtin("GSPBUT2.DEF"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.pvp.randomTownVs.help")), [getBannedTowns](){
		LobbyPvPAction lpa;
		lpa.action = LobbyPvPAction::RANDOM_TOWN_VS;
		lpa.bannedTowns = getBannedTowns();
		GAME->server().sendLobbyPack(lpa);
	}, EShortcut::LOBBY_RANDOM_TOWN_VS);
	buttonRandomTownVs->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.pvp.randomTownVs.hover"), EFonts::FONT_SMALL, Colors::WHITE);

	buttonHandicap = std::make_shared<CButton>(Point(190, 81), AnimationPath::builtin("GSPBUT2.DEF"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.handicap")), [](){
		if(!GAME->server().isHost())
			return;
		ENGINE->windows().createAndPushWindow<OptionsTab::HandicapWindow>();
	}, EShortcut::LOBBY_HANDICAP);
	buttonHandicap->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.handicap"), EFonts::FONT_SMALL, Colors::WHITE);
}

TownSelector::TownSelector(const Point & loc)
{
	OBJECT_CONSTRUCTION;
	pos += loc;
	setRedrawParent(true);

	int count = 0;
	for(auto const & factionID : LIBRARY->townh->getDefaultAllowed())
	{
		townsEnabled[factionID] = true;
		count++;
	}

	auto divisionRoundUp = [](int x, int y){ return (x + (y - 1)) / y; };

	if(count > 9)
	{
		slider = std::make_shared<CSlider>(Point(144, 0), 96, std::bind(&TownSelector::sliderMove, this, _1), 3, divisionRoundUp(count, 3), 0, Orientation::VERTICAL, CSlider::BLUE);
		slider->setPanningStep(24);
		slider->setScrollBounds(Rect(-144, 0, slider->pos.x - pos.x + slider->pos.w, slider->pos.h));
	}

	updateListItems();
}

void TownSelector::updateListItems()
{
	OBJECT_CONSTRUCTION;
	int line = slider ? slider->getValue() : 0;
	int x_offset = slider ? 0 : 8;
	
	towns.clear();
	townsArea.clear();

	int x = 0;
	int y = 0;
	for (auto const & factionID : LIBRARY->townh->getDefaultAllowed())
	{
		if(y >= line && (y - line) < 3)
		{
			auto getImageIndex = [](FactionID targetFactionID, bool enabled){ return targetFactionID.toFaction()->town->clientInfo.icons[true][!enabled] + 2; };
			towns[factionID] = std::make_shared<CAnimImage>(AnimationPath::builtin("ITPA"), getImageIndex(factionID, townsEnabled[factionID]), 0, x_offset + 48 * x, 32 * (y - line));
			townsArea[factionID] = std::make_shared<LRClickableArea>(Rect(x_offset + 48 * x, 32 * (y - line), 48, 32), [this, getImageIndex, factionID](){
				townsEnabled[factionID] = !townsEnabled[factionID];
				towns[factionID]->setFrame(getImageIndex(factionID, townsEnabled[factionID]));
				redraw();
			}, [factionID](){ CRClickPopup::createAndPush(factionID.toFaction()->town->faction->getNameTranslated()); });
		}

		if (x < 2)
			x++;
		else
		{
			x = 0;
			y++;
		}
	}
}

void TownSelector::sliderMove(int slidPos)
{
	if(!slider)
		return; // ignore spurious call when slider is being created
	updateListItems();
	redraw();
}

CFlagBox::CFlagBox(const Rect & rect)
	: CIntObject(SHOW_POPUP)
{
	pos += rect.topLeft();
	pos.w = rect.w;
	pos.h = rect.h;
	OBJECT_CONSTRUCTION;

	labelAllies = std::make_shared<CLabel>(0, 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[390] + ":");
	labelEnemies = std::make_shared<CLabel>(133, 0, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->allTexts[391] + ":");
}

void CFlagBox::recreate()
{
	flagsAllies.clear();
	flagsEnemies.clear();
	OBJECT_CONSTRUCTION;
	const int alliesX = 5 + (int)labelAllies->getWidth();
	const int enemiesX = 5 + 133 + (int)labelEnemies->getWidth();
	for(auto i = GAME->server().si->playerInfos.cbegin(); i != GAME->server().si->playerInfos.cend(); i++)
	{
		auto flag = std::make_shared<CAnimImage>(AnimationPath::builtin("ITGFLAGS.DEF"), i->first.getNum(), 0);
		if(i->first == GAME->server().myFirstColor() || GAME->server().getPlayerTeamId(i->first) == GAME->server().getPlayerTeamId(GAME->server().myFirstColor()))
		{
			flag->moveTo(Point(pos.x + alliesX + (int)flagsAllies.size()*flag->pos.w, pos.y));
			flagsAllies.push_back(flag);
		}
		else
		{
			flag->moveTo(Point(pos.x + enemiesX + (int)flagsEnemies.size()*flag->pos.w, pos.y));
			flagsEnemies.push_back(flag);
		}
	}
}

void CFlagBox::showPopupWindow(const Point & cursorPosition)
{
	if(SEL->getMapInfo())
		ENGINE->windows().createAndPushWindow<CFlagBoxTooltipBox>();
}

CFlagBox::CFlagBoxTooltipBox::CFlagBoxTooltipBox()
	: CWindowObject(BORDERED | RCLICK_POPUP | SHADOW_DISABLED, ImagePath::builtin("DIBOXBCK"))
{
	OBJECT_CONSTRUCTION;

	labelTeamAlignment = std::make_shared<CLabel>(128, 30, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->allTexts[657]);
	labelGroupTeams = std::make_shared<CLabelGroup>(FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);

	std::vector<std::set<PlayerColor>> teams(PlayerColor::PLAYER_LIMIT_I);

	for(PlayerColor j(0); j < PlayerColor::PLAYER_LIMIT; j++)
	{
		if(SEL->getPlayerInfo(j).canHumanPlay || SEL->getPlayerInfo(j).canComputerPlay)
		{
			teams[SEL->getPlayerInfo(j).team].insert(j);
		}
	}

	auto curIdx = 0;
	for(const auto & team : teams)
	{
		if(team.empty())
			continue;

		labelGroupTeams->add(128, 65 + 50 * curIdx, boost::str(boost::format(LIBRARY->generaltexth->allTexts[656]) % (curIdx + 1)));
		int curx = 128 - 9 * team.size();
		for(const auto & player : team)
		{
			iconsFlags.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("ITGFLAGS.DEF"), player, 0, curx, 75 + 50 * curIdx));
			curx += 18;
		}
		++curIdx;
	}
	pos.w = 256;
	pos.h = 90 + 50 * curIdx;

	background->scaleTo(Point(pos.w, pos.h));
	center();
}
