/*
 * CPanel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CFilledTexture;

/// Opaque area that is a base for other controls. A redraw of a control inside stops at the panel
class CPanel : public CIntObject
{
	std::shared_ptr<CFilledTexture> background;
	bool bordered;

public:
	CPanel(const Rect & area, bool bordered = true);

	void showAll(Canvas & to) override;
};
