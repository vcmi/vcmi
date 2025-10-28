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

#include "../gui/CIntObject.h"
#include "../gui/TextAlignment.h"
#include "../render/EFont.h"

#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/FunctionList.h"

class CLabel;
class IImage;

/// UIElement which can get input focus
class CFocusable : public CIntObject
{
	friend void removeFocusFromActiveInput();

	static std::atomic<int> usageIndex;
	static std::list<CFocusable *> focusables; //all existing objs
	static CFocusable * inputWithFocus; //who has focus now

	void focusGot();
	void focusLost();

	virtual void onFocusGot() = 0;
	virtual void onFocusLost() = 0;

public:
	void giveFocus(); //captures focus
	void moveFocus(); //moves focus to next active control (may be used for tab switching)
	void removeFocus(); //remove focus
	bool hasFocus() const;

	CFocusable();
	~CFocusable();
};

/// Text input box where players can enter text
class CTextInput : public CFocusable
{
protected:
	using TextEditedCallback = std::function<void(const std::string &)>;
	using TextFilterCallback = std::function<void(std::string &, const std::string &)>;

	std::string currentText;
	std::string composedText;
	ETextAlignment originalAlignment;

	std::shared_ptr<CPicture> background;
	std::shared_ptr<CLabel> label;

	TextEditedCallback onTextEdited;
	TextFilterCallback onTextFiltering;
	CFunctionList<void()> callbackPopup;

	//Filter that will block all characters not allowed in filenames
	static void filenameFilter(std::string & text, const std::string & oldText);
	//Filter that will allow only input of numbers in range min-max (min-max are allowed)
	//min-max should be set via something like std::bind
	static void numberFilter(std::string & text, const std::string & oldText, int minValue, int maxValue, int metricDigits);

	std::string getVisibleText() const;
	void createLabel(bool giveFocusToInput);
	void updateLabel();

	void clickPressed(const Point & cursorPosition) override;
	void textInputted(const std::string & enteredText) override;
	void textEdited(const std::string & enteredText) override;
	void onFocusGot() override;
	void onFocusLost() override;
	void showPopupWindow(const Point & cursorPosition) override;

	CTextInput(const Rect & Pos);
public:
	CTextInput(const Rect & Pos, EFonts font, ETextAlignment alignment, bool giveFocusToInput);
	CTextInput(const Rect & Pos, const Point & bgOffset, const ImagePath & bgName);
	CTextInput(const Rect & Pos, std::shared_ptr<IImage> srf);

	/// Returns currently entered text. May not match visible text
	const std::string & getText() const;

	void setText(const std::string & nText);

	/// Set callback that will be called whenever player enters new text
	void setCallback(const TextEditedCallback & cb);

	/// Set callback when player want to open popup
	void setPopupCallback(const std::function<void()> & cb);

	/// Enables filtering entered text that ensures that text is valid filename (existing or not)
	void setFilterFilename();
	/// Enable filtering entered text that ensures that text is valid number in provided range [min, max]
	void setFilterNumber(int minValue, int maxValue, int metricDigits=0);

	void setFont(EFonts Font);
	void setColor(const ColorRGBA & Color);
	void setAlignment(ETextAlignment alignment);

	// CIntObject interface impl
	void keyPressed(EShortcut key) override;
	void activate() override;
	void deactivate() override;
};

class CTextInputWithConfirm final : public CTextInput
{
	std::string initialText;
	std::function<void()> confirmCb;
	bool limitToRect;

	void confirm();
public:
	CTextInputWithConfirm(const Rect & Pos, EFonts font, ETextAlignment alignment, std::string text, bool limitToRect, std::function<void()> confirmCallback);

	bool captureThisKey(EShortcut key) override;
	void keyPressed(EShortcut key) override;
	void clickReleased(const Point & cursorPosition) override;
	void clickPressed(const Point & cursorPosition) override;
	bool receiveEvent(const Point & position, int eventType) const override;
	void onFocusGot() override;
	void textInputted(const std::string & enteredText) override;
	void deactivate() override;
};
