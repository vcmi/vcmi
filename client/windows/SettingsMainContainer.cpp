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

#include "gui/CGuiHandler.h"
#include "gui/InterfaceObjectConfigurable.h"
#include "SettingsMainContainer.h"
#include "SystemOptionsWindow.h"
#include "VcmiSettingsWindow.h"
#include "../../lib/filesystem/ResourceID.h"
#include "battle/BattleInterfaceClasses.h"

SettingsMainContainer::SettingsMainContainer() : InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(ResourceID("config/widgets/settingsMainContainer.json"));
	addCallback("activateMainTab", [this](int) { openTab(0); });
	addCallback("activateBattleSettingsTab", [this](int) { openTab(1); });
	addCallback("activateVcmiSettingsTab", [this](int) { openTab(2); });
	build(config);

	int defaultTabIndex = 0;
	if(settings["general"]["lastSettingsTab"].isNumber())
		defaultTabIndex = settings["general"]["lastSettingsTab"].Integer();

	tabContentArea = std::make_shared<CTabbedInt>(std::bind(&SettingsMainContainer::createTab, this, _1), Point(50, 50), defaultTabIndex);
}

std::shared_ptr<CIntObject> SettingsMainContainer::createTab(size_t index)
{
	switch(index)
	{
		case 0:
			return std::make_shared<SystemOptionsWindow>();
		case 1:
			return std::make_shared<BattleOptionsWindow>(nullptr);
		case 2:
			return std::make_shared<VcmiSettingsWindow>();
		default:
			logGlobal->error("Wrong settings tab ID!");
			return std::make_shared<SystemOptionsWindow>();
	}
}

void SettingsMainContainer::openTab(size_t index)
{
	tabContentArea->setActive(index);
	CIntObject::redraw();

	Settings lastUsedTab = settings.write["general"]["lastSettingsTab"];
	lastUsedTab->Integer() = index;
}