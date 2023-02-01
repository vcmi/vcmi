/*
 * ICursor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class CAnimation;
class IImage;
struct SDL_Surface;
struct SDL_Texture;
struct SDL_Cursor;

#include "../../lib/Point.h"

class ICursor
{
public:
	virtual ~ICursor() = default;

	virtual void setImage(std::shared_ptr<IImage> image, const Point & pivotOffset) = 0;
	virtual void setCursorPosition( const Point & newPos ) = 0;
	virtual void render() = 0;
	virtual void setVisible( bool on) = 0;
};

