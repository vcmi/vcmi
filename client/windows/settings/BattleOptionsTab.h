/*
 * BattleOptionsTab.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../gui/InterfaceObjectConfigurable.h"
#include "../../battle/BattleInterface.h"

class BattleOptionsTab : public InterfaceObjectConfigurable
{
private:
	std::shared_ptr<CToggleGroup> animSpeeds;
	std::vector<std::shared_ptr<CToggleButton>> toggles;

	int getAnimSpeed() const;
	void viewGridChangedCallback(bool value, BattleInterface * parentBattleInterface = nullptr);
	void movementShadowChangedCallback(bool value, BattleInterface * parentBattleInterface = nullptr);
	void mouseShadowChangedCallback(bool value);
	void animationSpeedChangedCallback(int value);
public:
	BattleOptionsTab(BattleInterface * owner = nullptr);
};


