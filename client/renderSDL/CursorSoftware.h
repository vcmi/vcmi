/*
 * CursorSoftware.h, part of VCMI engine
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

class CursorSoftware : public ICursor
{
	std::shared_ptr<IImage> cursorImage;

	SDL_Texture * cursorTexture;
	SDL_Surface * cursorSurface;

	Point pos;
	Point pivot;
	bool needUpdate;
	bool visible;

	void createTexture(const Point & dimensions);
	void updateTexture();
public:
	CursorSoftware();
	~CursorSoftware();

	void setImage(std::shared_ptr<IImage> image, const Point & pivotOffset) override;
	void setCursorPosition( const Point & newPos ) override;
	void render() override;
	void setVisible( bool on) override;
};

