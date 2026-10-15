/*
 * CPromptWindows.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPromptWindows.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CNumberInput.h"
#include "../widgets/CTextField.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"

namespace
{
constexpr int MESSAGE_WIDTH = 440;
constexpr int PROMPT_WIDTH = 400;
}

CMessageDialog::CMessageDialog(const std::string & title, const std::string & text, bool askQuestion)
	: CDialogWindow(Point(MESSAGE_WIDTH, 200), title, askQuestion)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	textBox = std::make_shared<CTextBox>(text, Rect(0, 0, size.x, size.y), CSlider::BLUE, UiTheme::FONT, ETextAlignment::CENTER, UiTheme::TEXT);

	if(askQuestion)
	{
		setAcceptText(LIBRARY->generaltexth->translate("vcmi.ui.yes"));
		setCancelText(LIBRARY->generaltexth->translate("vcmi.ui.no"));
	}
}

void CMessageDialog::showMessage(const std::string & title, const std::string & text)
{
	ENGINE->windows().createAndPushWindow<CMessageDialog>(title, text, false);
}

void CMessageDialog::askConfirmation(const std::string & title, const std::string & text, const std::function<void()> & onYes, const std::function<void()> & onNo)
{
	auto dialog = std::make_shared<CMessageDialog>(title, text, true);
	dialog->afterAccept = onYes;
	dialog->onCancel = onNo;
	ENGINE->windows().pushWindow(dialog);
}

CTextPrompt::CTextPrompt(const std::string & title, const std::string & prompt, const std::string & initialText, bool filenameOnly)
	: CDialogWindow(Point(PROMPT_WIDTH, 140), title)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	promptLabel = std::make_shared<CLabel>(0, 0, UiTheme::FONT, ETextAlignment::TOPLEFT, UiTheme::TEXT, prompt);
	field = std::make_shared<CTextField>(Rect(0, 24, size.x, UiTheme::CONTROL_HEIGHT), initialText);
	if(filenameOnly)
		field->setFilterFilename();

#if !defined(VCMI_MOBILE)
	field->giveFocus();
#endif
}

void CTextPrompt::show(const std::string & title, const std::string & prompt, const std::string & initialText, const std::function<void(const std::string &)> & onAccept, bool filenameOnly)
{
	auto dialog = std::make_shared<CTextPrompt>(title, prompt, initialText, filenameOnly);
	auto * raw = dialog.get();
	dialog->onAccept = [raw, onAccept]()
	{
		const std::string text = raw->field->getText();
		if(onAccept)
			onAccept(text);
		return true;
	};
	ENGINE->windows().pushWindow(dialog);
}

CNumberPrompt::CNumberPrompt(const std::string & title, const std::string & prompt, int value, int minValue, int maxValue)
	: CDialogWindow(Point(PROMPT_WIDTH, 140), title)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	promptLabel = std::make_shared<CLabel>(0, 0, UiTheme::FONT, ETextAlignment::TOPLEFT, UiTheme::TEXT, prompt);
	input = std::make_shared<CNumberInput>(Rect(0, 24, size.x, UiTheme::CONTROL_HEIGHT), value, minValue, maxValue);
}

void CNumberPrompt::show(const std::string & title, const std::string & prompt, int value, int minValue, int maxValue, const std::function<void(int)> & onAccept)
{
	auto dialog = std::make_shared<CNumberPrompt>(title, prompt, value, minValue, maxValue);
	auto * raw = dialog.get();
	dialog->onAccept = [raw, onAccept]()
	{
		const int number = raw->input->getValue();
		if(onAccept)
			onAccept(number);
		return true;
	};
	ENGINE->windows().pushWindow(dialog);
}
