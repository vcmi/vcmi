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

#include "../../../lib/filesystem/ResourcePath.h"
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
	setRedrawParent(true);

#ifdef VCMI_MOBILE
	addConditional("mobile", true);
	addConditional("desktop", false);
#else
	addConditional("mobile", false);
	addConditional("desktop", true);
#endif

	const JsonNode config(ResourcePath("config/widgets/settings/adventureOptionsTab.json"));
	addCallback("playerHeroSpeedChanged", [this](int value)
	{
		auto targetLabel = widget<CLabel>("heroSpeedValueLabel");
		if (targetLabel)
		{
			if (value <= 0)
			{
				targetLabel->setText("-");
			}
			else
			{
				int valuePercentage = 100 * 100 / value;
				targetLabel->setText(std::to_string(valuePercentage) + "%");
			}
		}
		setIntSetting("adventure", "heroMoveTime", value);
	});
	addCallback("enemyHeroSpeedChanged", [this](int value)
	{
		auto targetLabel = widget<CLabel>("enemySpeedValueLabel");

		if (targetLabel)
		{
			if (value <= 0)
			{
				targetLabel->setText("-");
			}
			else
			{
				int valuePercentage = 100 * 100 / value;
				targetLabel->setText(std::to_string(valuePercentage) + "%");
			}
		}
		setIntSetting("adventure", "enemyMoveTime", value);
	});
	addCallback("mapScrollSpeedChanged", [this](int value)
	{
		auto targetLabel = widget<CLabel>("mapScrollingValueLabel");
		int valuePercentage = 100 * value / 1200; // H3 max value is "1200", displaying it to be 100%
		if (targetLabel)
			targetLabel->setText(std::to_string(valuePercentage) + "%");

		return setIntSetting("adventure", "scrollSpeedPixels", value);
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
	addCallback("infoBarPickChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "infoBarPick", value);
	});
	addCallback("borderScrollChanged", [](bool value)
	{
		return setBoolSetting("adventure", "borderScroll", value);
	});
	addCallback("infoBarCreatureManagementChanged", [](bool value)
	{
		return setBoolSetting("gameTweaks", "infoBarCreatureManagement", value);
	});
	addCallback("leftButtonDragChanged", [](bool value)
	{
		return setBoolSetting("adventure", "leftButtonDrag", value);
	});
	build(config);

	std::shared_ptr<CToggleGroup> playerHeroSpeedToggle = widget<CToggleGroup>("heroMovementSpeedPicker");
	playerHeroSpeedToggle->setSelected(static_cast<int>(settings["adventure"]["heroMoveTime"].Float()));

	std::shared_ptr<CToggleGroup> enemyHeroSpeedToggle = widget<CToggleGroup>("enemyMovementSpeedPicker");
	enemyHeroSpeedToggle->setSelected(static_cast<int>(settings["adventure"]["enemyMoveTime"].Float()));

	std::shared_ptr<CToggleGroup> mapScrollSpeedToggle = widget<CToggleGroup>("mapScrollSpeedPicker");
	mapScrollSpeedToggle->setSelected(static_cast<int>(settings["adventure"]["scrollSpeedPixels"].Float()));

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

	std::shared_ptr<CToggleButton> infoBarPickCheckbox = widget<CToggleButton>("infoBarPickCheckbox");
	infoBarPickCheckbox->setSelected(settings["gameTweaks"]["infoBarPick"].Bool());

	std::shared_ptr<CToggleButton> borderScrollCheckbox = widget<CToggleButton>("borderScrollCheckbox");
	borderScrollCheckbox->setSelected(settings["adventure"]["borderScroll"].Bool());

	std::shared_ptr<CToggleButton> infoBarCreatureManagementCheckbox = widget<CToggleButton>("infoBarCreatureManagementCheckbox");
	infoBarCreatureManagementCheckbox->setSelected(settings["gameTweaks"]["infoBarCreatureManagement"].Bool());

	std::shared_ptr<CToggleButton> leftButtonDragCheckbox = widget<CToggleButton>("leftButtonDragCheckbox");
	if (leftButtonDragCheckbox)
		leftButtonDragCheckbox->setSelected(settings["adventure"]["leftButtonDrag"].Bool());
}
