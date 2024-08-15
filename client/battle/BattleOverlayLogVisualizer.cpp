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
#include "../render/IFont.h"
#include "../gui/TextAlignment.h"
#include "../render/Graphics.h"

BattleOverlayLogVisualizer::BattleOverlayLogVisualizer(
	BattleRenderer::RendererRef & target,
	BattleInterface & owner)
	: target(target), owner(owner)
{
}

void BattleOverlayLogVisualizer::drawText(BattleHex hex, int lineNumber, const std::string & text)
{
	Point offset = owner.fieldController->hexPositionLocal(hex).topLeft() + Point(20, 20);
	int h = graphics->fonts[EFonts::FONT_TINY]->getLineHeight();

	offset.y += h * lineNumber;

	target.drawText(offset, EFonts::FONT_TINY, Colors::YELLOW, ETextAlignment::TOPCENTER, text);
}
