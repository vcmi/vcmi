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

	/// Current panels consist of direct image and text children, without nested containers.
	std::optional<Rect> visibleContentBounds() const
	{
		if(isDisabled())
			return {};
		std::optional<Rect> bounds;
		if(pos.w > 0 && pos.h > 0)
			bounds = pos;
		for(const auto child : children)
		{
			if(!child->isDisabled() && child->pos.w > 0 && child->pos.h > 0)
				bounds = bounds ? bounds->include(child->pos) : child->pos;
		}
		return bounds;
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
