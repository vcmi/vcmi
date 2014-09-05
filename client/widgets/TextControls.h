#pragma once

#include "../gui/CIntObject.h"
#include "../gui/SDL_Extensions.h"
#include "../../lib/FunctionList.h"

/*
 * TextControls.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

	CTextContainer(EAlignment alignment, EFonts font, SDL_Color color);

public:
	EAlignment alignment;
	EFonts font;
	SDL_Color color; // default font color. Can be overridden by placing "{}" into the string
};

/// Label which shows text
class CLabel : public CTextContainer
{
protected:
	Point getBorderSize() override;
	virtual std::string visibleText();

	CPicture *bg;
public:

	std::string text;
	bool autoRedraw;  //whether control will redraw itself on setTxt

	std::string getText();
	virtual void setText(const std::string &Txt);

	CLabel(int x=0, int y=0, EFonts Font = FONT_SMALL, EAlignment Align = TOPLEFT,
	       const SDL_Color &Color = Colors::WHITE, const std::string &Text =  "");
	void showAll(SDL_Surface * to); //shows statusbar (with current text)
};

/// Small helper class to manage group of similar labels
class CLabelGroup : public CIntObject
{
	std::list<CLabel*> labels;
	EFonts font;
	EAlignment align;
	SDL_Color color;
public:
	CLabelGroup(EFonts Font = FONT_SMALL, EAlignment Align = TOPLEFT, const SDL_Color &Color = Colors::WHITE);
	void add(int x=0, int y=0, const std::string &text =  "");
};

/// Multi-line label that can display multiple lines of text
/// If text is too big to fit into requested area remaining part will not be visible
class CMultiLineLabel : public CLabel
{
	// text to blit, split into lines that are no longer than widget width
	std::vector<std::string> lines;

	// area of text that actually will be printed, default is widget size
	Rect visibleSize;

	void splitText(const std::string &Txt);
	Rect getTextLocation();
public:
	// total size of text, x = longest line of text, y = total height of lines
	Point textSize;

	CMultiLineLabel(Rect position, EFonts Font = FONT_SMALL, EAlignment Align = TOPLEFT, const SDL_Color &Color = Colors::WHITE, const std::string &Text =  "");

	void setText(const std::string &Txt);
	void showAll(SDL_Surface * to);

	void setVisibleSize(Rect visibleSize);
	// scrolls text visible in widget. Positive value will move text up
	void scrollTextTo(int distance);
	void scrollTextBy(int distance);
};

/// a multi-line label that tries to fit text with given available width and height;
/// if not possible, it creates a slider for scrolling text
class CTextBox : public CIntObject
{
	int sliderStyle;
public:
	CMultiLineLabel * label;
	CSlider *slider;

	CTextBox(std::string Text, const Rect &rect, int SliderStyle, EFonts Font = FONT_SMALL, EAlignment Align = TOPLEFT, const SDL_Color &Color = Colors::WHITE);

	void resize(Point newSize);
	void setText(const std::string &Txt);
	void sliderMoved(int to);
};

/// Status bar which is shown at the bottom of the in-game screens
class CGStatusBar : public CLabel
{
	bool textLock; //Used for blocking changes to the text
	void init();

	CGStatusBar *oldStatusBar;
protected:
	Point getBorderSize() override;

public:

	void clear();//clears statusbar and refreshes
	void setText(const std::string & Text) override; //prints text and refreshes statusbar

	void show(SDL_Surface * to); //shows statusbar (with current text)

	CGStatusBar(CPicture *BG, EFonts Font = FONT_SMALL, EAlignment Align = CENTER, const SDL_Color &Color = Colors::WHITE); //given CPicture will be captured by created sbar and it's pos will be used as pos for sbar
	CGStatusBar(int x, int y, std::string name, int maxw=-1);
	~CGStatusBar();

	void lock(bool shouldLock); //If true, current text cannot be changed until lock(false) is called
};

/// UIElement which can get input focus
class CFocusable : public virtual CIntObject
{
protected:
	virtual void focusGot(){};
	virtual void focusLost(){};
public:
	bool focus; //only one focusable control can have focus at one moment

	void giveFocus(); //captures focus
	void moveFocus(); //moves focus to next active control (may be used for tab switching)

	static std::list<CFocusable*> focusables; //all existing objs
	static CFocusable *inputWithFocus; //who has focus now
	CFocusable();
	~CFocusable();
};

/// Text input box where players can enter text
class CTextInput : public CLabel, public CFocusable
{
	std::string newText;
protected:
	std::string visibleText() override;

	void focusGot() override;
	void focusLost() override;
public:
	CFunctionList<void(const std::string &)> cb;
	CFunctionList<void(std::string &, const std::string &)> filters;
	void setText(const std::string &nText, bool callCb = false);

	CTextInput(const Rect &Pos, EFonts font, const CFunctionList<void(const std::string &)> &CB);
	CTextInput(const Rect &Pos, const Point &bgOffset, const std::string &bgName, const CFunctionList<void(const std::string &)> &CB);
	CTextInput(const Rect &Pos, SDL_Surface *srf = nullptr);

	void clickLeft(tribool down, bool previousState) override;
	void keyPressed(const SDL_KeyboardEvent & key) override;
	bool captureThisEvent(const SDL_KeyboardEvent & key) override;
	
#ifndef VCMI_SDL1
	void textInputed(const SDL_TextInputEvent & event) override;
	void textEdited(const SDL_TextEditingEvent & event) override;
	
	
#endif // VCMI_SDL1	

	//Filter that will block all characters not allowed in filenames
	static void filenameFilter(std::string &text, const std::string & oldText);
	//Filter that will allow only input of numbers in range min-max (min-max are allowed)
	//min-max should be set via something like std::bind
	static void numberFilter(std::string &text, const std::string & oldText, int minValue, int maxValue);
};
