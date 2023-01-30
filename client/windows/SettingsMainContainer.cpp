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
#include "../lib/filesystem/ResourceID.h"


SettingsMainContainer::SettingsMainContainer() : InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	//OBJECT_CONSTRUCTION_CAPTURING(255);

	const JsonNode config(ResourceID("config/widgets/settingsMainContainer.json"));
	addCallback("activateMainTab", [this](int) { tabContentArea->setActive(0); CIntObject::redraw(); });
	addCallback("activateVcmiSettingsTab", [this](int) { tabContentArea->setActive(1); CIntObject::redraw(); });
	build(config);

	tabContentArea = std::make_shared<CTabbedInt>(std::bind(&SettingsMainContainer::createTab, this, _1), Point(50, 50));
}

std::shared_ptr<CIntObject> SettingsMainContainer::createTab(size_t index)
{
	switch(index)
	{
		case 0:
			return std::make_shared<SystemOptionsWindow>();
		case 1:
			return std::make_shared<VcmiSettingsWindow>();
		default:
			logGlobal->error("Wrong settings tab ID!");
			return std::make_shared<CIntObject>();
	}
}