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
#include "../gui/SDL_Extensions.h"
#include "../../lib/FunctionList.h"

class CSlider;

/// Base class for all text-related widgets.
/// Controls text blitting-related options
class CTextContainer : public virtual CIntObject
{
protected:
	/// returns size of border, for left- or right-aligned text
	virtual Point getBorderSize() = 0;
	/// do actual blitting of line. Text "what" will be placed at "where" and aligned according to alignment
	void blitLine(SDL_Surface * to, Rect where, std::string what);

	CTextContainer(ETextAlignment alignment, EFonts font, SDL_Color color);

public:
	ETextAlignment alignment;
	EFonts font;
	SDL_Color color; // default font color. Can be overridden by placing "{}" into the string
};

/// Label which shows text
class CLabel : public CTextContainer
{
protected:
	Point getBorderSize() override;
	virtual std::string visibleText();

	std::shared_ptr<CPicture> background;
public:

	std::string text;
	bool autoRedraw;  //whether control will redraw itself on setTxt

	std::string getText();
	virtual void setAutoRedraw(bool option);
	virtual void setText(const std::string & Txt);
	virtual void setColor(const SDL_Color & Color);
	size_t getWidth();

	CLabel(int x = 0, int y = 0, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT,
		const SDL_Color & Color = Colors::WHITE, const std::string & Text = "");
	void showAll(SDL_Surface * to) override; //shows statusbar (with current text)
};

/// Small helper class to manage group of similar labels
class CLabelGroup : public CIntObject
{
	std::vector<std::shared_ptr<CLabel>> labels;
	EFonts font;
	ETextAlignment align;
	SDL_Color color;
public:
	CLabelGroup(EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT, const SDL_Color & Color = Colors::WHITE);
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

	CMultiLineLabel(Rect position, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT, const SDL_Color & Color = Colors::WHITE, const std::string & Text = "");

	void setText(const std::string & Txt) override;
	void showAll(SDL_Surface * to) override;

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

	CTextBox(std::string Text, const Rect & rect, int SliderStyle, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::TOPLEFT, const SDL_Color & Color = Colors::WHITE);

	void resize(Point newSize);
	void setText(const std::string & Txt);
	void sliderMoved(int to);
};

/// Status bar which is shown at the bottom of the in-game screens
class CGStatusBar : public CLabel, public std::enable_shared_from_this<CGStatusBar>, public IStatusBar
{
	bool textLock; //Used for blocking changes to the text
	void init();

	CGStatusBar(std::shared_ptr<CPicture> background_, EFonts Font = FONT_SMALL, ETextAlignment Align = ETextAlignment::CENTER, const SDL_Color & Color = Colors::WHITE);
	CGStatusBar(int x, int y, std::string name, int maxw = -1);
protected:
	Point getBorderSize() override;

	void clickLeft(tribool down, bool previousState) override;

private:
	std::function<void()> onClick;

public:
	template<typename ...Args>
	static std::shared_ptr<CGStatusBar> create(Args... args)
	{
		std::shared_ptr<CGStatusBar> ret{new CGStatusBar{args...}};
		ret->init();
		return ret;
	}

	void clearIfMatching(const std::string & Text) override;
	void clear() override;//clears statusbar and refreshes
	void write(const std::string & Text) override; //prints text and refreshes statusbar

	void show(SDL_Surface * to) override; //shows statusbar (with current text)

	void lock(bool shouldLock) override; //If true, current text cannot be changed until lock(false) is called
	void setOnClick(std::function<void()> handler);
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
protected:
	std::string visibleText() override;

#ifdef VCMI_ANDROID
	void notifyAndroidTextInputChanged(std::string & text);
#endif
public:
	CFunctionList<void(const std::string &)> cb;
	CFunctionList<void(std::string &, const std::string &)> filters;
	void setText(const std::string & nText, bool callCb = false);

	CTextInput(const Rect & Pos, EFonts font, const CFunctionList<void(const std::string &)> & CB);
	CTextInput(const Rect & Pos, const Point & bgOffset, const std::string & bgName, const CFunctionList<void(const std::string &)> & CB);
	CTextInput(const Rect & Pos, SDL_Surface * srf = nullptr);

	void clickLeft(tribool down, bool previousState) override;
	void keyPressed(const SDL_KeyboardEvent & key) override;
	bool captureThisEvent(const SDL_KeyboardEvent & key) override;

	void textInputed(const SDL_TextInputEvent & event) override;
	void textEdited(const SDL_TextEditingEvent & event) override;

	//Filter that will block all characters not allowed in filenames
	static void filenameFilter(std::string & text, const std::string & oldText);
	//Filter that will allow only input of numbers in range min-max (min-max are allowed)
	//min-max should be set via something like std::bind
	static void numberFilter(std::string & text, const std::string & oldText, int minValue, int maxValue);

	friend class CKeyboardFocusListener;
};
