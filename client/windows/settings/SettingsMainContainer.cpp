/*
 * SettingsMainContainer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include <SDL_events.h>
#include "StdInc.h"

#include "SettingsMainContainer.h"

#include "GeneralOptionsTab.h"
#include "AdventureOptionsTab.h"
#include "BattleOptionsTab.h"
#include "OtherOptionsWindow.h"

#include "filesystem/ResourceID.h"
#include "CGeneralTextHandler.h"
#include "gui/CGuiHandler.h"
#include "lobby/CSavingScreen.h"
#include "widgets/Buttons.h"
#include "widgets/Images.h"
#include "widgets/ObjectLists.h"
#include "CGameInfo.h"
#include "CPlayerInterface.h"
#include "CServerHandler.h"


SettingsMainContainer::SettingsMainContainer(BattleInterface * parentBattleUi) : InterfaceObjectConfigurable()
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

	std::shared_ptr<CPicture> background = widget<CPicture>("background");
	pos.w = background->pos.w;
	pos.h = background->pos.h;
	pos = center();

	std::shared_ptr<CButton> loadButton = widget<CButton>("loadButton");
	assert(loadButton);

	std::shared_ptr<CButton> saveButton = widget<CButton>("saveButton");
	assert(saveButton);

	std::shared_ptr<CButton> restartButton = widget<CButton>("restartButton");
	assert(restartButton);

	if(CSH->isGuest())
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
	tabContentArea = std::make_shared<CTabbedInt>(std::bind(&SettingsMainContainer::createTab, this, _1), Point(0, 40), defaultTabIndex);

	std::shared_ptr<CToggleGroup> mainTabs = widget<CToggleGroup>("settingsTabs");
	mainTabs->setSelected(defaultTabIndex);
}

std::shared_ptr<CIntObject> SettingsMainContainer::createTab(size_t index)
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
			return std::make_shared<OtherOptionsWindow>();
		default:
			logGlobal->error("Wrong settings tab ID!");
			return std::make_shared<GeneralOptionsTab>();
	}
}

void SettingsMainContainer::openTab(size_t index)
{
	tabContentArea->setActive(index);
	CIntObject::redraw();

	Settings lastUsedTab = settings.write["general"]["lastSettingsTab"];
	lastUsedTab->Integer() = index;
}

void SettingsMainContainer::close()
{
	if(GH.topInt().get() != this)
		logGlobal->error("Only top interface must be closed");
	GH.popInts(1);
}

void SettingsMainContainer::quitGameButtonCallback()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this](){ closeAndPushEvent(EUserEvent::FORCE_QUIT); }, 0);
}

void SettingsMainContainer::backButtonCallback()
{
	close();
}

void SettingsMainContainer::mainMenuButtonCallback()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[578], [this](){ closeAndPushEvent(EUserEvent::RETURN_TO_MAIN_MENU); }, 0);
}

void SettingsMainContainer::loadGameButtonCallback()
{
	close();
	LOCPLINT->proposeLoadingGame();
}

void SettingsMainContainer::saveGameButtonCallback()
{
	close();
	GH.pushIntT<CSavingScreen>();
}

void SettingsMainContainer::restartGameButtonCallback()
{
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [this](){ closeAndPushEvent(EUserEvent::RESTART_GAME); }, 0);
}

void SettingsMainContainer::closeAndPushEvent(EUserEvent code)
{
	close();
	GH.pushUserEvent(code);
}