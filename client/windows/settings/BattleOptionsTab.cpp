/*
 * BattleOptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleOptionsTab.h"

#include "../../battle/BattleInterface.h"
#include "../../gui/CGuiHandler.h"
#include "../../../lib/CConfigHandler.h"
#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/CGeneralTextHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

BattleOptionsTab::BattleOptionsTab(BattleInterface * owner)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	setRedrawParent(true);

	const JsonNode config(JsonPath::builtin("config/widgets/settings/battleOptionsTab.json"));
	addCallback("viewGridChanged", [this, owner](bool value)
	{
		viewGridChangedCallback(value, owner);
	});
	addCallback("movementShadowChanged", [this, owner](bool value)
	{
		movementShadowChangedCallback(value, owner);
	});
	addCallback("movementHighlightOnHoverChanged", [this, owner](bool value)
	{
		movementHighlightOnHoverChangedCallback(value, owner);
	});
	addCallback("rangeLimitHighlightOnHoverChanged", [this, owner](bool value)
	{
		rangeLimitHighlightOnHoverChangedCallback(value, owner);
	});
	addCallback("mouseShadowChanged", [this](bool value)
	{
		mouseShadowChangedCallback(value);
	});
	addCallback("animationSpeedChanged", [this](int value)
	{
		animationSpeedChangedCallback(value);
	});
	addCallback("showQueueChanged", [this, owner](bool value)
	{
		showQueueChangedCallback(value, owner);
	});
	addCallback("queueSizeChanged", [this, owner](int value)
	{
		queueSizeChangedCallback(value, owner);
	});
	addCallback("skipBattleIntroMusicChanged", [this](bool value)
	{
		skipBattleIntroMusicChangedCallback(value);
	});
	addCallback("showStickyHeroWindowsChanged", [this, owner](bool value)
	{
		showStickyHeroWindowsChangedCallback(value, owner);
	});
	addCallback("enableAutocombatSpellsChanged", [this](bool value)
	{
		enableAutocombatSpellsChangedCallback(value);
	});
	build(config);

	std::shared_ptr<CToggleGroup> animationSpeedToggle = widget<CToggleGroup>("animationSpeedPicker");
	animationSpeedToggle->setSelected(getAnimSpeed());

	std::shared_ptr<CToggleGroup> queueSizeToggle = widget<CToggleGroup>("queueSizePicker");
	queueSizeToggle->setSelected(getQueueSizeId());

	std::shared_ptr<CToggleButton> viewGridCheckbox = widget<CToggleButton>("viewGridCheckbox");
	viewGridCheckbox->setSelected(settings["battle"]["cellBorders"].Bool());

	std::shared_ptr<CToggleButton> movementShadowCheckbox = widget<CToggleButton>("movementShadowCheckbox");
	movementShadowCheckbox->setSelected(settings["battle"]["stackRange"].Bool());

	std::shared_ptr<CToggleButton> movementHighlightOnHoverCheckbox = widget<CToggleButton>("movementHighlightOnHoverCheckbox");
	movementHighlightOnHoverCheckbox->setSelected(settings["battle"]["movementHighlightOnHover"].Bool());

	std::shared_ptr<CToggleButton> rangeLimitHighlightOnHoverCheckbox = widget<CToggleButton>("rangeLimitHighlightOnHoverCheckbox");
	rangeLimitHighlightOnHoverCheckbox->setSelected(settings["battle"]["rangeLimitHighlightOnHover"].Bool());

	std::shared_ptr<CToggleButton> showStickyHeroInfoWindowsCheckbox = widget<CToggleButton>("showStickyHeroInfoWindowsCheckbox");
	showStickyHeroInfoWindowsCheckbox->setSelected(settings["battle"]["stickyHeroInfoWindows"].Bool());

	std::shared_ptr<CToggleButton> mouseShadowCheckbox = widget<CToggleButton>("mouseShadowCheckbox");
	mouseShadowCheckbox->setSelected(settings["battle"]["mouseShadow"].Bool());

	std::shared_ptr<CToggleButton> skipBattleIntroMusicCheckbox = widget<CToggleButton>("skipBattleIntroMusicCheckbox");
	skipBattleIntroMusicCheckbox->setSelected(settings["gameTweaks"]["skipBattleIntroMusic"].Bool());

	std::shared_ptr<CToggleButton> enableAutocombatSpellsCheckbox = widget<CToggleButton>("enableAutocombatSpellsCheckbox");
	enableAutocombatSpellsCheckbox->setSelected(settings["battle"]["enableAutocombatSpells"].Bool());
}

int BattleOptionsTab::getAnimSpeed() const
{
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-battle-speed"].isNull())
		return static_cast<int>(std::round(settings["session"]["spectate-battle-speed"].Float()));

	return static_cast<int>(std::round(settings["battle"]["speedFactor"].Float()));
}

int BattleOptionsTab::getQueueSizeId() const
{
	std::string sizeText = settings["battle"]["queueSize"].String();
	bool visible = settings["battle"]["showQueue"].Bool();

	if(!visible)
		return -1;

	if(sizeText == "none")
		return -1;
	if(sizeText == "auto")
		return 0;
	if(sizeText == "small")
		return 1;
	if(sizeText == "big")
		return 2;

	return 0;
}

std::string BattleOptionsTab::getQueueSizeStringFromId(int value) const
{
	switch(value)
	{
		case -1:
			return "none";
		case 0:
			return "auto";
		case 1:
			return "small";
		case 2:
			return "big";
		default:
			return "auto";
	}
}

void BattleOptionsTab::viewGridChangedCallback(bool value, BattleInterface * parentBattleInterface)
{
	Settings cellBorders = settings.write["battle"]["cellBorders"];
	cellBorders->Bool() = value;
	if(parentBattleInterface)
		parentBattleInterface->redrawBattlefield();
}

void BattleOptionsTab::movementShadowChangedCallback(bool value, BattleInterface * parentBattleInterface)
{
	Settings stackRange = settings.write["battle"]["stackRange"];
	stackRange->Bool() = value;
	if(parentBattleInterface)
		parentBattleInterface->redrawBattlefield();
}

void BattleOptionsTab::movementHighlightOnHoverChangedCallback(bool value, BattleInterface * parentBattleInterface)
{
	Settings stackRange = settings.write["battle"]["movementHighlightOnHover"];
	stackRange->Bool() = value;
	if(parentBattleInterface)
		parentBattleInterface->redrawBattlefield();
}

void BattleOptionsTab::rangeLimitHighlightOnHoverChangedCallback(bool value, BattleInterface * parentBattleInterface)
{
	Settings stackRange = settings.write["battle"]["rangeLimitHighlightOnHover"];
	stackRange->Bool() = value;
	if(parentBattleInterface)
		parentBattleInterface->redrawBattlefield();
}

void BattleOptionsTab::mouseShadowChangedCallback(bool value)
{
	Settings shadow = settings.write["battle"]["mouseShadow"];
	shadow->Bool() = value;
}

void BattleOptionsTab::animationSpeedChangedCallback(int value)
{
	Settings speed = settings.write["battle"]["speedFactor"];
	speed->Float() = static_cast<float>(value);

	auto targetLabel = widget<CLabel>("animationSpeedValueLabel");
	int valuePercentage = value * 100 / 3; // H3 max value is "3", displaying it to be 100%
	if (targetLabel)
		targetLabel->setText(std::to_string(valuePercentage) + "%");
}

void BattleOptionsTab::showQueueChangedCallback(bool value, BattleInterface * parentBattleInterface)
{
	if(!parentBattleInterface)
	{
		Settings showQueue = settings.write["battle"]["showQueue"];
		showQueue->Bool() = value;
	}
	else
	{
		parentBattleInterface->setBattleQueueVisibility(value);
	}
}

void BattleOptionsTab::showStickyHeroWindowsChangedCallback(bool value, BattleInterface * parentBattleInterface)
{
	if(!parentBattleInterface)
	{
		Settings showStickyWindows = settings.write["battle"]["stickyHeroInfoWindows"];
		showStickyWindows->Bool() = value;
	}
	else
	{
		parentBattleInterface->setStickyHeroWindowsVisibility(value);
	}
}

void BattleOptionsTab::queueSizeChangedCallback(int value, BattleInterface * parentBattleInterface)
{
	if (value == -1)
	{
		showQueueChangedCallback(false, parentBattleInterface);
		return;
	}

	std::string stringifiedValue = getQueueSizeStringFromId(value);
	Settings size = settings.write["battle"]["queueSize"];
	size->String() = stringifiedValue;

	showQueueChangedCallback(true, parentBattleInterface);
}

void BattleOptionsTab::skipBattleIntroMusicChangedCallback(bool value)
{
	Settings musicSkipSettingValue = settings.write["gameTweaks"]["skipBattleIntroMusic"];
	musicSkipSettingValue->Bool() = value;
}

void BattleOptionsTab::enableAutocombatSpellsChangedCallback(bool value)
{
	Settings enableAutocombatSpells = settings.write["battle"]["enableAutocombatSpells"];
	enableAutocombatSpells->Bool() = value;
}

