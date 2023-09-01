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

#include "OtherOptionsTab.h"

#include "../../../lib/filesystem/ResourcePath.h"
#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "CConfigHandler.h"

static void setBoolSetting(std::string group, std::string field, bool value)
{
	Settings fullscreen = settings.write[group][field];
	fullscreen->Bool() = value;
}

OtherOptionsTab::OtherOptionsTab() : InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(JsonPath::builtin("config/widgets/settings/otherOptionsTab.json"));
	addCallback("availableCreaturesAsDwellingLabelChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "availableCreaturesAsDwellingLabel", value);
	});
	addCallback("compactTownCreatureInfoChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "compactTownCreatureInfo", value);
	});
	build(config);

	std::shared_ptr<CToggleButton> availableCreaturesAsDwellingLabelCheckbox = widget<CToggleButton>("availableCreaturesAsDwellingLabelCheckbox");
	availableCreaturesAsDwellingLabelCheckbox->setSelected(settings["gameTweaks"]["availableCreaturesAsDwellingLabel"].Bool());

	std::shared_ptr<CToggleButton> compactTownCreatureInfo = widget<CToggleButton>("compactTownCreatureInfoCheckbox");
	compactTownCreatureInfo->setSelected(settings["gameTweaks"]["compactTownCreatureInfo"].Bool());
}


