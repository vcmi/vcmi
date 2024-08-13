/*
 * GraphicalPrimitiveCanvas.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GraphicalPrimitiveCanvas.h"

#include "../render/Canvas.h"

GraphicalPrimitiveCanvas::GraphicalPrimitiveCanvas(Rect dimensions)
{
	pos = dimensions + pos.topLeft();
}

void GraphicalPrimitiveCanvas::showAll(Canvas & to)
{
	auto const & translatePoint = [this](const Point & input){
		int x = input.x < 0 ? pos.w + input.x : input.x;
		int y = input.y < 0 ? pos.h + input.y : input.y;

		return Point(x,y);
	};

	for (auto const & entry : primitives)
	{
		switch (entry.type)
		{
			case PrimitiveType::LINE:
			{
				to.drawLine(pos.topLeft() + translatePoint(entry.a), pos.topLeft() + translatePoint(entry.b), entry.color, entry.color);
				break;
			}
			case PrimitiveType::FILLED_BOX:
			{
				to.drawColorBlended(Rect(pos.topLeft() + translatePoint(entry.a), translatePoint(entry.b)), entry.color);
				break;
			}
			case PrimitiveType::RECTANGLE:
			{
				to.drawBorder(Rect(pos.topLeft() + translatePoint(entry.a), translatePoint(entry.b)), entry.color);
				break;
			}
		}
	}
}

void GraphicalPrimitiveCanvas::addLine(const Point & from, const Point & to, const ColorRGBA & color)
{
	primitives.push_back({color, from, to, PrimitiveType::LINE});
}

void GraphicalPrimitiveCanvas::addBox(const Point & topLeft, const Point & size, const ColorRGBA & color)
{
	primitives.push_back({color, topLeft, size, PrimitiveType::FILLED_BOX});
}

void GraphicalPrimitiveCanvas::addRectangle(const Point & topLeft, const Point & size, const ColorRGBA & color)
{
	primitives.push_back({color, topLeft, size, PrimitiveType::RECTANGLE});
}

TransparentFilledRectangle::TransparentFilledRectangle(Rect position, ColorRGBA color) :
	GraphicalPrimitiveCanvas(position)
{
	addBox(Point(0,0), Point(-1, -1), color);
}

TransparentFilledRectangle::TransparentFilledRectangle(Rect position, ColorRGBA color, ColorRGBA colorLine, int width) :
	GraphicalPrimitiveCanvas(position)
{
	addBox(Point(0,0), Point(-1, -1), color);
	for (int i = 0; i < width; ++i)
		addRectangle(Point(i,i), Point(-1-i*2, -1-i*2), colorLine);
}

SimpleLine::SimpleLine(Point pos1, Point pos2, ColorRGBA color) :
	GraphicalPrimitiveCanvas(Rect(pos1, pos2 - pos1))
{
	addLine(Point(0,0), Point(-1, -1), color);
}
