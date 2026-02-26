/*
 * TextControls.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../gui/TextAlignment.h"
#include "../render/Colors.h"
#include "../render/EFont.h"
#include "../../lib/filesystem/ResourcePath.h"

class IImage;
class CSlider;

/// Base class for all text-related widgets.
/// Controls text blitting-related options
class CTextContainer : public virtual CIntObject
{
protected:
	/// returns size of border, for left- or right-aligned text
	virtual Point getBorderSize() = 0;
	/// do actual blitting of line. Text "what" will be placed at "where" and aligned according to alignment
	void blitLine(Canvas & to, Rect where, std::string what);

	CTextContainer(ETextAlignment alignment, EFonts font, ColorRGBA color);

public:
	static int getDelimitersWidth(EFonts font, std::string text);

	ETextAlignment alignment;
	EFonts font;
	ColorRGBA color; // default font color. Can be overridden by placing "{}" into the string
};

/// Label which shows text
class CLabel : public CTextContainer
{
protected:
	Point getBorderSize() override;
	virtual std::string visibleText();
	virtual void trimText();

	std::shared_ptr<CIntObject> background;
	std::string text;
	int maxWidth;
	bool autoRedraw;  //whether control will redraw itself on setTxt

public:

	std::string getText();
	virtual void setAutoRedraw(bool option);
	virtual void setText(const std::string & Txt);
	virtual void setMaxWidth(int width);
	virtual void setColor(const ColorRGBA & Color);
	void clear();
	size_t getWidth();

	CLabel(int x = 0, int y = 0, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT,
		const ColorRGBA & Color = Colors::WHITE, const std::string & Text = "", int maxWidth = 0);
	void showAll(Canvas & to) override; //shows statusbar (with current text)
};

/// Small helper class to manage group of similar labels
class CLabelGroup : public CIntObject
{
	std::vector<std::shared_ptr<CLabel>> labels;
	EFonts font;
	ETextAlignment align;
	ColorRGBA color;
public:
	CLabelGroup(EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT, const ColorRGBA & Color = Colors::WHITE);
	void add(int x = 0, int y = 0, const std::string & text = "");
	size_t currentSize() const;
};

/// Multi-line label that can display multiple lines of text
/// If text is too big to fit into requested area remaining part will not be visible
class CMultiLineLabel : public CLabel
{
	// text to blit, split into lines that are no longer than widget width
	std::vector<std::string> lines;

	// area of text that actually will be printed, default is widget size
	Rect visibleSize;

	void splitText(const std::string & Txt, bool redrawAfter);
	Rect getTextLocation();
public:
	// total size of text, x = longest line of text, y = total height of lines
	Point textSize;

	CMultiLineLabel(Rect position, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT, const ColorRGBA & Color = Colors::WHITE, const std::string & Text = "");

	void setText(const std::string & Txt) override;
	std::vector<std::string> getLines();
	void showAll(Canvas & to) override;

	void setVisibleSize(Rect visibleSize, bool redrawElement = true);
	// scrolls text visible in widget. Positive value will move text up
	void scrollTextTo(int distance, bool redrawAfterScroll = true);
	void scrollTextBy(int distance);
};

/// a multi-line label that tries to fit text with given available width and height;
/// if not possible, it creates a slider for scrolling text
class CTextBox : public CIntObject
{
	int sliderStyle;
public:
	std::shared_ptr<CMultiLineLabel> label;
	std::shared_ptr<CSlider> slider;

	CTextBox(std::string Text, const Rect & rect, int SliderStyle, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT, const ColorRGBA & Color = Colors::WHITE);

	void resize(Point newSize);
	/// Resizes text box to minimal size needed to fit current text
	/// No effect if text is too large to fit and requires slider
	void trimToFit();
	void setText(const std::string & Txt);
	void sliderMoved(int to);
};

/// Status bar which is shown at the bottom of the in-game screens
class CGStatusBar : public CLabel, public std::enable_shared_from_this<CGStatusBar>, public IStatusBar
{
	std::string hoverText;
	std::string consoleText;
	bool enteringText;

	CGStatusBar(std::shared_ptr<CIntObject> background_, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::CENTER, const ColorRGBA & Color = Colors::WHITE);
	CGStatusBar(int x, int y, const ImagePath & name, int maxw = -1);
	CGStatusBar(int x, int y);

	//make CLabel API private
	using CLabel::getText;
	using CLabel::setAutoRedraw;
	using CLabel::setText;
	using CLabel::setColor;
	using CLabel::getWidth;

protected:
	Point getBorderSize() override;

	void clickPressed(const Point & cursorPosition) override;

public:
	~CGStatusBar();

	template<typename ...Args>
	static std::shared_ptr<CGStatusBar> create(Args... args)
	{
		std::shared_ptr<CGStatusBar> ret{new CGStatusBar{args...}};
		return ret;
	}

	void show(Canvas & to) override;
	void activate() override;
	void deactivate() override;

	// IStatusBar interface
	void write(const std::string & Text) override;
	void clearIfMatching(const std::string & Text) override;
	void clear() override;
	void setEnteringMode(bool on) override;
	void setEnteredText(const std::string & text) override;
};
