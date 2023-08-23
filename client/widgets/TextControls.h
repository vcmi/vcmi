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
#include "../../lib/FunctionList.h"
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

	std::shared_ptr<CIntObject> background;
	std::string text;
	bool autoRedraw;  //whether control will redraw itself on setTxt

public:

	std::string getText();
	virtual void setAutoRedraw(bool option);
	virtual void setText(const std::string & Txt);
	virtual void setColor(const ColorRGBA & Color);
	size_t getWidth();

	CLabel(int x = 0, int y = 0, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT,
		const ColorRGBA & Color = Colors::WHITE, const std::string & Text = "");
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

class CFocusable;

class IFocusListener
{
public:
	virtual void focusGot() {};
	virtual void focusLost() {};
	virtual ~IFocusListener() = default;
};

/// UIElement which can get input focus
class CFocusable : public virtual CIntObject
{
private:
	std::shared_ptr<IFocusListener> focusListener;

public:
	bool focus; //only one focusable control can have focus at one moment

	void giveFocus(); //captures focus
	void moveFocus(); //moves focus to next active control (may be used for tab switching)
	bool hasFocus() const;

	static std::list<CFocusable *> focusables; //all existing objs
	static CFocusable * inputWithFocus; //who has focus now

	CFocusable();
	CFocusable(std::shared_ptr<IFocusListener> focusListener);
	~CFocusable();
};

class CTextInput;
class CKeyboardFocusListener : public IFocusListener
{
private:
	static std::atomic<int> usageIndex;
	CTextInput * textInput;

public:
	CKeyboardFocusListener(CTextInput * textInput);
	void focusGot() override;
	void focusLost() override;
};

/// Text input box where players can enter text
class CTextInput : public CLabel, public CFocusable
{
	std::string newText;
	std::string helpBox; //for right-click help
	
protected:
	std::string visibleText() override;

public:
	
	CFunctionList<void(const std::string &)> cb;
	CFunctionList<void(std::string &, const std::string &)> filters;
	void setText(const std::string & nText) override;
	void setText(const std::string & nText, bool callCb);
	void setHelpText(const std::string &);

	CTextInput(const Rect & Pos, EFonts font, const CFunctionList<void(const std::string &)> & CB);
	CTextInput(const Rect & Pos, const Point & bgOffset, const ImagePath & bgName, const CFunctionList<void(const std::string &)> & CB);
	CTextInput(const Rect & Pos, std::shared_ptr<IImage> srf);

	void clickPressed(const Point & cursorPosition) override;
	void keyPressed(EShortcut key) override;
	void showPopupWindow(const Point & cursorPosition) override;

	//bool captureThisKey(EShortcut key) override;

	void textInputed(const std::string & enteredText) override;
	void textEdited(const std::string & enteredText) override;

	//Filter that will block all characters not allowed in filenames
	static void filenameFilter(std::string & text, const std::string & oldText);
	//Filter that will allow only input of numbers in range min-max (min-max are allowed)
	//min-max should be set via something like std::bind
	static void numberFilter(std::string & text, const std::string & oldText, int minValue, int maxValue);

	friend class CKeyboardFocusListener;
};
