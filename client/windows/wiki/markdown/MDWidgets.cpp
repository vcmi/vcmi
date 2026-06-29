/*
 * MDWidgets.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MDWidgets.h"

#include "../../../widgets/Images.h"
#include "../../../windows/InfoWindows.h"
#include "../../../render/Canvas.h"
#include "../../../render/Colors.h"
#include "../../../render/EFont.h"
#include "../../../render/IFont.h"
#include "../../../render/IRenderHandler.h"
#include "../../../GameEngine.h"

// ============================================================================
// WikiScaledImage
// ============================================================================

WikiScaledImage::WikiScaledImage(const ImagePath & path, int x, int y, int maxWidth)
	: CIntObject(0, Point(x, y))
{
	auto img = ENGINE->renderHandler().loadImage(path, EImageBlitMode::COLORKEY);
	if(!img) return;

	const Point nativeSz = img->dimensions();
	if(nativeSz.x <= 0 || nativeSz.y <= 0) return;

	if(nativeSz.x <= maxWidth)
	{
		scaledSize = nativeSz;
	}
	else
	{
		const double scale = static_cast<double>(maxWidth) / nativeSz.x;
		scaledSize = Point(maxWidth, std::max(1, static_cast<int>(std::round(nativeSz.y * scale))));
	}

	offscreen = ENGINE->renderHandler().createImage(nativeSz, CanvasScalingPolicy::IGNORE);
	Canvas c  = offscreen->getCanvas();
	c.draw(img, Point(0, 0));
	pos.w = scaledSize.x;
	pos.h = scaledSize.y;
}

int WikiScaledImage::height() const
{
	return scaledSize.y;
}

void WikiScaledImage::showAll(Canvas & to)
{
	if(!offscreen || scaledSize.x <= 0 || scaledSize.y <= 0) return;
	Canvas src = offscreen->getCanvas();
	to.drawScaled(src, pos.topLeft(), scaledSize);
}

// ============================================================================
// WikiAnimLoopWidget
// ============================================================================

WikiAnimLoopWidget::WikiAnimLoopWidget(const AnimationPath & path, int x, int y)
	: CIntObject(TIME, Point(x, y))
{
	OBJECT_CONSTRUCTION;
	img = std::make_shared<CAnimImage>(path, 0, 0, 0, 0);
	if(img)
	{
		frameCount = img->size();
		pos.w      = img->pos.w;
		pos.h      = img->pos.h;
	}
}

void WikiAnimLoopWidget::tick(uint32_t msPassed)
{
	if(!img || frameCount <= 1) return;
	elapsed += msPassed;
	while(elapsed >= FRAME_MS)
	{
		elapsed -= FRAME_MS;
		curFrame = (curFrame + 1) % frameCount;
		img->setFrame(curFrame);
	}
	redraw();
}

// ============================================================================
// WikiVideoWidget
// ============================================================================

WikiVideoWidget::WikiVideoWidget(const Point & position, const VideoPath & video, float scaleFactor)
	: VideoWidgetBase(position, video, false, scaleFactor)
	, loopedVideo(video)
{}

void WikiVideoWidget::onPlaybackFinished()
{
	playVideo(loopedVideo);
}

// ============================================================================
// WikiAltPopup
// ============================================================================

WikiAltPopup::WikiAltPopup(const Rect & area, std::string alt)
	: CIntObject(SHOW_POPUP, area.topLeft())
	, altText(std::move(alt))
{
	pos.w = area.w;
	pos.h = area.h;
}

void WikiAltPopup::showPopupWindow(const Point &)
{
	CRClickPopup::createAndPush(altText);
}

// ============================================================================
// WikiLinkLabel
// ============================================================================

WikiLinkLabel::WikiLinkLabel(int x, int y, int w, int h,
                             std::string text, std::string dest,
                             std::function<void(const std::string &)> callback)
	: CIntObject(LCLICK | HOVER)
	, linkText(std::move(text))
	, target(std::move(dest))
	, onLink(std::move(callback))
{
	pos.x += x; pos.y += y;
	pos.w = w;  pos.h = h;
}

void WikiLinkLabel::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	const ColorRGBA col = hovered ? LINK_HOVER : LINK_NORMAL;
	to.drawText(pos.topLeft(), FONT_SMALL, col, ETextAlignment::TOPLEFT, linkText);
	// Underline drawn 1 px below the font baseline
	const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
	const int lineY = pos.y + static_cast<int>(font->getLineHeight()) - 1;
	to.drawLine(Point(pos.x, lineY), Point(pos.x + pos.w - 1, lineY), col, col);
}

void WikiLinkLabel::clickPressed(const Point &)
{
	if(onLink) onLink(target);
}

void WikiLinkLabel::hover(bool on)
{
	hovered = on;
	redraw();
}

// ============================================================================
// WikiLinkOverlay
// ============================================================================

WikiLinkOverlay::WikiLinkOverlay(const Rect & area, std::string dest,
                                 std::function<void(const std::string &)> callback)
	: CIntObject(LCLICK)
	, target(std::move(dest))
	, onLink(std::move(callback))
{
	pos.x += area.x; pos.y += area.y;
	pos.w = area.w;  pos.h = area.h;
}

void WikiLinkOverlay::clickPressed(const Point &)
{
	if(onLink) onLink(target);
}
