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
#include "GpuResources.h"

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
#include "render/SDLImage.h"
#include "render/TextTextureCache.h"

#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>

/// Reports a problem in the GPU render path once per distinct message, so that a failure
/// happening every frame cannot flood the log. All draw calls run on the main thread under
/// the interface mutex, so this needs no locking of its own.
static void logGpuIssueOnce(const std::string & message)
{
	static std::set<std::string> reported;

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
	renderTargetGeneration(other.renderTargetGeneration),
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
	renderTargetGeneration(GpuResources::get().generation()),
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

/// Uploads one region of a surface into the shared texture, which then holds it at its origin.
/// Returns nullptr when the surface is not in the format our canvases use.
static SDL_Texture * uploadSurfaceRegion(SDL_Surface * surface, const Rect & area)
{
	if(surface->format != SDL_PIXELFORMAT_ARGB8888 || area.w <= 0 || area.h <= 0)
		return nullptr;

	SDL_Texture * texture = GpuResources::get().acquireUploadTexture(area.dimensions());

	if(!texture)
		return nullptr;

	static constexpr int bytesPerPixel = 4;
	const SDL_Rect destination{0, 0, area.w, area.h};
	const auto * pixels = static_cast<const uint8_t *>(surface->pixels) + static_cast<size_t>(area.y) * surface->pitch + static_cast<size_t>(area.x) * bytesPerPixel;

	if(!SDL_UpdateTexture(texture, &destination, pixels, surface->pitch))
		return nullptr;

	return texture;
}

void Canvas::bindRenderTarget() const
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	// switching targets flushes the batch, so only do it when it actually changes
	if(SDL_GetRenderTarget(renderer) != renderTarget)
		if(!SDL_SetRenderTarget(renderer, renderTarget))
			logGpuIssueOnce(std::string("SDL_SetRenderTarget failed: ") + SDL_GetError());
}

void Canvas::copyFromCanvas(const Canvas & image, const Rect & targetArea, uint32_t blendMode, uint8_t alpha)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	bindRenderTarget();

	SDL_FRect source = CSDL_Ext::toSDLFloat(image.renderArea);
	SDL_FRect target = CSDL_Ext::toSDLFloat(targetArea);

	// A surface-backed source has no texture of its own - upload it once for this copy, so
	// windows composing into plain surfaces can be drawn onto a GPU target at all.
	if(!image.renderTarget)
	{
		SDL_Texture * shared = uploadSurfaceRegion(image.surface, image.renderArea);

		// an unexpected surface format still has to go through a conversion of its own
		SDL_Texture * uploaded = shared ? shared : SDL_CreateTextureFromSurface(renderer, image.surface);

		if(!uploaded)
		{
			logGpuIssueOnce(std::string("failed to upload a surface-backed canvas: ") + SDL_GetError());
			return;
		}

		// the shared texture received only the requested region, placed at its origin
		if(shared)
			source = CSDL_Ext::toSDLFloat(Rect(Point(0, 0), image.renderArea.dimensions()));

		SDL_SetTextureBlendMode(uploaded, static_cast<SDL_BlendMode>(blendMode));
		SDL_SetTextureAlphaMod(uploaded, alpha);
		SDL_SetTextureScaleMode(uploaded, SDL_SCALEMODE_NEAREST);

		if(!SDL_RenderTexture(renderer, uploaded, &source, &target))
			logGpuIssueOnce(std::string("SDL_RenderTexture failed for uploaded surface: ") + SDL_GetError());

		if(!shared)
			SDL_DestroyTexture(uploaded);

		return;
	}

	SDL_SetTextureBlendMode(image.renderTarget, static_cast<SDL_BlendMode>(blendMode));
	SDL_SetTextureAlphaMod(image.renderTarget, alpha);
	// matches SDL_BlitSurfaceScaled on the surface path; see the note in SDLImageShared
	SDL_SetTextureScaleMode(image.renderTarget, SDL_SCALEMODE_NEAREST);

	if(!SDL_RenderTexture(renderer, image.renderTarget, &source, &target))
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

	// owners are released on whichever thread drops them, and a texture may only be
	// destroyed on the rendering thread
	if(ownsRenderTarget && renderTarget && renderTargetGeneration == GpuResources::get().generation())
		GpuResources::get().destroyTextureDeferred(renderTarget);
}

