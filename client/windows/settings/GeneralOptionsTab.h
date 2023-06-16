/*
 * GeneralOptionsTab.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../lib/CConfigHandler.h"
#include "../../gui/InterfaceObjectConfigurable.h"


class GeneralOptionsTab : public InterfaceObjectConfigurable
{
private:
	SettingsListener onFullscreenChanged;

	std::vector<Point> supportedResolutions;
	std::vector<int> supportedScaling;
	std::vector<int> longTouchDurations;

	void setFullscreenMode( bool on, bool exclusive);

	void selectGameResolution();
	void setGameResolution(int index);

	void selectGameScaling();
	void setGameScaling(int index);

	void selectLongTouchDuration();
	void setLongTouchDuration(int index);

public:
	GeneralOptionsTab();

	void updateResolutionSelector();
};
