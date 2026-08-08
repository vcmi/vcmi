/*
 * CanvasImage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IImage.h"
#include "Canvas.h"

class CanvasImage : public IImage
{
public:
	CanvasImage(const Point & size, CanvasScalingPolicy scalingPolicy);
	~CanvasImage();

	Canvas getCanvas();

	void draw(SDL_Surface * where, const Point & pos, const Rect * src, int scalingFactor) const override;
	void scaleTo(const Point & size, EScalingAlgorithm algorithm) override;
	void exportBitmap(const boost::filesystem::path & path) const override;
	Rect contentRect() const override;
	Point dimensions() const override;

	/// Draws the composed surface onto the renderer's target, uploading it on first use
	bool drawTexture(SDL_Renderer * renderer, const Point & pos, const Rect * src, int scalingFactor) const override;

	//no-op methods

	bool isTransparent(const Point & coords) const override{ return false;};
	void setAlpha(uint8_t value) override{};
	void playerColored(const PlayerColor & player) override{};
	void setOverlayColor(const ColorRGBA & color) override{};
	void setEffectColor(const ColorRGBA & color) override{};
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override{};
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override{};

	std::shared_ptr<ISharedImage> toSharedImage();

private:
	SDL_Surface * surface;

	/// GPU copy of surface, rebuilt whenever the surface is redrawn or the renderer changes
	mutable SDL_Texture * texture = nullptr;
	mutable uint32_t textureGeneration = 0;

	/// Drops the cached texture, so that the next GPU draw uploads the current surface
	void invalidateTexture() const;
	CanvasScalingPolicy scalingPolicy;
};

