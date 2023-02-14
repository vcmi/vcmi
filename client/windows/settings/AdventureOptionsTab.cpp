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
	addCallback("playerHeroSpeedChanged", std::bind(&setIntSetting, "adventure", "heroSpeed", _1));
	addCallback("enemyHeroSpeedChanged", std::bind(&setIntSetting, "adventure", "enemySpeed", _1));
	addCallback("mapScrollSpeedChanged", std::bind(&setIntSetting, "adventure", "scrollSpeed", _1));
	addCallback("heroReminderChanged", std::bind(&setBoolSetting, "adventure", "heroReminder", _1));
	addCallback("quickCombatChanged", std::bind(&setBoolSetting, "adventure", "quickCombat", _1));
	//settings that do not belong to base game:
	addCallback("numericQuantitiesChanged", std::bind(&setBoolSetting, "gameTweaks", "numericCreaturesQuantities", _1));
	addCallback("forceMovementInfoChanged", std::bind(&setBoolSetting, "gameTweaks", "forceMovementInfo", _1));
	addCallback("showGridChanged", std::bind(&setBoolSetting, "gameTweaks", "showGrid", _1));
	build(config);

	std::shared_ptr<CToggleGroup> playerHeroSpeedToggle = widget<CToggleGroup>("heroMovementSpeedPicker");
	playerHeroSpeedToggle->setSelected((int)settings["adventure"]["heroSpeed"].Float());

	std::shared_ptr<CToggleGroup> enemyHeroSpeedToggle = widget<CToggleGroup>("enemyMovementSpeedPicker");
	enemyHeroSpeedToggle->setSelected((int)settings["adventure"]["enemySpeed"].Float());

	std::shared_ptr<CToggleGroup> mapScrollSpeedToggle = widget<CToggleGroup>("mapScrollSpeedPicker");
	mapScrollSpeedToggle->setSelected((int)settings["adventure"]["scrollSpeed"].Float());

	std::shared_ptr<CToggleButton> heroReminderCheckbox = widget<CToggleButton>("heroReminderCheckbox");
	heroReminderCheckbox->setSelected((bool)settings["adventure"]["heroReminder"].Bool());

	std::shared_ptr<CToggleButton> quickCombatCheckbox = widget<CToggleButton>("quickCombatCheckbox");
	quickCombatCheckbox->setSelected((bool)settings["adventure"]["quickCombat"].Bool());

	std::shared_ptr<CToggleButton> numericQuantitiesCheckbox = widget<CToggleButton>("numericQuantitiesCheckbox");
	numericQuantitiesCheckbox->setSelected((bool)settings["gameTweaks"]["numericCreaturesQuantities"].Bool());

	std::shared_ptr<CToggleButton> forceMovementInfoCheckbox = widget<CToggleButton>("forceMovementInfoCheckbox");
	forceMovementInfoCheckbox->setSelected((bool)settings["gameTweaks"]["forceMovementInfo"].Bool());

	std::shared_ptr<CToggleButton> showGridCheckbox = widget<CToggleButton>("showGridCheckbox");
	showGridCheckbox->setSelected((bool)settings["gameTweaks"]["showGrid"].Bool());
}