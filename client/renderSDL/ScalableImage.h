/*
 * ScalableImage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/IImage.h"
#include "../render/ImageLocator.h"
#include "../render/Colors.h"

#include "../../lib/Color.h"

struct SDL_Palette;

class ScalableImageInstance;
class CanvasImage;

struct ScalableImageParameters : boost::noncopyable
{
	SDL_Palette * palette = nullptr;

	ColorRGBA colorMultiplier = Colors::WHITE_TRUE;
	ColorRGBA ovelayColorMultiplier = Colors::WHITE_TRUE;
	ColorRGBA effectColorMultiplier = Colors::TRANSPARENCY;

	PlayerColor player = PlayerColor::CANNOT_DETERMINE;
	uint8_t alphaValue = 255;

	bool flipVertical = false;
	bool flipHorizontal = false;

	ScalableImageParameters(const SDL_Palette * originalPalette, EImageBlitMode blitMode);
	~ScalableImageParameters();

	void setShadowTransparency(const SDL_Palette * originalPalette, float factor);
	void shiftPalette(const SDL_Palette * originalPalette, uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove);
	void playerColored(PlayerColor player);
	void setOverlayColor(const SDL_Palette * originalPalette, const ColorRGBA & color, bool includeShadow);
	void preparePalette(const SDL_Palette * originalPalette, EImageBlitMode blitMode);
	void adjustPalette(const SDL_Palette * originalPalette, EImageBlitMode blitMode, const ColorFilter & shifter, uint32_t colorsToSkipMask);
};

class ScalableImageShared final : public std::enable_shared_from_this<ScalableImageShared>, boost::noncopyable
{
	static constexpr int scalingSize = 5; // 0-4 range. TODO: switch to 1-4 since there is no '0' scaling
	static constexpr int maxFlips = 4;

	using ImageType = std::shared_ptr<const ISharedImage>;
	using FlippedImages = std::array<ImageType, maxFlips>;
	using PlayerColoredImages = std::array<ImageType, PlayerColor::PLAYER_LIMIT_I + 1>; // all valid colors+neutral

	struct ScaledImage
	{
		/// Upscaled shadow of our image, may be null
		FlippedImages shadow;

		/// Upscaled main part of our image, may be null
		FlippedImages body;

		/// Upscaled overlay (player color, selection highlight) of our image, may be null
		FlippedImages overlay;

		/// Upscaled grayscale version of body, for special effects in combat (e.g clone / petrify / berserk)
		FlippedImages bodyGrayscale;

		/// player-colored images of this particular scale, mostly for UI. These are never flipped in h3
		PlayerColoredImages playerColored;
	};

	/// 1x-4x images. body for 1x scaling is guaranteed to be loaded
	std::array<ScaledImage, scalingSize> scaled;

	/// Locator of this image, for loading additional (e.g. upscaled) images
	const SharedImageLocator locator;

	std::shared_ptr<const ISharedImage> loadOrGenerateImage(EImageBlitMode mode, int8_t scalingFactor, PlayerColor color, ImageType upscalingSource) const;

	void loadScaledImages(int8_t scalingFactor, PlayerColor color);

public:
	ScalableImageShared(const SharedImageLocator & locator, const std::shared_ptr<const ISharedImage> & baseImage);

	Point dimensions() const;
	void exportBitmap(const boost::filesystem::path & path, const ScalableImageParameters & parameters) const;
	bool isTransparent(const Point & coords) const;
	Rect contentRect() const;
	void draw(SDL_Surface * where, const Point & dest, const Rect * src, const ScalableImageParameters & parameters, int scalingFactor);

	const SDL_Palette * getPalette() const;

	std::shared_ptr<ScalableImageInstance> createImageReference();

	void prepareEffectImage();
	void preparePlayerColoredImage(PlayerColor color);
};

class ScalableImageInstance final : public IImage
{
	friend class ScalableImageShared;

	std::shared_ptr<ScalableImageShared> image;
	std::shared_ptr<CanvasImage> scaledImage;

	ScalableImageParameters parameters;
	EImageBlitMode blitMode;

public:
	ScalableImageInstance(const std::shared_ptr<ScalableImageShared> & image, EImageBlitMode blitMode);

	void scaleTo(const Point & size, EScalingAlgorithm algorithm) override;
	void exportBitmap(const boost::filesystem::path & path) const override;
	bool isTransparent(const Point & coords) const override;
	Rect contentRect() const override;
	Point dimensions() const override;
	void setAlpha(uint8_t value) override;
	void draw(SDL_Surface * where, const Point & pos, const Rect * src, int scalingFactor) const override;
	void setOverlayColor(const ColorRGBA & color) override;
	void setEffectColor(const ColorRGBA & color) override;
	void playerColored(const PlayerColor & player) override;
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override;
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override;

	void horizontalFlip();
	void verticalFlip();
};

