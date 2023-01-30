/*
 * VcmiSettingsWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "VcmiSettingsWindow.h"

#include "../lib/filesystem/ResourceID.h"
#include "gui/CGuiHandler.h"
#include "SystemOptionsWindow.h"

VcmiSettingsWindow::VcmiSettingsWindow() : InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(ResourceID("config/widgets/vcmiSettings.json"));
	addCallback("closeWindow", [this](int) { backButtonCallback(); });
	addCallback("openSystemOptions", [this](int) { openSystemOptionsCallback(); });
	build(config);
}

void VcmiSettingsWindow::close()
{
	if(GH.topInt().get() != this)
		logGlobal->error("Only top interface must be closed");
	GH.popInts(1);
}

void VcmiSettingsWindow::backButtonCallback()
{
	close();
}

void VcmiSettingsWindow::openSystemOptionsCallback()
{
	close();
	GH.pushIntT<SystemOptionsWindow>();
}

