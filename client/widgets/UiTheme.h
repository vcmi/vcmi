/*
 * UiTheme.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "render/EFont.h"
#include "../../lib/Color.h"

/// Sizes and colors shared by the form controls (CTextButton, CCheckBox, CSelectList, ...)
namespace UiTheme
{
	constexpr int CONTROL_HEIGHT = 22;
	constexpr int ROW_HEIGHT = 22;
	constexpr int SPACING = 6;
	constexpr int SLIDER_WIDTH = 16;
	constexpr int CHECK_SIZE = 14;
	constexpr int SCROLL_STEP = 3;

	constexpr EFonts FONT = FONT_SMALL;
	constexpr EFonts FONT_TITLE = FONT_MEDIUM;

	constexpr ColorRGBA TEXT = ColorRGBA(255, 255, 255, 255);
	constexpr ColorRGBA TEXT_TITLE = ColorRGBA(255, 215, 0, 255);
	constexpr ColorRGBA TEXT_HINT = ColorRGBA(100, 160, 240, 255);
	constexpr ColorRGBA TEXT_DISABLED = ColorRGBA(128, 128, 128, 255);
	constexpr ColorRGBA TEXT_SELECTED = ColorRGBA(255, 215, 0, 255);
	constexpr ColorRGBA TEXT_HOVER = ColorRGBA(255, 232, 128, 255);

	constexpr ColorRGBA BORDER = ColorRGBA(32, 96, 192, 255);
	constexpr ColorRGBA BORDER_HIGHLIGHT = ColorRGBA(100, 180, 255, 255);
	constexpr ColorRGBA FIELD_FILL = ColorRGBA(0, 16, 48, 160);
	constexpr ColorRGBA BUTTON_FILL = ColorRGBA(0, 40, 120, 150);
	constexpr ColorRGBA BUTTON_FILL_PRESSED = ColorRGBA(0, 60, 160, 190);
	constexpr ColorRGBA BUTTON_FILL_BLOCKED = ColorRGBA(0, 20, 60, 120);
	constexpr ColorRGBA ROW_SELECTED = ColorRGBA(40, 100, 200, 130);
	constexpr ColorRGBA ROW_HOVER = ColorRGBA(60, 120, 220, 60);
	constexpr ColorRGBA SEPARATOR = ColorRGBA(40, 100, 200, 160);
	constexpr ColorRGBA OVERLAY_DIM = ColorRGBA(0, 0, 0, 96);
}
