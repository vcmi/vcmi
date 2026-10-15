/*
 * CPanel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPanel.h"

#include "Images.h"
#include "UiTheme.h"

#include "render/Canvas.h"

CPanel::CPanel(const Rect & area, bool bordered)
	: bordered(bordered)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	OBJECT_CONSTRUCTION;
	auto blueBackground = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, area.w, area.h));
	blueBackground->setPlayerColor(PlayerColor(1));
	background = blueBackground;
}

void CPanel::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	if(bordered)
		to.drawBorder(pos, UiTheme::BORDER);
}
