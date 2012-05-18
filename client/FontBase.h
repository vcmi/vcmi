#pragma once

/*
 * FontBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

enum EFonts
{
	FONT_BIG, FONT_CALLI, FONT_CREDITS, FONT_HIGH_SCORE, FONT_MEDIUM, FONT_SMALL, FONT_TIMES, FONT_TINY, FONT_VERD
};

struct Font
{
	struct Char
	{
		si32 leftOffset, width, rightOffset;
		ui8 *pixels;
	};

	Char chars[256];
	ui8 height;

	ui8 *data;

	Font(ui8 *Data);
	~Font();
	int getWidth(const char *text) const;
	int getCharWidth(char c) const;
};
