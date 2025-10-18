/*
 * BattleOnlyMode.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"

class FilledTexturePlayerColored;
class CButton;

class BattleOnlyMode
{
public:
	static void openBattleWindow();
};
class BattleOnlyModeWindow : public CWindowObject
{
private:
	std::shared_ptr<FilledTexturePlayerColored> backgroundTexture;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonAbort;

	void startBattle();
public:
	BattleOnlyModeWindow();
};