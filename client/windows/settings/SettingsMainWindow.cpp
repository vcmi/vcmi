/*
 * SettingsMainContainer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SettingsMainWindow.h"

#include "AdventureOptionsTab.h"
#include "BattleOptionsTab.h"
#include "GeneralOptionsTab.h"
#include "OtherOptionsTab.h"

#include "CMT.h"
#include "CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CPlayerInterface.h"
#include "CServerHandler.h"
#include "filesystem/ResourcePath.h"
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "render/Canvas.h"
#include "lobby/CSavingScreen.h"
#include "widgets/Buttons.h"
#include "widgets/Images.h"
#include "widgets/ObjectLists.h"
#include "windows/CMessage.h"

SettingsMainWindow::SettingsMainWindow(BattleInterface * parentBattleUi) : InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(JsonPath::builtin("config/widgets/settings/settingsMainContainer.json"));
	addCallback("activateSettingsTab", [this](int tabId) { openTab(tabId); });
	addCallback("loadGame", [this](int) { loadGameButtonCallback(); });
	addCallback("saveGame", [this](int) { saveGameButtonCallback(); });
	addCallback("restartGame", [this](int) { restartGameButtonCallback(); });
	addCallback("quitGame", [this](int) { quitGameButtonCallback(); });
	addCallback("returnToMainMenu", [this](int) { mainMenuButtonCallback(); });
	addCallback("closeWindow", [this](int) { backButtonCallback(); });
	build(config);

	std::shared_ptr<CIntObject> background = widget<CIntObject>("background");
	pos.w = background->pos.w;
	pos.h = background->pos.h;
	pos = center();

	std::shared_ptr<CButton> loadButton = widget<CButton>("loadButton");
	assert(loadButton);

	std::shared_ptr<CButton> saveButton = widget<CButton>("saveButton");
	assert(saveButton);

	std::shared_ptr<CButton> restartButton = widget<CButton>("restartButton");
	assert(restartButton);

	loadButton->block(CSH->isGuest());
	saveButton->block(CSH->isGuest() || parentBattleUi);
	restartButton->block(CSH->isGuest() || parentBattleUi);

	int defaultTabIndex = 0;
	if(parentBattleUi != nullptr)
		defaultTabIndex = 2;
	else if(settings["general"]["lastSettingsTab"].isNumber())
		defaultTabIndex = settings["general"]["lastSettingsTab"].Integer();

	parentBattleInterface = parentBattleUi;
	tabContentArea = std::make_shared<CTabbedInt>(std::bind(&SettingsMainWindow::createTab, this, _1), Point(0, 0), defaultTabIndex);
	tabContentArea->setRedrawParent(true);

	std::shared_ptr<CToggleGroup> mainTabs = widget<CToggleGroup>("settingsTabs");
	mainTabs->setSelected(defaultTabIndex);
}

std::shared_ptr<CIntObject> SettingsMainWindow::createTab(size_t index)
{
	switch(index)
	{
		case 0:
			return std::make_shared<GeneralOptionsTab>();
		case 1:
			return std::make_shared<AdventureOptionsTab>();
		case 2:
			return std::make_shared<BattleOptionsTab>(parentBattleInterface);
		case 3:
			return std::make_shared<OtherOptionsTab>();
		default:
			logGlobal->error("Wrong settings tab ID!");
			return std::make_shared<GeneralOptionsTab>();
	}
}

void SettingsMainWindow::openTab(size_t index)
{
	tabContentArea->setActive(index);
	CIntObject::redraw();

	Settings lastUsedTab = settings.write["general"]["lastSettingsTab"];
	lastUsedTab->Integer() = index;
}

void SettingsMainWindow::close()
{
	if(!GH.windows().isTopWindow(this))
		logGlobal->error("Only top interface must be closed");
	GH.windows().popWindows(1);
}

void SettingsMainWindow::quitGameButtonCallback()
{
	LOCPLINT->showYesNoDialog(
		CGI->generaltexth->allTexts[578],
		[this]()
		{
			close();
			GH.dispatchMainThread( []()
			{
				handleQuit(false);
			});
		},
		0
	);
}

void SettingsMainWindow::backButtonCallback()
{
	close();
}

void SettingsMainWindow::mainMenuButtonCallback()
{
	LOCPLINT->showYesNoDialog(
		CGI->generaltexth->allTexts[578],
		[this]()
		{
			close();
			GH.dispatchMainThread( []()
			{
				CSH->endGameplay();
				GH.defActionsDef = 63;
				CMM->menu->switchToTab("main");
			});
		},
		0
	);
}

void SettingsMainWindow::loadGameButtonCallback()
{
	close();
	LOCPLINT->proposeLoadingGame();
}

void SettingsMainWindow::saveGameButtonCallback()
{
	close();
	GH.windows().createAndPushWindow<CSavingScreen>();
}

void SettingsMainWindow::restartGameButtonCallback()
{
	LOCPLINT->showYesNoDialog(
		CGI->generaltexth->allTexts[67],
		[this]()
		{
			close();
			GH.dispatchMainThread([](){
				CSH->sendRestartGame();
			});
		},
		0
	);
}

void SettingsMainWindow::showAll(Canvas & to)
{
	auto color = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1);
	if(settings["session"]["spectate"].Bool())
		color = PlayerColor(1); // TODO: Spectator shouldn't need special code for UI colors

	CIntObject::showAll(to);
	CMessage::drawBorder(color, to.getInternalSurface(), pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

void SettingsMainWindow::onScreenResize()
{
	InterfaceObjectConfigurable::onScreenResize();

	auto tab = std::dynamic_pointer_cast<GeneralOptionsTab>(tabContentArea->getItem());

	if (tab)
		tab->updateResolutionSelector();
}
