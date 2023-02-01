/*
 * CCursorHandler.h, part of VCMI engine
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

class CursorHardware : public ICursor
{
	std::shared_ptr<IImage> cursorImage;

	SDL_Cursor * cursor;

public:
	CursorHardware();
	~CursorHardware();

	void setImage(std::shared_ptr<IImage> image, const Point & pivotOffset) override;
	void setCursorPosition( const Point & newPos ) override;
	void render() override;
	void setVisible( bool on) override;
};