void Canvas::drawViaScratchSurface(const Point & pos, const Point & size, const std::function<void(SDL_Surface *)> & render)
{
	if(size.x <= 0 || size.y <= 0)
		return;

	Canvas scratch(size, scalingPolicy);
	render(scratch.surface);

	// copyFromCanvas uploads a surface-backed source for us
	copyFromCanvas(scratch, Rect(transformPos(pos), scratch.renderArea.dimensions()), SDL_BLENDMODE_BLEND, SDL_ALPHA_OPAQUE);
}

void Canvas::draw(IVideoInstance & video, const Point & pos)
{
	if(renderTarget)
	{
		bindRenderTarget();

		// only a video opened while the GPU path was active decodes into a texture
		if(!video.renderFrame(transformPos(pos)))
			drawViaScratchSurface(pos, video.size(), [&video](SDL_Surface * target){ video.show(Point(0, 0), target); });
		return;
	}

	video.show(pos, surface);
}

void Canvas::draw(const IImage& image, const Point & pos)
{
	if(renderTarget)
	{
		bindRenderTarget();

		// an image that is still upscaling, or that the driver refused, has no texture yet
		if(!image.drawTexture(GpuResources::get().renderer(), transformPos(pos), nullptr, getScalingFactor()))
			drawViaScratchSurface(pos, image.dimensions(), [&](SDL_Surface * target){ image.draw(target, Point(0, 0), nullptr, getScalingFactor()); });
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
		if(!image->drawTexture(GpuResources::get().renderer(), transformPos(pos), nullptr, getScalingFactor()))
			drawViaScratchSurface(pos, image->dimensions(), [&](SDL_Surface * target){ image->draw(target, Point(0, 0), nullptr, getScalingFactor()); });
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
		if(!image->drawTexture(GpuResources::get().renderer(), transformPos(pos), &realSourceRect, getScalingFactor()))
			drawViaScratchSurface(pos, sourceRect.dimensions(), [&](SDL_Surface * target){ image->draw(target, Point(0, 0), &realSourceRect, getScalingFactor()); });
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
	SDL_Renderer * renderer = GpuResources::get().renderer();

	Point point = transformPos(dest);

	if(renderTarget)
	{
		bindRenderTarget();
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderPoint(renderer, point.x, point.y);
		return;
	}
	CSDL_Ext::putPixelWithoutRefreshIfInSurf(surface, point.x, point.y, color.r, color.g, color.b, color.a);
}

void Canvas::drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	if(renderTarget)
	{
		bindRenderTarget();
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

		const Point start = transformPos(from);
		const Point end = transformPos(dest);

		// SDL draws a line in a single color, so approximate the gradient with segments -
		// these are hairlines, so the banding is not visible
		static constexpr int segments = 16;
		for(int i = 0; i < segments; ++i)
		{
			const auto blend = [i](uint8_t a, uint8_t b){ return static_cast<uint8_t>(a + (b - a) * i / (segments - 1)); };

			Point segmentFrom = start + (end - start) * i / segments;
			Point segmentTo = start + (end - start) * (i + 1) / segments;

			SDL_SetRenderDrawColor(renderer, blend(colorFrom.r, colorDest.r), blend(colorFrom.g, colorDest.g), blend(colorFrom.b, colorDest.b), blend(colorFrom.a, colorDest.a));
			SDL_RenderLine(renderer, segmentFrom.x, segmentFrom.y, segmentTo.x, segmentTo.y);
		}
		return;
	}

	CSDL_Ext::drawLine(surface, transformPos(from), transformPos(dest), CSDL_Ext::toSDL(colorFrom), CSDL_Ext::toSDL(colorDest), getScalingFactor());
}

