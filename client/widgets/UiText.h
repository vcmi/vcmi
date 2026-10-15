/*
 * UiText.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/TextAlignment.h"
#include "render/EFont.h"

#include "../../lib/Color.h"
#include "../../lib/Point.h"

class Canvas;

namespace UiText
{
	/// Draws a line of text. Canvas::drawText() ignores the alignments that are not horizontal centers or left and top corners,
	/// this supports all of them. The position is the point of the text that the alignment names
	void draw(Canvas & to, const Point & position, EFonts font, const ColorRGBA & color, ETextAlignment alignment, const std::string & text);
}
