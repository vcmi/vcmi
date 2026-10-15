/*
 * UiText.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "UiText.h"

#include "../GameEngine.h"
#include "render/Canvas.h"
#include "render/IFont.h"
#include "render/IRenderHandler.h"

void UiText::draw(Canvas & to, const Point & position, EFonts font, const ColorRGBA & color, ETextAlignment alignment, const std::string & text)
{
	const auto fontPtr = ENGINE->renderHandler().loadFont(font);
	const Point size(static_cast<int>(fontPtr->getStringWidth(text)), static_cast<int>(fontPtr->getLineHeight()));
	Point topLeft = position;

	switch(alignment)
	{
		case ETextAlignment::TOPCENTER:
		case ETextAlignment::CENTER:
		case ETextAlignment::BOTTOMCENTER:
			topLeft.x -= size.x / 2;
			break;
		case ETextAlignment::TOPRIGHT:
		case ETextAlignment::CENTERRIGHT:
		case ETextAlignment::BOTTOMRIGHT:
			topLeft.x -= size.x;
			break;
		default:
			break;
	}

	switch(alignment)
	{
		case ETextAlignment::CENTERLEFT:
		case ETextAlignment::CENTER:
		case ETextAlignment::CENTERRIGHT:
			topLeft.y -= size.y / 2;
			break;
		case ETextAlignment::BOTTOMLEFT:
		case ETextAlignment::BOTTOMCENTER:
		case ETextAlignment::BOTTOMRIGHT:
			topLeft.y -= size.y;
			break;
		default:
			break;
	}

	to.drawText(topLeft, font, color, ETextAlignment::TOPLEFT, text);
}