void Canvas::drawBorder(const Rect & target, const ColorRGBA & color, int width)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	if(renderTarget)
	{
		bindRenderTarget();
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

		for(int i = 0; i < width * getScalingFactor(); ++i)
		{
			SDL_FRect ring = CSDL_Ext::toSDLFloat(realTarget.resize(-i));
			SDL_RenderRect(renderer, &ring);
		}
		return;
	}

	CSDL_Ext::drawBorder(surface, realTarget.x, realTarget.y, realTarget.w, realTarget.h, CSDL_Ext::toSDL(color), width * getScalingFactor());
}

void Canvas::drawBorderDashed(const Rect & target, const ColorRGBA & color)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	if(renderTarget)
	{
		// SDL has no dashed primitive; approximate with alternating short segments
		bindRenderTarget();
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

		const auto dashedLine = [&](Point from, Point to)
		{
			static constexpr int dash = 4;
			const int length = std::max(std::abs(to.x - from.x), std::abs(to.y - from.y));

			for(int i = 0; i < length; i += dash * 2)
			{
				Point a = from + (to - from) * i / std::max(1, length);
				Point b = from + (to - from) * std::min(i + dash, length) / std::max(1, length);
				SDL_RenderLine(renderer, a.x, a.y, b.x, b.y);
			}
		};

		dashedLine(realTarget.topLeft(),    realTarget.topRight());
		dashedLine(realTarget.bottomLeft(), realTarget.bottomRight());
		dashedLine(realTarget.topLeft(),    realTarget.bottomLeft());
		dashedLine(realTarget.topRight(),   realTarget.bottomRight());
		return;
	}

	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.topRight(),    CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.bottomLeft(), realTarget.bottomRight(), CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.bottomLeft(),  CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topRight(),   realTarget.bottomRight(), CSDL_Ext::toSDL(color));
}

