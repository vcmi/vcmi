/*
 * MapOverlayLogVisualizer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapOverlayLogVisualizer.h"
#include "MapViewModel.h"

#include "../../lib/logging/VisualLogger.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/EFont.h"
#include "../render/IFont.h"
#include "../render/IScreenHandler.h"
#include "../render/IRenderHandler.h"
#include "../render/Graphics.h"
#include "../gui/TextAlignment.h"
#include "../GameEngine.h"


MapOverlayLogVisualizer::MapOverlayLogVisualizer(Canvas & target, std::shared_ptr<MapViewModel> model)
	: target(target), model(model)
{
}

void MapOverlayLogVisualizer::drawLine(int3 start, int3 end)
{
	auto level = model->getLevel();

	if(start.z != level || end.z != level)
		return;

	int scaling = ENGINE->screenHandler().getScalingFactor();
	auto pStart = model->getTargetTileArea(start).center();
	auto pEnd = model->getTargetTileArea(end).center();
	Rect viewPortRaw = target.getRenderArea();
	Rect viewPort(viewPortRaw.topLeft() / scaling, viewPortRaw.dimensions() / scaling );

	Point workaroundOffset(8,8); // not sure why it is needed. Removing leads to incorrect clipping near view edges

	if(viewPort.isInside(pStart + workaroundOffset) && viewPort.isInside(pEnd + workaroundOffset))
	{
		target.drawLine(pStart, pEnd, ColorRGBA(255, 255, 0), ColorRGBA(255, 0, 0));
	}
}

void MapOverlayLogVisualizer::drawText(
	int3 tile,
	int lineNumber,
	const std::string & text,
	const std::optional<ColorRGBA> & background)
{
	const Point offset = Point(6, 6);

	auto level = model->getLevel();

	if(tile.z != level)
		return;

	auto pStart = offset + model->getTargetTileArea(tile).topLeft();
	auto viewPort = target.getRenderArea();

	ColorRGBA color = Colors::YELLOW;
	
	if(background)
	{
		color = ((background->b + background->r + background->g) < 300)
			? Colors::WHITE
			: Colors::BLACK;
	}

	if(viewPort.isInside(pStart))
	{
		const auto & font = ENGINE->renderHandler().loadFont(FONT_TINY);

		int w = font->getStringWidth(text);
		int h = font->getLineHeight();

		pStart.y += h * lineNumber;

		if(background)
		{
			target.drawColor(Rect(pStart, Point(w + 4, h)), *background);
			pStart.x += 2;
		}

		target.drawText(pStart, EFonts::FONT_TINY, color, ETextAlignment::TOPLEFT, text);
	}
}
