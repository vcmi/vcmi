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
#include "../render/IRenderHandler.h"
#include "../gui/TextAlignment.h"
#include "../GameEngine.h"
#include "../render/Graphics.h"

BattleOverlayLogVisualizer::BattleOverlayLogVisualizer(
	BattleRenderer::RendererRef & target,
	BattleInterface & owner)
	: target(target), owner(owner)
{
}

void BattleOverlayLogVisualizer::drawText(const BattleHex & hex, int lineNumber, const std::string & text)
{
	Point offset = owner.fieldController->hexPositionLocal(hex).topLeft() + Point(20, 20);
	const auto & font = ENGINE->renderHandler().loadFont(FONT_TINY);
	int h = font->getLineHeight();

	offset.y += h * lineNumber;

	target.drawText(offset, EFonts::FONT_TINY, Colors::YELLOW, ETextAlignment::TOPCENTER, text);
}