void Canvas::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text )
{
	if(renderTarget)
	{
		// The font stack writes glyphs into a surface, which a render target cannot accept,
		// so the string is rasterized once and drawn as a texture from then on
		auto image = GpuResources::get().textCache().getImage(font, text);

		if(image)
		{
			bindRenderTarget();
			Point topLeft = transformPos(position) + TextTextureCache::getAlignmentOffset(font, alignment, text);
			if(!image->drawTexture(GpuResources::get().renderer(), nullptr, topLeft, nullptr, colorDest, SDL_ALPHA_OPAQUE, EImageBlitMode::SIMPLE, ImageFlip{}))
				logGpuIssueOnce("rendered text has no texture representation");
		}
		return;
	}

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
	if(renderTarget)
	{
		// reuse the single-line GPU path per line, stepping down by the font's line height
		const auto & fontPtrGpu = ENGINE->renderHandler().loadFont(font);
		Point linePosition = position;

		for(const auto & line : text)
		{
			drawText(linePosition, font, colorDest, alignment, line);
			linePosition.y += fontPtrGpu->getLineHeight();
		}
		return;
	}

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
	SDL_Renderer * renderer = GpuResources::get().renderer();

	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	if(renderTarget)
	{
		bindRenderTarget();
		SDL_FRect rect = CSDL_Ext::toSDLFloat(realTarget);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderFillRect(renderer, &rect);
		return;
	}

	CSDL_Ext::fillRect(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::drawColorBlended(const Rect & target, const ColorRGBA & color)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	if(renderTarget)
	{
		bindRenderTarget();
		SDL_FRect rect = CSDL_Ext::toSDLFloat(realTarget);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderFillRect(renderer, &rect);
		return;
	}

	CSDL_Ext::fillRectBlended(surface, realTarget, CSDL_Ext::toSDL(color));
}

void Canvas::fillTexture(const std::shared_ptr<IImage>& image)
{
	assert(image);
	if (!image)
		return;
		
	Rect imageArea(Point(0, 0), image->dimensions());
	const Point area = renderArea.dimensions();

	for (int y=0; y < area.y; y+= imageArea.h)
	{
		for (int x=0; x < area.x; x+= imageArea.w)
		{
			Point at(renderArea.x + x * getScalingFactor(), renderArea.y + y * getScalingFactor());

			if(renderTarget)
			{
				bindRenderTarget();
				image->drawTexture(GpuResources::get().renderer(), at, nullptr, getScalingFactor());
			}
			else
				image->draw(surface, at, nullptr, getScalingFactor());
		}
	}
}

Rect Canvas::getRenderArea() const
{
	return renderArea;
}

ColorRGBA Canvas::getPixel(const Point & position) const
{
	if(renderTarget)
	{
		// readback stalls the GPU, but this is only used by occasional hit testing
		bindRenderTarget();

		SDL_Rect probe{ position.x, position.y, 1, 1 };
		SDL_Surface * pixel = SDL_RenderReadPixels(GpuResources::get().renderer(), &probe);

		if(!pixel)
		{
			logGpuIssueOnce(std::string("SDL_RenderReadPixels failed: ") + SDL_GetError());
			return ColorRGBA(0, 0, 0, 0);
		}

		SDL_Color read = CSDL_Ext::getColor(pixel, CSDL_Ext::getPixel(pixel, 0, 0));
		SDL_DestroySurface(pixel);
		return ColorRGBA(read.r, read.g, read.b, read.a);
	}

	SDL_Color color = CSDL_Ext::getColor(surface, CSDL_Ext::getPixel(surface, position.x, position.y));
	return ColorRGBA(color.r, color.g, color.b, color.a);
}

/// Rect::intersect reports "no overlap at all" as a negative rect, which SDL reads as a request
/// to stop clipping - clamp it, so a widget scrolled out of its viewport draws nothing.
static SDL_Rect toClipRect(const Rect & rect)
{
	SDL_Rect result = CSDL_Ext::toSDL(rect);

	result.w = std::max(0, result.w);
	result.h = std::max(0, result.h);

	return result;
}

CanvasClipRectGuard::CanvasClipRectGuard(Canvas & canvas, const Rect & rect): surf(canvas.surface)
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	const Rect scaled = rect * ENGINE->screenHandler().getScalingFactor();

	if(canvas.isRenderTarget())
	{
		// Clipping is renderer state and survives a target switch, so the destructor has to rebind
		// the canvas before undoing the clip.
		onRenderTarget = true;
		guarded = &canvas;
		canvas.bindRenderTarget();

		// An active clip may well be empty, so only SDL_RenderClipEnabled separates it from having
		// none - going by the rectangle would discard the clip this guard is nested in
		SDL_Rect previous{};
		hadClipRect = SDL_RenderClipEnabled(renderer) && SDL_GetRenderClipRect(renderer, &previous);

		// the clip is in target pixels, so it has to carry the canvas' own offset
		const Rect area = Rect(scaled.topLeft() + canvas.renderArea.topLeft(), scaled.dimensions()).intersect(canvas.renderArea);
		oldRect = hadClipRect ? CSDL_Ext::fromSDL(previous) : area;

		SDL_Rect clip = toClipRect(hadClipRect ? oldRect.intersect(area) : area);
		SDL_SetRenderClipRect(renderer, &clip);
		return;
	}

	CSDL_Ext::getClipRect(surf, oldRect);
	CSDL_Ext::setClipRect(surf, oldRect.intersect(scaled));
}

CanvasClipRectGuard::~CanvasClipRectGuard()
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	if(onRenderTarget)
	{
		// the clip belongs to whichever target is bound, so restore it on ours
		if(guarded)
			guarded->bindRenderTarget();

		if(hadClipRect)
		{
			SDL_Rect restored = toClipRect(oldRect);
			SDL_SetRenderClipRect(renderer, &restored);
		}
		else
			SDL_SetRenderClipRect(renderer, nullptr);
		return;
	}

	CSDL_Ext::setClipRect(surf, oldRect);
}
