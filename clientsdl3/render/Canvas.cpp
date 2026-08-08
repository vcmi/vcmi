/*
 * Canvas.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Canvas.h"

#include "GameEngine.h"
#include "../media/IVideoPlayer.h"
#include "render/IRenderHandler.h"
#include "IScreenHandler.h"
#include "SDL_Extensions.h"
#include "render/Colors.h"
#include "IImage.h"
#include "render/Graphics.h"
#include "render/IFont.h"

#include "CMT.h"

#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>

/// Reports a problem in the GPU render path once per distinct message, so that a failure
/// happening every frame cannot flood the log
static void logGpuIssueOnce(const std::string & message)
{
	// the failing draw may come from any thread, so the guard itself has to be safe
	static std::mutex mutex;
	static std::set<std::string> reported;

	std::lock_guard lock(mutex);
	if(reported.insert(message).second)
		logGlobal->error("GPU render path: %s", message);
}

Canvas::Canvas(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy):
	scalingPolicy(scalingPolicy),
	surface(surface),
	renderArea(0,0, surface->w, surface->h)
{
	surface->refcount++;
}

Canvas::Canvas(const Canvas & other):
	scalingPolicy(other.scalingPolicy),
	surface(other.surface),
	renderTarget(other.renderTarget),
	renderArea(other.renderArea)
{
	if(surface)
		surface->refcount++;
}

Canvas::Canvas(Canvas && other):
	scalingPolicy(other.scalingPolicy),
	surface(other.surface),
	renderTarget(other.renderTarget),
	ownsRenderTarget(other.ownsRenderTarget),
	renderArea(other.renderArea)
{
	// ownership of a render target moves; the source must not destroy it any more
	other.ownsRenderTarget = false;

	if(surface)
		surface->refcount++;
}

Canvas::Canvas(const Canvas & other, const Rect & newClipRect):
	Canvas(other)
{
	Rect scaledClipRect( transformPos(newClipRect.topLeft()), transformPos(newClipRect.dimensions()));
	renderArea = other.renderArea.intersect(scaledClipRect + other.renderArea.topLeft());
}

Canvas::Canvas(const Point & size, CanvasScalingPolicy scalingPolicy):
	scalingPolicy(scalingPolicy),
	surface(CSDL_Ext::newSurface(size * getScalingFactor())),
	renderArea(Point(0,0), size * getScalingFactor())
{
	CSDL_Ext::fillSurface(surface, CSDL_Ext::toSDL(Colors::TRANSPARENCY) );
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}

int Canvas::getScalingFactor() const
{
	if (scalingPolicy == CanvasScalingPolicy::IGNORE)
		return 1;
	return ENGINE->screenHandler().getScalingFactor();
}

Point Canvas::transformPos(const Point & input)
{
	return renderArea.topLeft() + input * getScalingFactor();
}

Point Canvas::transformSize(const Point & input)
{
	return input * getScalingFactor();
}

Canvas::Canvas(SDL_Texture * renderTarget, const Point & size, CanvasScalingPolicy scalingPolicy, bool ownsRenderTarget):
	scalingPolicy(scalingPolicy),
	surface(nullptr),
	renderTarget(renderTarget),
	ownsRenderTarget(ownsRenderTarget),
	renderArea(Point(0, 0), size * getScalingFactor())
{
}

Canvas Canvas::createFromSurface(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy)
{
	return Canvas(surface, scalingPolicy);
}

Canvas Canvas::createFromRenderTarget(SDL_Texture * renderTarget, const Point & size, CanvasScalingPolicy scalingPolicy)
{
	return Canvas(renderTarget, size, scalingPolicy);
}

Canvas Canvas::createOwningRenderTarget(SDL_Texture * renderTarget, const Point & size, CanvasScalingPolicy scalingPolicy)
{
	return Canvas(renderTarget, size, scalingPolicy, true);
}

void Canvas::bindRenderTarget() const
{
	// switching targets flushes the batch, so only do it when it actually changes
	if(SDL_GetRenderTarget(mainRenderer) != renderTarget)
		if(!SDL_SetRenderTarget(mainRenderer, renderTarget))
			logGpuIssueOnce(std::string("SDL_SetRenderTarget failed: ") + SDL_GetError());
}

void Canvas::copyFromCanvas(const Canvas & image, const Rect & targetArea, uint32_t blendMode, uint8_t alpha)
{
	if(!image.renderTarget)
	{
		logGpuIssueOnce("cannot copy a surface-backed canvas onto a GPU target");
		return;
	}

	bindRenderTarget();

	SDL_FRect source = CSDL_Ext::toSDLFloat(image.renderArea);
	SDL_FRect target = CSDL_Ext::toSDLFloat(targetArea);

	SDL_SetTextureBlendMode(image.renderTarget, static_cast<SDL_BlendMode>(blendMode));
	SDL_SetTextureAlphaMod(image.renderTarget, alpha);
	// matches SDL_BlitSurfaceScaled on the surface path; see the note in SDLImageShared
	SDL_SetTextureScaleMode(image.renderTarget, SDL_SCALEMODE_NEAREST);

	if(!SDL_RenderTexture(mainRenderer, image.renderTarget, &source, &target))
		logGpuIssueOnce(std::string("SDL_RenderTexture failed: ") + SDL_GetError());

	SDL_SetTextureAlphaMod(image.renderTarget, SDL_ALPHA_OPAQUE);
}

void Canvas::applyTransparency(bool on)
{
	if (on)
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}

void Canvas::applyGrayscale()
{
	if(renderTarget)
		return; // no GPU equivalent; only the puzzle map needs this and it stays software

	CSDL_Ext::convertToGrayscale(surface, renderArea);
}

Canvas::~Canvas()
{
	if(surface)
		SDL_DestroySurface(surface);

	if(ownsRenderTarget && renderTarget)
		SDL_DestroyTexture(renderTarget);
}

void Canvas::draw(IVideoInstance & video, const Point & pos)
{
	video.show(pos, surface);
}

void Canvas::draw(const IImage& image, const Point & pos)
{
	if(renderTarget)
	{
		bindRenderTarget();
		if(!image.drawTexture(mainRenderer, transformPos(pos), nullptr, getScalingFactor()))
			logGpuIssueOnce("image reference has no texture representation");
		return;
	}

	image.draw(surface, transformPos(pos), nullptr, getScalingFactor());
}

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos)
{
	assert(image);
	if (!image)
		return;

	if(renderTarget)
	{
		bindRenderTarget();
		if(!image->drawTexture(mainRenderer, transformPos(pos), nullptr, getScalingFactor()))
			logGpuIssueOnce("image has no texture representation");
		return;
	}

	image->draw(surface, transformPos(pos), nullptr, getScalingFactor());
}

void Canvas::draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect)
{
	Rect realSourceRect = sourceRect * getScalingFactor();
	assert(image);
	if (!image)
		return;

	if(renderTarget)
	{
		bindRenderTarget();
		if(!image->drawTexture(mainRenderer, transformPos(pos), &realSourceRect, getScalingFactor()))
			logGpuIssueOnce("image has no texture representation (subrect)");
		return;
	}

	image->draw(surface, transformPos(pos), &realSourceRect, getScalingFactor());
}

void Canvas::draw(const Canvas & image, const Point & pos)
{
	if(renderTarget)
	{
		copyFromCanvas(image, Rect(transformPos(pos), image.renderArea.dimensions()), SDL_BLENDMODE_NONE, SDL_ALPHA_OPAQUE);
		return;
	}

	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, transformPos(pos));
}

void Canvas::drawTransparent(const Canvas & image, const Point & pos, double transparency)
{
	if(renderTarget)
	{
		copyFromCanvas(image, Rect(transformPos(pos), image.renderArea.dimensions()), SDL_BLENDMODE_BLEND, 255 * transparency);
		return;
	}

	SDL_BlendMode oldMode;

	SDL_GetSurfaceBlendMode(image.surface, &oldMode);
	SDL_SetSurfaceBlendMode(image.surface, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(image.surface, 255 * transparency);
	CSDL_Ext::blitSurface(image.surface, image.renderArea, surface, transformPos(pos));
	SDL_SetSurfaceAlphaMod(image.surface, 255);
	SDL_SetSurfaceBlendMode(image.surface, oldMode);
}

void Canvas::drawScaled(const Canvas & image, const Point & pos, const Point & targetSize)
{
	// SDL_BlitSurfaceScaled is a software scaler - expensive on large areas.
	Rect targetArea(transformPos(pos), transformSize(targetSize));

	if(renderTarget)
	{
		copyFromCanvas(image, targetArea, SDL_BLENDMODE_NONE, SDL_ALPHA_OPAQUE);
		return;
	}

	SDL_Rect targetRect = CSDL_Ext::toSDL(targetArea);
	SDL_BlitSurfaceScaled(image.surface, nullptr, surface, &targetRect, SDL_SCALEMODE_NEAREST);
}

void Canvas::drawPoint(const Point & dest, const ColorRGBA & color)
{
	Point point = transformPos(dest);
	CSDL_Ext::putPixelWithoutRefreshIfInSurf(surface, point.x, point.y, color.r, color.g, color.b, color.a);
}

void Canvas::drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest)
{
	CSDL_Ext::drawLine(surface, transformPos(from), transformPos(dest), CSDL_Ext::toSDL(colorFrom), CSDL_Ext::toSDL(colorDest), getScalingFactor());
}

void Canvas::drawBorder(const Rect & target, const ColorRGBA & color, int width)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::drawBorder(surface, realTarget.x, realTarget.y, realTarget.w, realTarget.h, CSDL_Ext::toSDL(color), width * getScalingFactor());
}

void Canvas::drawBorderDashed(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.topRight(),    CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.bottomLeft(), realTarget.bottomRight(), CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.bottomLeft(),  CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topRight(),   realTarget.bottomRight(), CSDL_Ext::toSDL(color));
}

void Canvas::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text )
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return fontPtr->renderTextLeft  (surface, text, colorDest, transformPos(position));
	case ETextAlignment::TOPCENTER:    return fontPtr->renderTextCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::CENTER:       return fontPtr->renderTextCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::BOTTOMRIGHT:  return fontPtr->renderTextRight (surface, text, colorDest, transformPos(position));
	}
}

void Canvas::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::vector<std::string> & text )
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return fontPtr->renderTextLinesLeft  (surface, text, colorDest, transformPos(position));
	case ETextAlignment::TOPCENTER:    return fontPtr->renderTextLinesCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::CENTER:       return fontPtr->renderTextLinesCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::BOTTOMRIGHT:  return fontPtr->renderTextLinesRight (surface, text, colorDest, transformPos(position));
	}
}

void Canvas::drawColor(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	if(renderTarget)
	{
		bindRenderTarget();
		SDL_FRect rect = CSDL_Ext::toSDLFloat(realTarget);
		SDL_SetRenderDrawBlendMode(mainRenderer, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(mainRenderer, color.r, color.g, color.b, color.a);
		SDL_RenderFillRect(mainRenderer, &rect);
		return;
	}

	CSDL_Ext::fillRect(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::drawColorBlended(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::fillRectBlended(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::fillTexture(const std::shared_ptr<IImage>& image)
{
	assert(image);
	if (!image)
		return;
		
	Rect imageArea(Point(0, 0), image->dimensions());
	for (int y=0; y < surface->h; y+= imageArea.h)
	{
		for (int x=0; x < surface->w; x+= imageArea.w)
			image->draw(surface, Point(renderArea.x + x * getScalingFactor(), renderArea.y + y * getScalingFactor()), nullptr, getScalingFactor());
	}
}

Rect Canvas::getRenderArea() const
{
	return renderArea;
}

ColorRGBA Canvas::getPixel(const Point & position) const
{
	SDL_Color color = CSDL_Ext::getColor(surface, CSDL_Ext::getPixel(surface, position.x, position.y));
	return ColorRGBA(color.r, color.g, color.b, color.a);
}

CanvasClipRectGuard::CanvasClipRectGuard(Canvas & canvas, const Rect & rect): surf(canvas.surface)
{
	CSDL_Ext::getClipRect(surf, oldRect);
	const Rect scaled = rect * ENGINE->screenHandler().getScalingFactor();
	CSDL_Ext::setClipRect(surf, oldRect.intersect(scaled));
}

CanvasClipRectGuard::~CanvasClipRectGuard()
{
	CSDL_Ext::setClipRect(surf, oldRect);
}
