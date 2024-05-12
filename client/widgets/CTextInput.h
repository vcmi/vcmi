/*
 * CTextInput.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "TextControls.h"
#include "../gui/CIntObject.h"
#include "../gui/TextAlignment.h"
#include "../render/EFont.h"

#include "../../lib/FunctionList.h"
#include "../../lib/filesystem/ResourcePath.h"

class CLabel;
class IImage;

/// UIElement which can get input focus
class CFocusable : public virtual CIntObject
{
	static std::atomic<int> usageIndex;
public:
	bool focus; //only one focusable control can have focus at one moment

	void giveFocus(); //captures focus
	void moveFocus(); //moves focus to next active control (may be used for tab switching)
	void removeFocus(); //remove focus
	bool hasFocus() const;

	void focusGot();
	void focusLost();

	static std::list<CFocusable *> focusables; //all existing objs
	static CFocusable * inputWithFocus; //who has focus now

	CFocusable();
	~CFocusable();
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

	CTextInput(const Rect & Pos, EFonts font, const CFunctionList<void(const std::string &)> & CB, ETextAlignment alignment, bool giveFocusToInput);
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
