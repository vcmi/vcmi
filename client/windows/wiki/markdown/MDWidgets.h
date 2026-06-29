/*
 * MDWidgets.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../gui/CIntObject.h"
#include "../../../widgets/Images.h"
#include "../../../widgets/VideoWidget.h"
#include "../../../render/CanvasImage.h"

class Canvas;

// ============================================================================
// WikiScaledImage
//
// Renders a static image, downscaling it proportionally when it exceeds @p maxWidth.
// ============================================================================

class WikiScaledImage : public CIntObject
{
	std::shared_ptr<CanvasImage> offscreen;
	Point scaledSize;

public:
	WikiScaledImage(const ImagePath & path, int x, int y, int maxWidth);

	int height() const;

	void showAll(Canvas & to) override;
};

// ============================================================================
// WikiAnimLoopWidget
//
// Cycles all frames of a DEF animation at approximately 6 fps (~150 ms per frame).
// ============================================================================

class WikiAnimLoopWidget : public CIntObject
{
	std::shared_ptr<CAnimImage> img;
	size_t   frameCount = 0;
	size_t   curFrame   = 0;
	uint32_t elapsed    = 0;

	static constexpr uint32_t FRAME_MS = 150;

public:
	explicit WikiAnimLoopWidget(const AnimationPath & path, int x, int y);

	void tick(uint32_t msPassed) override;
};

// ============================================================================
// WikiVideoWidget
//
// Seamlessly looped video widget with optional downscaling.
// ============================================================================

class WikiVideoWidget final : public VideoWidgetBase
{
	VideoPath loopedVideo;

	void onPlaybackFinished() final;

public:
	WikiVideoWidget(const Point & position, const VideoPath & video, float scaleFactor);
};

// ============================================================================
// WikiAltPopup
//
// Invisible right-click overlay that shows the alt text of a media element.
// ============================================================================

class WikiAltPopup : public CIntObject
{
	std::string altText;

public:
	WikiAltPopup(const Rect & area, std::string alt);

	void showPopupWindow(const Point & cursor) override;
};

// ============================================================================
// WikiLinkLabel
//
// Clickable, underlined text link that navigates to another wiki page on click.
// ============================================================================

class WikiLinkLabel : public CIntObject
{
	std::string linkText;
	std::string target;
	std::function<void(const std::string &)> onLink;
	bool hovered = false;

	static constexpr ColorRGBA LINK_NORMAL = {120, 180, 255, 255};
	static constexpr ColorRGBA LINK_HOVER  = {180, 220, 255, 255};

public:
	WikiLinkLabel(int x, int y, int w, int h,
	              std::string text, std::string dest,
	              std::function<void(const std::string &)> callback);

	void showAll(Canvas & to) override;
	void clickPressed(const Point & cursor) override;
	void hover(bool on) override;
};

// ============================================================================
// WikiLinkOverlay
//
// Invisible click-area placed over a media widget to make clicking it navigate.
// Right-click is not intercepted so the WikiAltPopup below still fires.
// ============================================================================

class WikiLinkOverlay : public CIntObject
{
	std::string target;
	std::function<void(const std::string &)> onLink;

public:
	WikiLinkOverlay(const Rect & area, std::string dest,
	                std::function<void(const std::string &)> callback);

	void clickPressed(const Point & cursor) override;
};
