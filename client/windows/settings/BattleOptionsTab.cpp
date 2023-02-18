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
#include "CConfigHandler.h"

#include "../../battle/BattleInterface.h"
#include "../../gui/CGuiHandler.h"
#include "../../../lib/filesystem/ResourceID.h"
#include "../../../lib/CGeneralTextHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/TextControls.h"

BattleOptionsTab::BattleOptionsTab(BattleInterface * owner):
		InterfaceObjectConfigurable()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(ResourceID("config/widgets/settings/battleOptionsTab.json"));
	addCallback("viewGridChanged", [this, owner](bool value)
	{
		viewGridChangedCallback(value, owner);
	});
	addCallback("movementShadowChanged", [this, owner](bool value)
	{
		movementShadowChangedCallback(value, owner);
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
	addCallback("queueSizeChanged", [this](int value)
	{
		queueSizeChangedCallback(value);
	});
	addCallback("skipBattleIntroMusicChanged", [this](bool value)
	{
		skipBattleIntroMusicChangedCallback(value);
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

	std::shared_ptr<CToggleButton> mouseShadowCheckbox = widget<CToggleButton>("mouseShadowCheckbox");
	mouseShadowCheckbox->setSelected(settings["battle"]["mouseShadow"].Bool());

	std::shared_ptr<CToggleButton> showQueueCheckbox = widget<CToggleButton>("showQueueCheckbox");
	showQueueCheckbox->setSelected(settings["battle"]["showQueue"].Bool());

	std::shared_ptr<CToggleButton> skipBattleIntroMusicCheckbox = widget<CToggleButton>("skipBattleIntroMusicCheckbox");
	skipBattleIntroMusicCheckbox->setSelected(settings["gameTweaks"]["skipBattleIntroMusic"].Bool());
}

int BattleOptionsTab::getAnimSpeed() const
{
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-battle-speed"].isNull())
		return static_cast<int>(std::round(settings["session"]["spectate-battle-speed"].Float()));

	return static_cast<int>(std::round(settings["battle"]["speedFactor"].Float()));
}

int BattleOptionsTab::getQueueSizeId() const
{
	std::string text = settings["battle"]["queueSize"].String();
	if(text == "auto")
		return 0;
	else if(text == "small")
		return 1;
	else if(text == "big")
		return 2;

	return 0;
}

std::string BattleOptionsTab::getQueueSizeStringFromId(int value) const
{
	switch(value)
	{
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

void BattleOptionsTab::mouseShadowChangedCallback(bool value)
{
	Settings shadow = settings.write["battle"]["mouseShadow"];
	shadow->Bool() = value;
}

void BattleOptionsTab::animationSpeedChangedCallback(int value)
{
	Settings speed = settings.write["battle"]["speedFactor"];
	speed->Float() = static_cast<float>(value);
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

void BattleOptionsTab::queueSizeChangedCallback(int value)
{
	std::string stringifiedValue = getQueueSizeStringFromId(value);
	Settings size = settings.write["battle"]["queueSize"];
	size->String() = stringifiedValue;
}

void BattleOptionsTab::skipBattleIntroMusicChangedCallback(bool value)
{
	Settings musicSkipSettingValue = settings.write["gameTweaks"]["skipBattleIntroMusic"];
	musicSkipSettingValue->Bool() = value;
}

