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

class BattleInterface;

class BattleOptionsTab : public InterfaceObjectConfigurable
{
private:
	std::shared_ptr<CToggleGroup> animSpeeds;
	std::vector<std::shared_ptr<CToggleButton>> toggles;

	int getAnimSpeed() const;
	int getQueueSizeId() const;
	std::string getQueueSizeStringFromId(int value) const;
	void viewGridChangedCallback(bool value, BattleInterface * parentBattleInterface);
	void movementShadowChangedCallback(bool value, BattleInterface * parentBattleInterface);
	void mouseShadowChangedCallback(bool value);
	void animationSpeedChangedCallback(int value);
	void showQueueChangedCallback(bool value, BattleInterface * parentBattleInterface);
	void queueSizeChangedCallback(int value, BattleInterface * parentBattleInterface);
	void skipBattleIntroMusicChangedCallback(bool value);
public:
	BattleOptionsTab(BattleInterface * owner = nullptr);
};


