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
#include "SelectionTab.h"
#include "RandomMapTab.h"
#include "OptionsTab.h"
#include "../CServerHandler.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"

#include "../../CCallback.h"

#include "../CGameInfo.h"
#include "../../lib/CModHandler.h"
#include "../../lib/NetPacksLobby.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CCampaignHandler.h"
#include "../../lib/rmg/CMapGenOptions.h"

CLobbyScreen::CLobbyScreen(ESelectionScreen screenType)
	: CSelectionBase(screenType), bonusSel(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	tabSel = std::make_shared<SelectionTab>(screenType);
	curTab = tabSel;

	auto initLobby = [&]()
	{
		tabSel->callOnSelect = std::bind(&IServerAPI::setMapInfo, CSH, _1, nullptr);

		buttonSelect = std::make_shared<CButton>(Point(411, 80), "GSPBUTT.DEF", CGI->generaltexth->zelp[45], 0, EShortcut::LOBBY_SELECT_SCENARIO);
		buttonSelect->addCallback([&]()
		{
			toggleTab(tabSel);
			CSH->setMapInfo(tabSel->getSelectedMapInfo());
		});

		buttonOptions = std::make_shared<CButton>(Point(411, 510), "GSPBUTT.DEF", CGI->generaltexth->zelp[46], std::bind(&CLobbyScreen::toggleTab, this, tabOpt), EShortcut::LOBBY_ADDITIONAL_OPTIONS);
	};

	buttonChat = std::make_shared<CButton>(Point(619, 80), "GSPBUT2.DEF", CGI->generaltexth->zelp[48], std::bind(&CLobbyScreen::toggleChat, this), EShortcut::LOBBY_HIDE_CHAT);
	buttonChat->addTextOverlay(CGI->generaltexth->allTexts[532], FONT_SMALL);

	switch(screenType)
	{
	case ESelectionScreen::newGame:
	{
		tabOpt = std::make_shared<OptionsTab>();
		tabRand = std::make_shared<RandomMapTab>();
		tabRand->mapInfoChanged += std::bind(&IServerAPI::setMapInfo, CSH, _1, _2);
		buttonRMG = std::make_shared<CButton>(Point(411, 105), "GSPBUTT.DEF", CGI->generaltexth->zelp[47], 0, EShortcut::LOBBY_RANDOM_MAP);
		buttonRMG->addCallback([&]()
		{
			toggleTab(tabRand);
			tabRand->updateMapInfoByHost(); // TODO: This is only needed to force-update mapInfo in CSH when tab is opened
		});

		card->iconDifficulty->addCallback(std::bind(&IServerAPI::setDifficulty, CSH, _1));

		buttonStart = std::make_shared<CButton>(Point(411, 535), "SCNRBEG.DEF", CGI->generaltexth->zelp[103], std::bind(&CLobbyScreen::startScenario, this, true), EShortcut::LOBBY_BEGIN_GAME);
		initLobby();
		break;
	}
	case ESelectionScreen::loadGame:
	{
		tabOpt = std::make_shared<OptionsTab>();
		buttonStart = std::make_shared<CButton>(Point(411, 535), "SCNRLOD.DEF", CGI->generaltexth->zelp[103], std::bind(&CLobbyScreen::startScenario, this, true), EShortcut::LOBBY_LOAD_GAME);
		initLobby();
		break;
	}
	case ESelectionScreen::campaignList:
		tabSel->callOnSelect = std::bind(&IServerAPI::setMapInfo, CSH, _1, nullptr);
		buttonStart = std::make_shared<CButton>(Point(411, 535), "SCNRLOD.DEF", CButton::tooltip(), std::bind(&CLobbyScreen::startCampaign, this), EShortcut::LOBBY_BEGIN_GAME);
		break;
	}

	buttonBack = std::make_shared<CButton>(Point(581, 535), "SCNRBACK.DEF", CGI->generaltexth->zelp[105], [&]()
	{
		CSH->sendClientDisconnecting();
		close();
	}, EShortcut::GLOBAL_CANCEL);
}

CLobbyScreen::~CLobbyScreen()
{
	// TODO: For now we always destroy whole lobby when leaving bonus selection screen
	if(CSH->state == EClientState::LOBBY_CAMPAIGN)
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
	CSelectionBase::toggleTab(tab);
}

void CLobbyScreen::startCampaign()
{
	if(CSH->mi)
	{
		auto ourCampaign = std::make_shared<CCampaignState>(CCampaignHandler::getCampaign(CSH->mi->fileURI));
		CSH->setCampaignState(ourCampaign);
	}
}

void CLobbyScreen::startScenario(bool allowOnlyAI)
{
	try
	{
		CSH->sendStartGame(allowOnlyAI);
		buttonStart->block(true);
	}
	catch(CModHandler::Incompatibility & e)
	{
		logGlobal->warn("Incompatibility exception during start scenario: %s", e.what());
		
		auto errorMsg = CGI->generaltexth->translate("vcmi.server.errors.modsIncompatibility") + '\n';
		errorMsg += e.what();
		
		CInfoWindow::showInfoDialog(errorMsg, CInfoWindow::TCompsInfo(), PlayerColor(1));
	}
	catch(std::exception & e)
	{
		logGlobal->error("Exception during startScenario: %s", e.what());
		
		if(std::string(e.what()) == "ExceptionNoHuman")
			CInfoWindow::showInfoDialog(CGI->generaltexth->allTexts[530], CInfoWindow::TCompsInfo(), PlayerColor(1));
		
		if(std::string(e.what()) == "ExceptionNoTemplate")
			CInfoWindow::showInfoDialog(CGI->generaltexth->allTexts[751], CInfoWindow::TCompsInfo(), PlayerColor(1));
	}
	catch(...)
	{
		logGlobal->error("Unknown exception");
	}
}

void CLobbyScreen::toggleMode(bool host)
{
	tabSel->toggleMode();
	buttonStart->block(!host);
	if(screenType == ESelectionScreen::campaignList)
		return;

	auto buttonColor = host ? Colors::WHITE : Colors::ORANGE;
	buttonSelect->addTextOverlay(CGI->generaltexth->allTexts[500], FONT_SMALL, buttonColor);
	buttonOptions->addTextOverlay(CGI->generaltexth->allTexts[501], FONT_SMALL, buttonColor);
	if(buttonRMG)
	{
		buttonRMG->addTextOverlay(CGI->generaltexth->allTexts[740], FONT_SMALL, buttonColor);
		buttonRMG->block(!host);
	}
	buttonSelect->block(!host);
	buttonOptions->block(!host);

	if(CSH->mi)
		tabOpt->recreate();
}

void CLobbyScreen::toggleChat()
{
	card->toggleChat();
	if(card->showChat)
		buttonChat->addTextOverlay(CGI->generaltexth->allTexts[531], FONT_SMALL);
	else
		buttonChat->addTextOverlay(CGI->generaltexth->allTexts[532], FONT_SMALL);
}

void CLobbyScreen::updateAfterStateChange()
{
	if(CSH->mi && tabOpt)
		tabOpt->recreate();

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
