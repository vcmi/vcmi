/*
 * CWindowObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CWindowObject.h"

#include "../widgets/MiscWidgets.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../battle/BattleInterface.h"
#include "../battle/BattleInterfaceClasses.h"
#include "../windows/CMessage.h"
#include "../renderSDL/SDL_PixelAccess.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/Canvas.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CMusicHandler.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h" //for Unicode related stuff

#include <SDL_surface.h>

CWindowObject::CWindowObject(int options_, const ImagePath & imageName, Point centerAt):
	WindowBase(0, Point()),
	options(options_),
	background(createBg(imageName, options & PLAYER_COLORED))
{
	if(!(options & NEEDS_ANIMATED_BACKGROUND)) //currently workaround for highscores (currently uses window as normal control, because otherwise videos are not played in background yet)
		assert(parent == nullptr); //Safe to remove, but windows should not have parent

	defActions = 255-DISPOSE;

	if (options & RCLICK_POPUP)
		CCS->curh->hide();

	if (background)
		pos = background->center(centerAt);
	else
		center(centerAt);

	if (!(options & SHADOW_DISABLED))
		setShadow(true);
}

CWindowObject::CWindowObject(int options_, const ImagePath & imageName):
	WindowBase(0, Point()),
	options(options_),
	background(createBg(imageName, options_ & PLAYER_COLORED))
{
	if(!(options & NEEDS_ANIMATED_BACKGROUND)) //currently workaround for highscores (currently uses window as normal control, because otherwise videos are not played in background yet)
		assert(parent == nullptr); //Safe to remove, but windows should not have parent

	defActions = 255-DISPOSE;

	if(options & RCLICK_POPUP)
		CCS->curh->hide();

	if(background)
		pos = background->center();
	else
		center(GH.screenDimensions() / 2);

	if(!(options & SHADOW_DISABLED))
		setShadow(true);
}

CWindowObject::~CWindowObject()
{
	if(options & RCLICK_POPUP)
		CCS->curh->show();
}

std::shared_ptr<CPicture> CWindowObject::createBg(const ImagePath & imageName, bool playerColored)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	if(imageName.empty())
		return nullptr;

	auto image = std::make_shared<CPicture>(imageName);
	image->getSurface()->setBlitMode(EImageBlitMode::OPAQUE);
	if(playerColored)
		image->colorize(LOCPLINT->playerID);
	return image;
}

void CWindowObject::setBackground(const ImagePath & filename)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

	background = createBg(filename, options & PLAYER_COLORED);

	if(background)
		pos = background->center(Point(pos.w/2 + pos.x, pos.h/2 + pos.y));

	updateShadow();
}

void CWindowObject::updateShadow()
{
	setShadow(false);
	if (!(options & SHADOW_DISABLED))
		setShadow(true);
}

void CWindowObject::setShadow(bool on)
{
	//size of shadow
	static const int size = 8;

	if(on == !shadowParts.empty())
		return;

	shadowParts.clear();

	//object too small to cast shadow
	if(pos.h <= size || pos.w <= size)
		return;

	if(on)
	{

		//helper to set last row
		auto blitAlphaRow = [](SDL_Surface *surf, size_t row)
		{
			uint8_t * ptr = (uint8_t*)surf->pixels + surf->pitch * (row);

			for (size_t i=0; i< surf->w; i++)
			{
				Channels::px<4>::a.set(ptr, 128);
				ptr+=4;
			}
		};

		// helper to set last column
		auto blitAlphaCol = [](SDL_Surface *surf, size_t col)
		{
			uint8_t * ptr = (uint8_t*)surf->pixels + 4 * (col);

			for (size_t i=0; i< surf->h; i++)
			{
				Channels::px<4>::a.set(ptr, 128);
				ptr+= surf->pitch;
			}
		};

		static SDL_Surface * shadowCornerTempl = nullptr;
		static SDL_Surface * shadowBottomTempl = nullptr;
		static SDL_Surface * shadowRightTempl = nullptr;

		//one-time initialization
		if(!shadowCornerTempl)
		{
			//create "template" surfaces
			shadowCornerTempl = CSDL_Ext::createSurfaceWithBpp<4>(size, size);
			shadowBottomTempl = CSDL_Ext::createSurfaceWithBpp<4>(1, size);
			shadowRightTempl  = CSDL_Ext::createSurfaceWithBpp<4>(size, 1);

			//fill with shadow body color
			CSDL_Ext::fillSurface(shadowCornerTempl, { 0, 0, 0, 192 } );
			CSDL_Ext::fillSurface(shadowBottomTempl, { 0, 0, 0, 192 } );
			CSDL_Ext::fillSurface(shadowRightTempl,  { 0, 0, 0, 192 } );

			//fill last row and column with more transparent color
			blitAlphaCol(shadowRightTempl , size-1);
			blitAlphaCol(shadowCornerTempl, size-1);
			blitAlphaRow(shadowBottomTempl, size-1);
			blitAlphaRow(shadowCornerTempl, size-1);
		}

		//FIXME: do something with this points
		Point shadowStart;
		if (options & BORDERED)
			shadowStart = Point(size - 14, size - 14);
		else
			shadowStart = Point(size, size);

		Point shadowPos;
		if (options & BORDERED)
			shadowPos = Point(pos.w + 14, pos.h + 14);
		else
			shadowPos = Point(pos.w, pos.h);

		Point fullsize;
		if (options & BORDERED)
			fullsize = Point(pos.w + 28, pos.h + 29);
		else
			fullsize = Point(pos.w, pos.h);

		//create base 8x8 piece of shadow
		SDL_Surface * shadowCorner = CSDL_Ext::copySurface(shadowCornerTempl);
		SDL_Surface * shadowBottom = CSDL_Ext::scaleSurfaceFast(shadowBottomTempl, fullsize.x - size, size);
		SDL_Surface * shadowRight  = CSDL_Ext::scaleSurfaceFast(shadowRightTempl,  size, fullsize.y - size);

		blitAlphaCol(shadowBottom, 0);
		blitAlphaRow(shadowRight, 0);

		//generate "shadow" object with these 3 pieces in it
		{
			OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

			shadowParts.push_back(std::make_shared<CPicture>( GH.renderHandler().createImage(shadowCorner), Point(shadowPos.x,   shadowPos.y)));
			shadowParts.push_back(std::make_shared<CPicture>( GH.renderHandler().createImage(shadowRight ),  Point(shadowPos.x,   shadowStart.y)));
			shadowParts.push_back(std::make_shared<CPicture>( GH.renderHandler().createImage(shadowBottom), Point(shadowStart.x, shadowPos.y)));

		}
		SDL_FreeSurface(shadowCorner);
		SDL_FreeSurface(shadowBottom);
		SDL_FreeSurface(shadowRight);
	}
}

void CWindowObject::showAll(Canvas & to)
{
	auto color = LOCPLINT ? LOCPLINT->playerID : PlayerColor(1);
	if(settings["session"]["spectate"].Bool())
		color = PlayerColor(1); // TODO: Spectator shouldn't need special code for UI colors

	CIntObject::showAll(to);
	if ((options & BORDERED) && (pos.dimensions() != GH.screenDimensions()))
		CMessage::drawBorder(color, to.getInternalSurface(), pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

bool CWindowObject::isPopupWindow() const
{
	return options & RCLICK_POPUP;
}

CStatusbarWindow::CStatusbarWindow(int options, const ImagePath & imageName, Point centerAt) : CWindowObject(options, imageName, centerAt)
{
}

CStatusbarWindow::CStatusbarWindow(int options, const ImagePath & imageName) : CWindowObject(options, imageName)
{
}
