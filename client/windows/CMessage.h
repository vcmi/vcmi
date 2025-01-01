/*
 * CMessage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/EFont.h"
#include "../../lib/GameConstants.h"

struct SDL_Surface;
class CInfoWindow;
class CComponent;
class Canvas;

VCMI_LIB_NAMESPACE_BEGIN
class ColorRGBA;
VCMI_LIB_NAMESPACE_END

/// Class which draws formatted text messages and generates chat windows
class CMessage
{
	/// Draw simple dialog box (borders and background only)
	static SDL_Surface * drawDialogBox(int w, int h, PlayerColor playerColor = PlayerColor(1));

public:
	struct coloredline
	{
		std::string_view line;
		std::string_view startColorTag;
		bool closingTagNeeded;

		coloredline(std::string_view line, std::string_view startColorTag) 
			: line(line)
			, startColorTag(startColorTag)
			, closingTagNeeded(false)
		{}
	};

	/// Draw border on exiting surface
	static void drawBorder(PlayerColor playerColor, Canvas & to, int w, int h, int x, int y);

	static void drawIWindow(CInfoWindow * ret, std::string text, PlayerColor player);
	static bool validateTags(
		const std::vector<std::string_view::const_iterator> & openingTags,
		const std::vector<std::string_view::const_iterator> & closingTags);
	static std::vector<coloredline> splitLineBySpaces(const std::string_view & line, size_t maxLineWidth, const EFonts font);

	/// split text in lines
	static std::vector<std::string> breakText(const std::string_view & text, size_t maxLineWidth, const EFonts font);

	/// Try to guess a header of a message
	static std::string guessHeader(const std::string & msg);

	/// For convenience
	static int guessHeight(const std::string & string, int width, EFonts fnt);

	static int getEstimatedComponentHeight(int numComps);
	/// constructor
	static void init();
	/// destructor
	static void dispose();
};
