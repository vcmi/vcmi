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

#include "../CServerHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"
#include "../render/Colors.h"
#include "../globalLobby/GlobalLobbyClient.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../CGameInfo.h"

CLobbyScreen::CLobbyScreen(ESelectionScreen screenType)
	: CSelectionBase(screenType), bonusSel(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	tabSel = std::make_shared<SelectionTab>(screenType);
	curTab = tabSel;

	auto initLobby = [&]()
	{
		tabSel->callOnSelect = std::bind(&IServerAPI::setMapInfo, CSH, _1, nullptr);

		buttonSelect = std::make_shared<CButton>(Point(411, 80), AnimationPath::builtin("GSPBUTT.DEF"), CGI->generaltexth->zelp[45], 0, EShortcut::LOBBY_SELECT_SCENARIO);
		buttonSelect->addCallback([&]()
		{
			toggleTab(tabSel);
			CSH->setMapInfo(tabSel->getSelectedMapInfo());
		});

		buttonOptions = std::make_shared<CButton>(Point(411, 510), AnimationPath::builtin("GSPBUTT.DEF"), CGI->generaltexth->zelp[46], std::bind(&CLobbyScreen::toggleTab, this, tabOpt), EShortcut::LOBBY_ADDITIONAL_OPTIONS);
		if(settings["general"]["enableUiEnhancements"].Bool())
		{
			buttonTurnOptions = std::make_shared<CButton>(Point(619, 105), AnimationPath::builtin("GSPBUT2.DEF"), CGI->generaltexth->zelp[46], std::bind(&CLobbyScreen::toggleTab, this, tabTurnOptions), EShortcut::NONE);
			buttonExtraOptions = std::make_shared<CButton>(Point(619, 510), AnimationPath::builtin("GSPBUT2.DEF"), CGI->generaltexth->zelp[46], std::bind(&CLobbyScreen::toggleTab, this, tabExtraOptions), EShortcut::NONE);
		}
	};

	buttonChat = std::make_shared<CButton>(Point(619, 80), AnimationPath::builtin("GSPBUT2.DEF"), CGI->generaltexth->zelp[48], std::bind(&CLobbyScreen::toggleChat, this), EShortcut::LOBBY_HIDE_CHAT);
	buttonChat->setTextOverlay(CGI->generaltexth->allTexts[532], FONT_SMALL, Colors::WHITE);

	switch(screenType)
	{
	case ESelectionScreen::newGame:
	{
		tabOpt = std::make_shared<OptionsTab>();
		tabTurnOptions = std::make_shared<TurnOptionsTab>();
		tabExtraOptions = std::make_shared<ExtraOptionsTab>();
		tabRand = std::make_shared<RandomMapTab>();
		tabRand->mapInfoChanged += std::bind(&IServerAPI::setMapInfo, CSH, _1, _2);
		buttonRMG = std::make_shared<CButton>(Point(411, 105), AnimationPath::builtin("GSPBUTT.DEF"), CGI->generaltexth->zelp[47], 0, EShortcut::LOBBY_RANDOM_MAP);
		buttonRMG->addCallback([&]()
		{
			toggleTab(tabRand);
			tabRand->updateMapInfoByHost(); // TODO: This is only needed to force-update mapInfo in CSH when tab is opened
		});

		card->iconDifficulty->addCallback(std::bind(&IServerAPI::setDifficulty, CSH, _1));

		buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRBEG.DEF"), CGI->generaltexth->zelp[103], std::bind(&CLobbyScreen::startScenario, this, false), EShortcut::LOBBY_BEGIN_GAME);
		initLobby();
		break;
	}
	case ESelectionScreen::loadGame:
	{
		tabOpt = std::make_shared<OptionsTab>();
		tabTurnOptions = std::make_shared<TurnOptionsTab>();
		tabExtraOptions = std::make_shared<ExtraOptionsTab>();
		buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRLOD.DEF"), CGI->generaltexth->zelp[103], std::bind(&CLobbyScreen::startScenario, this, false), EShortcut::LOBBY_LOAD_GAME);
		initLobby();
		break;
	}
	case ESelectionScreen::campaignList:
		tabSel->callOnSelect = std::bind(&IServerAPI::setMapInfo, CSH, _1, nullptr);
		buttonStart = std::make_shared<CButton>(Point(411, 535), AnimationPath::builtin("SCNRLOD.DEF"), CButton::tooltip(), std::bind(&CLobbyScreen::startCampaign, this), EShortcut::LOBBY_BEGIN_GAME);
		break;
	}

	buttonStart->block(true); // to be unblocked after map list is ready

	buttonBack = std::make_shared<CButton>(Point(581, 535), AnimationPath::builtin("SCNRBACK.DEF"), CGI->generaltexth->zelp[105], [&]()
	{
		bool wasInLobbyRoom = CSH->inLobbyRoom();
		CSH->sendClientDisconnecting();
		close();

		if (wasInLobbyRoom)
			CSH->getGlobalLobby().activateInterface();
	}, EShortcut::GLOBAL_CANCEL);
}

