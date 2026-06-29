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
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/CursorHandler.h"
#include "../battle/BattleInterface.h"
#include "../windows/CMessage.h"
#include "../renderSDL/SDL_PixelAccess.h"
#include "../render/IImage.h"
#include "../render/IScreenHandler.h"
#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/CanvasImage.h"

#include "../CPlayerInterface.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h" //for Unicode related stuff

#include <SDL_surface.h>

CWindowObject::CWindowObject(int options_, const ImagePath & imageName, Point centerAt):
	WindowBase(0, Point()),
	options(options_),
	background(createBg(imageName, options_))
{
	if(!(options & NEEDS_ANIMATED_BACKGROUND)) //currently workaround for highscores (currently uses window as normal control, because otherwise videos are not played in background yet)
		assert(parent == nullptr); //Safe to remove, but windows should not have parent

	if (options & RCLICK_POPUP)
		ENGINE->cursor().hide();

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
	background(createBg(imageName, options_))
{
	if(!(options & NEEDS_ANIMATED_BACKGROUND)) //currently workaround for highscores (currently uses window as normal control, because otherwise videos are not played in background yet)
		assert(parent == nullptr); //Safe to remove, but windows should not have parent

	if(options & RCLICK_POPUP)
		ENGINE->cursor().hide();

	if(background)
		pos = background->center();
	else
		center(ENGINE->screenDimensions() / 2);

	if(!(options & SHADOW_DISABLED))
		setShadow(true);
}

CWindowObject::~CWindowObject()
{
	if(options & RCLICK_POPUP)
		ENGINE->cursor().show();
}

std::shared_ptr<CPicture> CWindowObject::createBg(const ImagePath & imageName, int windowOptions)
{
	OBJECT_CONSTRUCTION;

	if(imageName.empty())
		return nullptr;

	auto image = std::make_shared<CPicture>(imageName, Point(0,0), EImageBlitMode::OPAQUE);
	PlayerColor playerColor = GAME->interface() ? GAME->interface()->playerID : PlayerColor(1);

	if(windowOptions & PLAYER_COLORED_BORDERED_STATUSBAR)
	{
		return createPlayerColoredBorderedStatusbar(image, playerColor);
	}
	if(windowOptions & PLAYER_COLORED)
	{
		image->setPlayerColor(playerColor);
	}

	return image;
}

std::shared_ptr<CPicture> CWindowObject::createPlayerColoredBorderedStatusbar(const std::shared_ptr<CPicture> & image, PlayerColor playerColor)
{
	auto composited = ENGINE->renderHandler().createImage(image->getSurface()->dimensions(), CanvasScalingPolicy::AUTO);
	Canvas canvas = composited->getCanvas();
	canvas.draw(image->getSurface(), Point(0, 0));

	auto dialogBox = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("DIALGBOX"), EImageBlitMode::COLORKEY);
	if(playerColor.isValidPlayer() && playerColor != PlayerColor(1))
		dialogBox->playerColored(playerColor);

	const int width = composited->width();
	const int height = composited->height();

	auto drawHorizontal = [&canvas](const std::shared_ptr<IImage> & source, int y, int xBegin, int xEnd)
	{
		for(int x = xBegin; x < xEnd; x += source->width())
		{
			int blitWidth = std::min(source->width(), xEnd - x);
			canvas.draw(source, Point(x, y), Rect(0, 0, blitWidth, source->height()));
		}
	};
	auto drawVertical = [&canvas](const std::shared_ptr<IImage> & source, int x, int yBegin, int yEnd)
	{
		for(int y = yBegin; y < yEnd; y += source->height())
		{
			int blitHeight = std::min(source->height(), yEnd - y);
			canvas.draw(source, Point(x, y), Rect(0, 0, source->width(), blitHeight));
		}
	};

	auto topLeft = dialogBox->getImage(0, 0);
	auto topRight = dialogBox->getImage(1, 0);
	auto leftEdge = dialogBox->getImage(4, 0);
	auto rightEdge = dialogBox->getImage(5, 0);
	auto topEdge = dialogBox->getImage(6, 0);
	auto bottomLeft = dialogBox->getImage(8, 0);
	auto bottomRight = dialogBox->getImage(9, 0);
	auto bottomEdge = dialogBox->getImage(10, 0);

	drawHorizontal(topEdge, 0, topLeft->width(), width - topRight->width());
	drawHorizontal(bottomEdge, height - bottomEdge->height(), bottomLeft->width(), width - bottomRight->width());
	drawVertical(leftEdge, 0, topLeft->height(), height - bottomLeft->height());
	drawVertical(rightEdge, width - rightEdge->width(), topRight->height(), height - bottomRight->height());

	canvas.draw(topLeft, Point(0, 0));
	canvas.draw(topRight, Point(width - topRight->width(), 0));
	canvas.draw(bottomLeft, Point(0, height - bottomLeft->height()));
	canvas.draw(bottomRight, Point(width - bottomRight->width(), height - bottomRight->height()));

	return std::make_shared<CPicture>(composited, Point(0,0));
}

