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

#include "CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CPlayerInterface.h"
#include "CServerHandler.h"
#include "filesystem/ResourceID.h"
#include "gui/CGuiHandler.h"
#include "lobby/CSavingScreen.h"
#include "widgets/Buttons.h"
#include "widgets/Images.h"
#include "widgets/ObjectLists.h"
#include "windows/CMessage.h"

SettingsMainWindow::SettingsMainWindow(BattleInterface * parentBattleUi) : InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(ResourceID("config/widgets/settings/settingsMainContainer.json"));
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

	if(CSH->isGuest() || parentBattleUi)
	{
		loadButton->block(true);
		saveButton->block(true);
		restartButton->block(true);
	}

	int defaultTabIndex = 0;
	if(parentBattleUi != nullptr)
		defaultTabIndex = 2;
	else if(settings["general"]["lastSettingsTab"].isNumber())
		defaultTabIndex = settings["general"]["lastSettingsTab"].Integer();

	parentBattleInterface = parentBattleUi;
	tabContentArea = std::make_shared<CTabbedInt>(std::bind(&SettingsMainWindow::createTab, this, _1), Point(0, 0), defaultTabIndex);
	tabContentArea->type |= REDRAW_PARENT;

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
	if(GH.topInt().get() != this)
		logGlobal->error("Only top interface must be closed");
	GH.popInts(1);
}

void SettingsMainWindow::quitGameButtonCallback()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this](){ closeAndPushEvent(EUserEvent::FORCE_QUIT); }, 0);
}

void SettingsMainWindow::backButtonCallback()
{
	close();
}

void SettingsMainWindow::mainMenuButtonCallback()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this](){ closeAndPushEvent(EUserEvent::RETURN_TO_MAIN_MENU); }, 0);
}

void SettingsMainWindow::loadGameButtonCallback()
{
	close();
	LOCPLINT->proposeLoadingGame();
}

void SettingsMainWindow::saveGameButtonCallback()
{
	close();
	GH.pushIntT<CSavingScreen>();
}

void SettingsMainWindow::restartGameButtonCallback()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [this](){ closeAndPushEvent(EUserEvent::RESTART_GAME); }, 0);
}

void SettingsMainWindow::closeAndPushEvent(EUserEvent code)
{
	close();
	GH.pushUserEvent(code);
}

void SettingsMainWindow::showAll(SDL_Surface *to)
{
	auto color = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1);
	if(settings["session"]["spectate"].Bool())
		color = PlayerColor(1); // TODO: Spectator shouldn't need special code for UI colors

	CIntObject::showAll(to);
	CMessage::drawBorder(color, to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}
