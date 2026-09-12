/*
 * Canvas.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "gui/TextAlignment.h"
#include "lib/Rect.h"
#include "lib/Color.h"

struct SDL_Surface;
struct SDL_Texture;
class IImage;
class IVideoInstance;
enum EFonts : int8_t;

enum class CanvasScalingPolicy
{
	AUTO,  // automatically scale canvas operations by global scaling factor
	IGNORE // disable any scaling processing. Scaling factor will be set to 1

};

/// Class that represents surface for drawing on
class Canvas
{
	friend class CanvasClipRectGuard;

	/// Upscaler awareness. Must be first member for initialization
	CanvasScalingPolicy scalingPolicy;

	/// Target surface. Null when this canvas draws onto a GPU render target instead.
	SDL_Surface * surface;

	/// GPU render target this canvas draws onto, or null for the software path.
	/// The caller owns it; the canvas only references it.
	SDL_Texture * renderTarget = nullptr;

	/// Whether this canvas has to destroy renderTarget. Only the canvas handed out by
	/// the screen handler owns it; copies and clipped views never do.
	bool ownsRenderTarget = false;

	/// GpuResources generation renderTarget was created under; deferred destruction only
	/// queues it when this still matches, same as SDLImage/CanvasImage do for their textures
	uint32_t renderTargetGeneration = 0;

	/// Current rendering area, all rendering operations will be moved into selected area
	Rect renderArea;

	/// constructs canvas using existing surface. Caller maintains ownership on the surface
	explicit Canvas(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy);

	/// constructs canvas drawing onto a GPU render target. Caller maintains ownership
	Canvas(SDL_Texture * renderTarget, const Point & size, CanvasScalingPolicy scalingPolicy, bool ownsRenderTarget = false);

	/// Makes the renderer draw onto this canvas' target, if it is not already doing so
	void bindRenderTarget() const;

	/// GPU counterpart of the canvas-to-canvas blits. blendMode is an SDL_BlendMode,
	/// kept untyped so that this header does not have to pull in SDL
	void copyFromCanvas(const Canvas & image, const Rect & targetArea, uint32_t blendMode, uint8_t alpha);

	/// Fallback for content that has no texture of its own: renders it into a scratch
	/// surface through the software path, then uploads that onto this canvas' target
	void drawViaScratchSurface(const Point & pos, const Point & size, const std::function<void(SDL_Surface *)> & render);

	/// copy constructor
	Canvas(const Canvas & other);

	Point transformPos(const Point & input);
	Point transformSize(const Point & input);

public:
	Canvas & operator = (const Canvas & other) = delete;
	Canvas & operator = (Canvas && other) = delete;

	/// move constructor
	Canvas(Canvas && other);

	/// creates canvas that only covers specified subsection of a surface
	Canvas(const Canvas & other, const Rect & clipRect);

	/// constructs canvas of specified size
	explicit Canvas(const Point & size, CanvasScalingPolicy scalingPolicy);

	/// constructs canvas using existing surface. Caller maintains ownership on the surface
	/// Compatibility method. AVOID USAGE. To be removed once SDL abstraction layer is finished.
	static Canvas createFromSurface(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy);

	/// constructs canvas drawing onto a GPU render target of the given size in logical units
	static Canvas createFromRenderTarget(SDL_Texture * renderTarget, const Point & size, CanvasScalingPolicy scalingPolicy);

	/// Same, but the returned canvas destroys the render target when it dies
	static Canvas createOwningRenderTarget(SDL_Texture * renderTarget, const Point & size, CanvasScalingPolicy scalingPolicy);

	/// True when this canvas draws with the GPU rather than into a surface
	bool isRenderTarget() const { return renderTarget != nullptr; }

	/// The texture this canvas draws onto, for the screen handler to read from while it composes
	/// the frame. Null for a surface-backed canvas.
	SDL_Texture * getRenderTargetTexture() const { return renderTarget; }

	~Canvas();

	/// if set to true, drawing this canvas onto another canvas will use alpha channel information
	void applyTransparency(bool on);

	/// applies grayscale filter onto current image
	void applyGrayscale();

	/// renders image onto this canvas at specified position
	void draw(const std::shared_ptr<IImage>& image, const Point & pos);
	void draw(const IImage& image, const Point & pos);

	void draw(IVideoInstance & video, const Point & pos);

	/// renders section of image bounded by sourceRect at specified position
	void draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect);

	/// renders another canvas onto this canvas
	void draw(const Canvas &image, const Point & pos);

	/// renders another canvas onto this canvas with transparency
	void drawTransparent(const Canvas & image, const Point & pos, double transparency);

	/// renders another canvas onto this canvas with scaling
	void drawScaled(const Canvas &image, const Point & pos, const Point & targetSize);

	/// renders single pixels with specified color
	void drawPoint(const Point & dest, const ColorRGBA & color);

	/// renders continuous, 1-pixel wide line with color gradient
	void drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest);

	/// renders rectangular, solid-color border in specified location
	void drawBorder(const Rect & target, const ColorRGBA & color, int width = 1);

	/// renders rectangular, dashed border in specified location
	void drawBorderDashed(const Rect & target, const ColorRGBA & color);

	/// renders single line of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text );

	/// renders multiple lines of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::vector<std::string> & text );

	/// fills selected area with solid color
	void drawColor(const Rect & target, const ColorRGBA & color);

	/// fills selected area with blended color
	void drawColorBlended(const Rect & target, const ColorRGBA & color);

	/// fills canvas with texture
	void fillTexture(const std::shared_ptr<IImage>& image);

	int getScalingFactor() const;

	/// get the render area
	Rect getRenderArea() const;

	/// get pixel color
	ColorRGBA getPixel(const Point & position) const;
};

class CanvasClipRectGuard : boost::noncopyable
{
	SDL_Surface * surf;
	Rect oldRect;

	/// Set when the guarded canvas draws on the GPU, where clipping is renderer state
	/// rather than a property of the target
	bool onRenderTarget = false;
	bool hadClipRect = false;

	/// The guarded canvas, so its target can be rebound before the clip is restored
	const Canvas * guarded = nullptr;

public:
	CanvasClipRectGuard(Canvas & canvas, const Rect & rect);
	~CanvasClipRectGuard();
};