void CWindowObject::setBackground(const ImagePath & filename)
{
	OBJECT_CONSTRUCTION;

	background = createBg(filename, options);

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
	int size = 8;

	if(on == !shadowParts.empty())
		return;

	shadowParts.clear();

	//object too small to cast shadow
	if(pos.h <= size || pos.w <= size)
		return;

	if(on)
	{
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

		Point sizeCorner(size, size);
		Point sizeRight(fullsize.x - size, size);
		Point sizeBottom(size, fullsize.y - size);

		//create base 8x8 piece of shadow
		auto imageCorner = ENGINE->renderHandler().createImage(sizeCorner, CanvasScalingPolicy::AUTO);
		auto imageRight  = ENGINE->renderHandler().createImage(sizeRight,  CanvasScalingPolicy::AUTO);
		auto imageBottom = ENGINE->renderHandler().createImage(sizeBottom, CanvasScalingPolicy::AUTO);

		Canvas canvasCorner = imageCorner->getCanvas();
		Canvas canvasRight = imageRight->getCanvas();
		Canvas canvasBottom = imageBottom->getCanvas();

		canvasCorner.drawColor(Rect(Point(0,0), sizeCorner), { 0, 0, 0, 128 });
		canvasRight.drawColor(Rect(Point(0,0), sizeRight), { 0, 0, 0, 128 });
		canvasBottom.drawColor(Rect(Point(0,0), sizeBottom), { 0, 0, 0, 128 });

		canvasCorner.drawColor(Rect(Point(0,0), sizeCorner - Point(1,1)), { 0, 0, 0, 192 });
		canvasRight.drawColor(Rect(Point(0,0),   sizeRight - Point(0,1)), { 0, 0, 0, 192 });
		canvasBottom.drawColor(Rect(Point(0,0), sizeBottom - Point(1,0)), { 0, 0, 0, 192 });

		//generate "shadow" object with these 3 pieces in it
		{
			OBJECT_CONSTRUCTION;

			shadowParts.push_back(std::make_shared<CPicture>( imageCorner, Point(shadowPos.x,   shadowPos.y)));
			shadowParts.push_back(std::make_shared<CPicture>( imageRight, Point(shadowStart.x, shadowPos.y)));
			shadowParts.push_back(std::make_shared<CPicture>( imageBottom,  Point(shadowPos.x,   shadowStart.y)));

		}
	}
}

void CWindowObject::showAll(Canvas & to)
{
	auto color = GAME->interface() ? GAME->interface()->playerID : PlayerColor(1);
	if(settings["session"]["spectate"].Bool())
		color = PlayerColor(1); // TODO: Spectator shouldn't need special code for UI colors

	CIntObject::showAll(to);
	if ((options & BORDERED) && (pos.dimensions() != ENGINE->screenDimensions()))
		CMessage::drawBorder(color, to, pos.w+28, pos.h+29, pos.x-14, pos.y-15);
}

bool CWindowObject::isPopupWindow() const
{
	return options & RCLICK_POPUP;
}
