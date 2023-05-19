/*
 * SettingsMainContainer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "gui/InterfaceObjectConfigurable.h"

class BattleInterface;
class CTabbedInt;
enum class EUserEvent;

/// <summary>
///Original H3 settings were replaced with this window in Github PR #1540. New options added are intended to be clientside settings.
///By design, most new options that alter gameplay UI from now on should be added to "gameTweaks" config key.
/// </summary>
class SettingsMainWindow : public InterfaceObjectConfigurable
{
private:
	BattleInterface * parentBattleInterface;
	std::shared_ptr<CTabbedInt> tabContentArea;

	std::shared_ptr<CIntObject> createTab(size_t index);
	void openTab(size_t index);

	void close(); //TODO: copypaste of WindowBase::close(), consider changing Windowbase to IWindowbase with default close() implementation and changing WindowBase inheritance to CIntObject + IWindowBase
	void closeAndPushEvent(EUserEvent code);

	void loadGameButtonCallback();
	void saveGameButtonCallback();
	void quitGameButtonCallback();
	void backButtonCallback();
	void restartGameButtonCallback();
	void mainMenuButtonCallback();
public:
	SettingsMainWindow(BattleInterface * parentBattleInterface = nullptr);

	void showAll(SDL_Surface * to) override;
	void onScreenResize() override;
};

