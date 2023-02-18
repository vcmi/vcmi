/*
 * AdventureOptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AdventureOptionsTab.h"

#include "../../../lib/filesystem/ResourceID.h"
#include "../../gui/CGuiHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/Images.h"
#include "CConfigHandler.h"

static void setBoolSetting(std::string group, std::string field, bool value)
{
	Settings fullscreen = settings.write[group][field];
	fullscreen->Bool() = value;
}

static void setIntSetting(std::string group, std::string field, int value)
{
	Settings entry = settings.write[group][field];
	entry->Float() = value;
}

AdventureOptionsTab::AdventureOptionsTab()
		: InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	const JsonNode config(ResourceID("config/widgets/settings/adventureOptionsTab.json"));
	addCallback("playerHeroSpeedChanged", [](int value)
	{
		return setIntSetting("adventure", "heroSpeed", value);
	});
	addCallback("enemyHeroSpeedChanged", [](int value)
	{
		return setIntSetting("adventure", "enemySpeed", value);
	});
	addCallback("mapScrollSpeedChanged", [](int value)
	{
		return setIntSetting("adventure", "scrollSpeed", value);
	});
	addCallback("heroReminderChanged", [](bool value)
	{
		return setBoolSetting("adventure", "heroReminder", value);
	});
	addCallback("quickCombatChanged", [](bool value)
	{
		return setBoolSetting("adventure", "quickCombat", value);
	});
	//settings that do not belong to base game:
	addCallback("numericQuantitiesChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "numericCreaturesQuantities", value);
	});
	addCallback("forceMovementInfoChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "forceMovementInfo", value);
	});
	addCallback("showGridChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "showGrid", value);
	});
	build(config);

	std::shared_ptr<CToggleGroup> playerHeroSpeedToggle = widget<CToggleGroup>("heroMovementSpeedPicker");
	playerHeroSpeedToggle->setSelected(static_cast<int>(settings["adventure"]["heroSpeed"].Float()));

	std::shared_ptr<CToggleGroup> enemyHeroSpeedToggle = widget<CToggleGroup>("enemyMovementSpeedPicker");
	enemyHeroSpeedToggle->setSelected(static_cast<int>(settings["adventure"]["enemySpeed"].Float()));

	std::shared_ptr<CToggleGroup> mapScrollSpeedToggle = widget<CToggleGroup>("mapScrollSpeedPicker");
	mapScrollSpeedToggle->setSelected(static_cast<int>(settings["adventure"]["scrollSpeed"].Float()));

	std::shared_ptr<CToggleButton> heroReminderCheckbox = widget<CToggleButton>("heroReminderCheckbox");
	heroReminderCheckbox->setSelected(settings["adventure"]["heroReminder"].Bool());

	std::shared_ptr<CToggleButton> quickCombatCheckbox = widget<CToggleButton>("quickCombatCheckbox");
	quickCombatCheckbox->setSelected(settings["adventure"]["quickCombat"].Bool());

	std::shared_ptr<CToggleButton> numericQuantitiesCheckbox = widget<CToggleButton>("numericQuantitiesCheckbox");
	numericQuantitiesCheckbox->setSelected(settings["gameTweaks"]["numericCreaturesQuantities"].Bool());

	std::shared_ptr<CToggleButton> forceMovementInfoCheckbox = widget<CToggleButton>("forceMovementInfoCheckbox");
	forceMovementInfoCheckbox->setSelected(settings["gameTweaks"]["forceMovementInfo"].Bool());

	std::shared_ptr<CToggleButton> showGridCheckbox = widget<CToggleButton>("showGridCheckbox");
	showGridCheckbox->setSelected(settings["gameTweaks"]["showGrid"].Bool());
}