/*
 * CLobbyScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CLobbyScreen.h"

#include "CBonusSelection.h"
#include "TurnOptionsTab.h"
#include "ExtraOptionsTab.h"
#include "OptionsTab.h"
#include "RandomMapTab.h"
#include "SelectionTab.h"
#include "BattleOnlyModeTab.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../windows/InfoWindows.h"
#include "../render/Colors.h"
#include "../globalLobby/GlobalLobbyClient.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/GameLibrary.h"

CLobbyScreen::CLobbyScreen(ESelectionScreen screenType, bool hideScreen)
	: CSelectionBase(screenType), bonusSel(nullptr)
{
	OBJECT_CONSTRUCTION;
	tabSel = std::make_shared<SelectionTab>(screenType);
	curTab = tabSel;

	auto initLobby = [&]()
	{
		tabSel->callOnSelect = std::bind(&IServerAPI::setMapInfo, &GAME->server(), _1, nullptr);

		buttonSelect = std::make_shared<CButton>(Point(411, 80), AnimationPath::builtin("GSPBUTT.DEF"), LIBRARY->generaltexth->zelp[45], 0, EShortcut::LOBBY_SELECT_SCENARIO);
		buttonSelect->addCallback([this]()
		{
			toggleTab(tabSel);
			if (getMapInfo() && getMapInfo()->isRandomMap)
				GAME->server().setMapInfo(tabSel->getSelectedMapInfo());
		});

		buttonOptions = std::make_shared<CButton>(Point(411, 510), AnimationPath::builtin("GSPBUTT.DEF"), LIBRARY->generaltexth->zelp[46], std::bind(&CLobbyScreen::toggleTab, this, tabOpt), EShortcut::LOBBY_ADDITIONAL_OPTIONS);
		if(settings["general"]["enableUiEnhancements"].Bool())
		{
			if(screenType == ESelectionScreen::newGame)
				buttonBattleMode = std::make_shared<CButton>(Point(619, 105), AnimationPath::builtin("GSPButton2Arrow"), CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyMode.help")), [this](){
					updateAfterStateChange(); // creates tabBattleOnlyMode -> cannot created by init of object because GAME->server().isGuest() isn't valid at that point
					toggleTab(tabBattleOnlyMode);
				}, EShortcut::LOBBY_BATTLE_MODE);
			buttonExtraOptions = std::make_shared<CButton>(Point(619, 510), AnimationPath::builtin("GSPButton2Arrow"), LIBRARY->generaltexth->zelp[46], std::bind(&CLobbyScreen::toggleTab, this, tabExtraOptions), EShortcut::LOBBY_EXTRA_OPTIONS);
		}
	};

	if(screenType != ESelectionScreen::campaignList)
	{
		buttonChat = std::make_shared<CButton>(Point(619, 80), AnimationPath::builtin("GSPBUT2.DEF"), LIBRARY->generaltexth->zelp[48], std::bind(&CLobbyScreen::toggleChat, this), EShortcut::LOBBY_TOGGLE_CHAT);
		buttonChat->setTextOverlay(LIBRARY->generaltexth->allTexts[532], FONT_SMALL, Colors::WHITE);
	}

	switch(screenType)
	{
	case ESelectionScreen::newGame:
	{
		tabOpt = std::make_shared<OptionsTab>();
		tabTurnOptions = std::make_shared<TurnOptionsTab>();
		tabExtraOptions = std::make_shared<ExtraOptionsTab>();
		tabRand = std::make_shared<RandomMapTab>();
		tabRand->mapInfoChanged += std::bind(&IServerAPI::setMapInfo, &GAME->server(), _1, _2);
		buttonRMG = std::make_shared<CButton>(Point(411, 105), AnimationPath::builtin("GSPBUTT.DEF"), LIBRARY->generaltexth->zelp[47], 0, EShortcut::LOBBY_RANDOM_MAP);
		buttonRMG->addCallback([this]()
		{
			toggleTab(tabRand);
			if (getMapInfo() && !getMapInfo()->isRandomMap)
				tabRand->updateMapInfoByHost();
		});

		card->iconDifficulty->addCallback(std::bind(&IServerAPI::setDifficulty, &GAME->server(), _1));

		buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRBEG.DEF"), LIBRARY->generaltexth->zelp[103], std::bind(&CLobbyScreen::start, this, false), EShortcut::LOBBY_BEGIN_STANDARD_GAME);
		initLobby();
		break;
	}
	case ESelectionScreen::loadGame:
	{
		tabOpt = std::make_shared<OptionsTab>();
		tabTurnOptions = std::make_shared<TurnOptionsTab>();
		tabExtraOptions = std::make_shared<ExtraOptionsTab>();
		buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRLOD.DEF"), LIBRARY->generaltexth->zelp[103], std::bind(&CLobbyScreen::start, this, false), EShortcut::LOBBY_LOAD_GAME);
		initLobby();
		break;
	}
	case ESelectionScreen::campaignList:
		tabSel->callOnSelect = std::bind(&IServerAPI::setMapInfo, &GAME->server(), _1, nullptr);
		buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRLOD.DEF"), CButton::tooltip(), std::bind(&CLobbyScreen::start, this, true), EShortcut::LOBBY_BEGIN_CAMPAIGN);
		break;
	}

	buttonStart->block(true); // to be unblocked after map list is ready

	buttonBack = std::make_shared<CButton>(Point(581, 535), AnimationPath::builtin("SCNRBACK.DEF"), LIBRARY->generaltexth->zelp[105], [&]()
	{
		bool wasInLobbyRoom = GAME->server().inLobbyRoom();
		GAME->server().sendClientDisconnecting();
		close();

		if (wasInLobbyRoom)
			GAME->server().getGlobalLobby().activateInterface();
	}, EShortcut::GLOBAL_CANCEL);

	if(hideScreen) // workaround to avoid confusing players by custom campaign list displaying for a few ms -> instead of this draw a black screen while "loading"
	{
		blackScreen = std::make_shared<GraphicalPrimitiveCanvas>(Rect(Point(0, 0), pos.dimensions()));
		blackScreen->addBox(Point(0, 0), pos.dimensions(), Colors::BLACK);
	}
}

CLobbyScreen::~CLobbyScreen()
{
	// TODO: For now we always destroy whole lobby when leaving bonus selection screen
	if(GAME->server().getState() == EClientState::LOBBY_CAMPAIGN)
		GAME->server().sendClientDisconnecting();
}

void CLobbyScreen::toggleTab(std::shared_ptr<CIntObject> tab)
{
	if(tab == curTab)
		GAME->server().sendGuiAction(LobbyGuiAction::NO_TAB);
	else if(tab == tabOpt)
		GAME->server().sendGuiAction(LobbyGuiAction::OPEN_OPTIONS);
	else if(tab == tabSel)
		GAME->server().sendGuiAction(LobbyGuiAction::OPEN_SCENARIO_LIST);
	else if(tab == tabRand)
		GAME->server().sendGuiAction(LobbyGuiAction::OPEN_RANDOM_MAP_OPTIONS);
	else if(tab == tabTurnOptions)
		GAME->server().sendGuiAction(LobbyGuiAction::OPEN_TURN_OPTIONS);
	else if(tab == tabExtraOptions)
		GAME->server().sendGuiAction(LobbyGuiAction::OPEN_EXTRA_OPTIONS);
	else if(tab == tabBattleOnlyMode)
		GAME->server().sendGuiAction(LobbyGuiAction::BATTLE_MODE);

	if(tab == tabBattleOnlyMode)
	{
		tabBattleOnlyMode->setStartButtonEnabled();
		card->clearSelection();
	}
	else
	{
		buttonStart->block(GAME->server().mi == nullptr || GAME->server().isGuest());
		card->changeSelection();
	}

	CSelectionBase::toggleTab(tab);
}

void CLobbyScreen::start(bool campaign)
{
	if(curTab == tabBattleOnlyMode)
		tabBattleOnlyMode->startBattle();
	else if(campaign)
		startCampaign();
	else
		startScenario(false);
}

void CLobbyScreen::startCampaign()
{
	if(!GAME->server().mi)
		return;

	try {
		auto ourCampaign = CampaignHandler::getCampaign(GAME->server().mi->fileURI);
		GAME->server().setCampaignState(ourCampaign);
	}
	catch (const std::runtime_error &e)
	{
		// handle possible exception on map loading. For example campaign that contains map in unsupported format
		// for example, wog campaigns or hota campaigns without hota map support mod
		MetaString message;
		message.appendTextID("vcmi.client.errors.invalidMap");
		message.replaceRawString(e.what());

		CInfoWindow::showInfoDialog(message.toString(), {});
	}
}

void CLobbyScreen::startScenario(bool allowOnlyAI)
{
	if (tabRand && GAME->server().si->mapGenOptions)
	{
		// Save RMG settings at game start
		tabRand->saveOptions(*GAME->server().si->mapGenOptions);
	}

	// Save chosen difficulty
	Settings lastDifficulty = settings.write["general"]["lastDifficulty"];
	lastDifficulty->Integer() = getCurrentDifficulty();

	if (GAME->server().validateGameStart(allowOnlyAI))
	{
		GAME->server().sendStartGame(allowOnlyAI);
		buttonStart->block(true);
	}
}

void CLobbyScreen::toggleMode(bool host)
{
	tabSel->toggleMode();
	buttonStart->block(!host);
	if(screenType == ESelectionScreen::campaignList)
		return;

	auto buttonColor = host ? Colors::WHITE : Colors::ORANGE;
	buttonSelect->setTextOverlay("  " + LIBRARY->generaltexth->allTexts[500], FONT_SMALL, buttonColor);
	buttonOptions->setTextOverlay(LIBRARY->generaltexth->allTexts[501], FONT_SMALL, buttonColor);

	if (buttonBattleMode)
		buttonBattleMode->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyMode"), FONT_SMALL, buttonColor);

	if (buttonExtraOptions)
		buttonExtraOptions->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.optionsTab.extraOptions.hover"), FONT_SMALL, buttonColor);

	if(buttonRMG)
	{
		buttonRMG->setTextOverlay("  " + LIBRARY->generaltexth->allTexts[740], FONT_SMALL, buttonColor);
		buttonRMG->block(!host);
	}
	buttonSelect->block(!host);
	buttonOptions->block(!host);

	if (buttonBattleMode)
		buttonBattleMode->block(!host);

	if (buttonExtraOptions)
		buttonExtraOptions->block(!host);

	if(GAME->server().mi)
	{
		tabOpt->recreate();
		tabTurnOptions->recreate();
		tabExtraOptions->recreate();
	}
}

void CLobbyScreen::toggleChat()
{
	card->toggleChat();
	if(card->showChat)
		buttonChat->setTextOverlay(LIBRARY->generaltexth->allTexts[531], FONT_SMALL, Colors::WHITE);
	else
		buttonChat->setTextOverlay(LIBRARY->generaltexth->allTexts[532], FONT_SMALL, Colors::WHITE);
}

void CLobbyScreen::updateAfterStateChange()
{
	OBJECT_CONSTRUCTION;
	if(!tabBattleOnlyMode)
	{
		tabBattleOnlyMode = std::make_shared<BattleOnlyModeTab>();
		tabBattleOnlyMode->setEnabled(false);

		if(GAME->server().battleMode)
			toggleTab(tabBattleOnlyMode);
	}

	if(GAME->server().isHost() && screenType == ESelectionScreen::newGame)
	{
		bool isMultiplayer = GAME->server().loadMode == ELoadMode::MULTI;
		ExtraOptionsInfo info = SEL->getStartInfo()->extraOptionsInfo;
		info.cheatsAllowed = isMultiplayer ? persistentStorage["startExtraOptions"]["multiPlayer"]["cheatsAllowed"].Bool() : !persistentStorage["startExtraOptions"]["singlePlayer"]["cheatsNotAllowed"].Bool();
		info.unlimitedReplay = persistentStorage["startExtraOptions"][isMultiplayer ? "multiPlayer" : "singlePlayer"]["unlimitedReplay"].Bool();
		if(info.cheatsAllowed != GAME->server().si->extraOptionsInfo.cheatsAllowed || info.unlimitedReplay != GAME->server().si->extraOptionsInfo.unlimitedReplay)
			GAME->server().setExtraOptionsInfo(info);
	}

	if(GAME->server().mi)
	{
		if (tabOpt)
			tabOpt->recreate();
		if (tabTurnOptions)
			tabTurnOptions->recreate();
		if (tabExtraOptions)
			tabExtraOptions->recreate();
	}

	if(curTab && curTab != tabBattleOnlyMode)
	{
		buttonStart->block(GAME->server().mi == nullptr || GAME->server().isGuest());
		card->changeSelection();
	}

	if (card->iconDifficulty)
	{
		if (screenType == ESelectionScreen::loadGame)
		{
			// When loading the game, only one button in the difficulty toggle group should be enabled, so here disable all other buttons first, then make selection
			card->iconDifficulty->setSelectedOnly(GAME->server().si->difficulty);
		}
		else
		{
			card->iconDifficulty->setSelected(GAME->server().si->difficulty);
		}
	}
	
	if(curTab && curTab == tabRand && GAME->server().si->mapGenOptions)
		tabRand->setMapGenOptions(GAME->server().si->mapGenOptions);
}

const StartInfo * CLobbyScreen::getStartInfo()
{
	return GAME->server().si.get();
}

const CMapInfo * CLobbyScreen::getMapInfo()
{
	return GAME->server().mi.get();
}
