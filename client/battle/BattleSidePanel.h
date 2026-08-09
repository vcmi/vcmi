/*
 * BattleSidePanel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

/// Panel next to the battlefield, which may end up placed above it on small resolutions
class BattleSidePanel : public CIntObject
{
	/// set while the panel sits above the battlefield, which erases its area every frame
	bool aboveBattlefield = false;

public:
	explicit BattleSidePanel(int used = 0, Point offset = Point())
		: CIntObject(used, offset)
	{
	}

	void setAboveBattlefield(bool on)
	{
		aboveBattlefield = on;
	}

	void show(Canvas & to) override
	{
		if(aboveBattlefield)
			showAll(to);
		CIntObject::show(to);
	}
};
