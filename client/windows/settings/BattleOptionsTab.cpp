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
	addCallback("viewGridChanged", std::bind(&BattleOptionsTab::viewGridChangedCallback, this, _1, owner));
	addCallback("movementShadowChanged", std::bind(&BattleOptionsTab::movementShadowChangedCallback, this, _1, owner));
	addCallback("mouseShadowChanged", std::bind(&BattleOptionsTab::mouseShadowChangedCallback, this, _1));
	addCallback("animationSpeedChanged", std::bind(&BattleOptionsTab::animationSpeedChangedCallback, this, _1));
	build(config);

	std::shared_ptr<CToggleGroup> animationSpeedToggle = widget<CToggleGroup>("animationSpeedPicker");
	animationSpeedToggle->setSelected(getAnimSpeed());


	std::shared_ptr<CToggleButton> viewGridCheckbox = widget<CToggleButton>("viewGridCheckbox");
	viewGridCheckbox->setSelected((bool)settings["battle"]["cellBorders"].Bool());

	std::shared_ptr<CToggleButton> movementShadowCheckbox = widget<CToggleButton>("movementShadowCheckbox");
	movementShadowCheckbox->setSelected((bool)settings["battle"]["stackRange"].Bool());

	std::shared_ptr<CToggleButton> mouseShadowCheckbox = widget<CToggleButton>("mouseShadowCheckbox");
	mouseShadowCheckbox->setSelected((bool)settings["battle"]["mouseShadow"].Bool());
}

int BattleOptionsTab::getAnimSpeed() const
{
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-battle-speed"].isNull())
		return static_cast<int>(std::round(settings["session"]["spectate-battle-speed"].Float()));

	return static_cast<int>(std::round(settings["battle"]["speedFactor"].Float()));
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
	speed->Float() = float(value);
}

