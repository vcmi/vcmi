/*
 * CScrollPanel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CSlider;

/// Container that scrolls its content vertically. Only the children that are fully visible receive the input
class CScrollPanel : public CIntObject
{
	std::shared_ptr<CIntObject> contentHolder;
	std::shared_ptr<CSlider> slider;
	int contentHeight = 0;
	int scrollY = 0;
	int panningAccumulated = 0;
	std::set<CIntObject *> suppressed;

	void scrollTo(int y);
	void applyScroll();
	int maxScroll() const;

public:
	explicit CScrollPanel(const Rect & area);
	~CScrollPanel() override;

	/// Parent for the scrolled widgets. Their positions are relative to the top left corner of the panel
	CIntObject * content() const;

	/// Width available for the content
	int contentWidth() const;

	/// Sets the height of the scrolled area, the scroll position is kept if possible
	void setContentHeight(int height);
	int getContentHeight() const;
	void scrollToTop();

	/// Scrolls so that the vertical range of content is visible
	void ensureVisible(int top, int bottom);

	void wheelScrolled(int distance) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void showAll(Canvas & to) override;
	void show(Canvas & to) override;
};
