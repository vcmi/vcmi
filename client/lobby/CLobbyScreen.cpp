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
#include "SelectionTab.h"
#include "RandomMapTab.h"
#include "OptionsTab.h"
#include "../CServerHandler.h"

#include "../gui/CGuiHandler.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"

#include "../../CCallback.h"
#include "../../lib/NetPacks.h"

#include "../CGameInfo.h"
#include "../../lib/CGeneralTextHandler.h"

// campaigns
#include "CBonusSelection.h"

CLobbyScreen::CLobbyScreen(ESelectionScreen screenType)
	: CSelectionBase(screenType), bonusSel(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	tabSel = new SelectionTab(screenType); //scenario selection tab
	tabSel->recActions = 255;
	curTab = tabSel;

	auto initLobby = [&]()
	{
		tabSel->onSelect = std::bind(&IServerAPI::setMapInfo, CSH, _1, nullptr);

		buttonSelect = new CButton(Point(411, 80), "GSPBUTT.DEF", CGI->generaltexth->zelp[45], 0, SDLK_s);
		buttonSelect->addCallback([&]()
		{
			toggleTab(tabSel);
			CSH->setMapInfo(tabSel->getSelectedMapInfo());
		});

		buttonOptions = new CButton(Point(411, 510), "GSPBUTT.DEF", CGI->generaltexth->zelp[46], std::bind(&CLobbyScreen::toggleTab, this, tabOpt), SDLK_a);
	};

	CButton * buttonChat = new CButton(Point(619, 83), "GSPBUT2.DEF", CGI->generaltexth->zelp[48], std::bind(&InfoCard::toggleChat, card), SDLK_h);
	buttonChat->addTextOverlay(CGI->generaltexth->allTexts[531], FONT_SMALL);

	switch(screenType)
	{
	case ESelectionScreen::newGame:
	{
		tabRand = new RandomMapTab();
		tabRand->mapInfoChanged += std::bind(&IServerAPI::setMapInfo, CSH, _1, _2);
		tabRand->recActions = DISPOSE;
		buttonRMG = new CButton(Point(411, 105), "GSPBUTT.DEF", CGI->generaltexth->zelp[47], 0, SDLK_r);
		buttonRMG->addCallback([&]()
		{
			toggleTab(tabRand);
			tabRand->updateMapInfoByHost(); // TODO: This is only needed to force-update mapInfo in CSH when tab is opened
		});

		card->difficulty->addCallback(std::bind(&IServerAPI::setDifficulty, CSH, _1));

		buttonStart = new CButton(Point(411, 535), "SCNRBEG.DEF", CGI->generaltexth->zelp[103], std::bind(&CLobbyScreen::startScenario, this), SDLK_b);
		initLobby();
		break;
	}
	case ESelectionScreen::loadGame:
	{
		buttonStart = new CButton(Point(411, 535), "SCNRLOD.DEF", CGI->generaltexth->zelp[103], std::bind(&CLobbyScreen::startScenario, this), SDLK_l);
		initLobby();
		break;
	}
	case ESelectionScreen::campaignList:
		tabSel->onSelect = std::bind(&IServerAPI::setMapInfo, CSH, _1, nullptr);
		buttonStart = new CButton(Point(411, 535), "SCNRLOD.DEF", CButton::tooltip(), std::bind(&CLobbyScreen::startCampaign, this), SDLK_b);
		break;
	}

	buttonStart->assignedKeys.insert(SDLK_RETURN);

	buttonBack = new CButton(Point(581, 535), "SCNRBACK.DEF", CGI->generaltexth->zelp[105], std::bind(&IServerAPI::sendClientDisconnecting, CSH), SDLK_ESCAPE);
}

CLobbyScreen::~CLobbyScreen()
{
	// TODO: For now we always destroy whole lobby when leaving bonus selection screen
	if(CSH->state == EClientState::LOBBY_CAMPAIGN)
		CSH->sendClientDisconnecting();
}

void CLobbyScreen::toggleTab(CIntObject * tab)
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

void CLobbyScreen::startScenario()
{
	try
	{
		CSH->sendStartGame();
		buttonStart->block(true);
	}
	catch(ExceptionMapMissing & e)
	{

	}
	catch(ExceptionNoHuman & e)
	{
		GH.pushInt(CInfoWindow::create(CGI->generaltexth->allTexts[530])); // You must position yourself prior to starting the game.
	}
	catch(ExceptionNoTemplate & e)
	{
		GH.pushInt(CInfoWindow::create(CGI->generaltexth->allTexts[751]));
	}
	catch(...)
	{

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

void CLobbyScreen::activate()
{
	CIntObject::activate();

	// MPTODO: campaigns screen startup hack
	if(CSH->campaignState && !CSH->campaignSent)
	{
		CSH->setCampaignState(CSH->campaignState);
		CSH->campaignState.reset();
		CSH->campaignSent = true;
	}
}

void CLobbyScreen::updateAfterStateChange()
{
	if(CSH->mi && tabOpt)
		tabOpt->recreate();

	card->changeSelection();
	if(card->difficulty)
		card->difficulty->setSelected(CSH->si->difficulty);

	if(curTab == tabRand && CSH->si->mapGenOptions)
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