CLobbyScreen::~CLobbyScreen()
{
	// TODO: For now we always destroy whole lobby when leaving bonus selection screen
	if(CSH->getState() == EClientState::LOBBY_CAMPAIGN)
		CSH->sendClientDisconnecting();
}

void CLobbyScreen::toggleTab(std::shared_ptr<CIntObject> tab)
{
	if(tab == curTab)
		CSH->sendGuiAction(LobbyGuiAction::NO_TAB);
	else if(tab == tabOpt)
		CSH->sendGuiAction(LobbyGuiAction::OPEN_OPTIONS);
	else if(tab == tabSel)
		CSH->sendGuiAction(LobbyGuiAction::OPEN_SCENARIO_LIST);
	else if(tab == tabRand)
		CSH->sendGuiAction(LobbyGuiAction::OPEN_RANDOM_MAP_OPTIONS);
	else if(tab == tabTurnOptions)
		CSH->sendGuiAction(LobbyGuiAction::OPEN_TURN_OPTIONS);
	else if(tab == tabExtraOptions)
		CSH->sendGuiAction(LobbyGuiAction::OPEN_EXTRA_OPTIONS);
	CSelectionBase::toggleTab(tab);
}

void CLobbyScreen::startCampaign()
{
	if(!CSH->mi)
		return;

	try {
		auto ourCampaign = CampaignHandler::getCampaign(CSH->mi->fileURI);
		CSH->setCampaignState(ourCampaign);
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
	if (tabRand && CSH->si->mapGenOptions)
	{
		// Save RMG settings at game start
		tabRand->saveOptions(*CSH->si->mapGenOptions);
	}

	// Save chosen difficulty
	Settings lastDifficulty = settings.write["general"]["lastDifficulty"];
	lastDifficulty->Integer() = getCurrentDifficulty();

	if (CSH->validateGameStart(allowOnlyAI))
	{
		CSH->sendStartGame(allowOnlyAI);
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
	buttonSelect->setTextOverlay(CGI->generaltexth->allTexts[500], FONT_SMALL, buttonColor);
	buttonOptions->setTextOverlay(CGI->generaltexth->allTexts[501], FONT_SMALL, buttonColor);

	if (buttonTurnOptions)
		buttonTurnOptions->setTextOverlay(CGI->generaltexth->translate("vcmi.optionsTab.turnOptions.hover"), FONT_SMALL, buttonColor);

	if (buttonExtraOptions)
		buttonExtraOptions->setTextOverlay(CGI->generaltexth->translate("vcmi.optionsTab.extraOptions.hover"), FONT_SMALL, buttonColor);

	if(buttonRMG)
	{
		buttonRMG->setTextOverlay(CGI->generaltexth->allTexts[740], FONT_SMALL, buttonColor);
		buttonRMG->block(!host);
	}
	buttonSelect->block(!host);
	buttonOptions->block(!host);

	if (buttonTurnOptions)
		buttonTurnOptions->block(!host);

	if (buttonExtraOptions)
		buttonExtraOptions->block(!host);

	if(CSH->mi)
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
		buttonChat->setTextOverlay(CGI->generaltexth->allTexts[531], FONT_SMALL, Colors::WHITE);
	else
		buttonChat->setTextOverlay(CGI->generaltexth->allTexts[532], FONT_SMALL, Colors::WHITE);
}

void CLobbyScreen::updateAfterStateChange()
{
	if(CSH->mi)
	{
		if (tabOpt)
			tabOpt->recreate();
		if (tabTurnOptions)
			tabTurnOptions->recreate();
		if (tabExtraOptions)
			tabExtraOptions->recreate();
	}

	buttonStart->block(CSH->mi == nullptr || CSH->isGuest());

	card->changeSelection();
	if (card->iconDifficulty)
	{
		if (screenType == ESelectionScreen::loadGame)
		{
			// When loading the game, only one button in the difficulty toggle group should be enabled, so here disable all other buttons first, then make selection
			card->iconDifficulty->setSelectedOnly(CSH->si->difficulty);
		}
		else
		{
			card->iconDifficulty->setSelected(CSH->si->difficulty);
		}
	}
	
	if(curTab && curTab == tabRand && CSH->si->mapGenOptions)
		tabRand->setMapGenOptions(CSH->si->mapGenOptions);
}

const StartInfo * CLobbyScreen::getStartInfo()
{
	return CSH->si.get();
}

const CMapInfo * CLobbyScreen::getMapInfo()
{
	return CSH->mi.get();
}
