/*
 * BattleOverlayLogVisualizer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleOverlayLogVisualizer.h"
#include "BattleInterface.h"
#include "BattleFieldController.h"

#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/EFont.h"
#include "../gui/TextAlignment.h"

BattleOverlayLogVisualizer::BattleOverlayLogVisualizer(
	BattleRenderer::RendererRef & target,
	BattleInterface & owner)
	: target(target), owner(owner)
{
}

void BattleOverlayLogVisualizer::drawText(BattleHex hex, std::vector<std::string> texts)
{
	const Point offset = owner.fieldController->hexPositionLocal(hex).topLeft() + Point(20, 20);

	target.drawText(offset, EFonts::FONT_TINY, Colors::RED, ETextAlignment::TOPCENTER, texts);
}
