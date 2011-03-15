#ifndef __FONTBASE_H__
#define __FONTBASE_H__

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
		si32 unknown1, width, unknown2, offset;
		unsigned char *pixels;
	};

	Char chars[256];
	ui8 height;

	unsigned char *data;


	Font(unsigned char *Data);
	~Font();
	int getWidth(const char *text) const;
	int getCharWidth(char c) const;
};

#endif
