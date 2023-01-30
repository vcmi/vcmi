/*
 * SystemOptionsWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/CConfigHandler.h"
#include "gui/InterfaceObjectConfigurable.h"

class SystemOptionsWindow : public InterfaceObjectConfigurable
{
private:
	SettingsListener onFullscreenChanged;

	void selectGameResolution();
	void setGameResolution(int index);

public:
	SystemOptionsWindow();
};