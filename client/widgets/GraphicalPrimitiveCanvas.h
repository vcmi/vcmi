/*
 * GraphicalPrimitiveCanvas.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class GraphicalPrimitiveCanvas : public CIntObject
{
	enum class PrimitiveType
	{
		LINE,
		RECTANGLE,
		FILLED_BOX
	};

	struct PrimitiveEntry
	{
		ColorRGBA color;
		Point a;
		Point b;
		PrimitiveType type;
	};

	std::vector<PrimitiveEntry> primitives;

	void showAll(Canvas & to) override;

public:
	GraphicalPrimitiveCanvas(Rect position);

	void addLine(const Point & from, const Point & to, const ColorRGBA & color);
	void addBox(const Point & topLeft, const Point & size, const ColorRGBA & color);
	void addRectangle(const Point & topLeft, const Point & size, const ColorRGBA & color);
};

class TransparentFilledRectangle : public GraphicalPrimitiveCanvas
{
public:
	TransparentFilledRectangle(Rect position, ColorRGBA color);
	TransparentFilledRectangle(Rect position, ColorRGBA color, ColorRGBA colorLine, int width = 1);
};

class SimpleLine : public GraphicalPrimitiveCanvas
{
public:
	SimpleLine(Point pos1, Point pos2, ColorRGBA color);
};
