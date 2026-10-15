/*
 * CPromptWindows.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CDialogWindow.h"

class CTextBox;
class CTextField;
class CNumberInput;
class CLabel;

/// Dialog with a text and one or two buttons
class CMessageDialog : public CDialogWindow
{
	std::shared_ptr<CTextBox> textBox;

public:
	CMessageDialog(const std::string & title, const std::string & text, bool askQuestion);

	/// Shows the text with a single OK button
	static void showMessage(const std::string & title, const std::string & text);
	/// Shows the text with Yes and No buttons
	static void askConfirmation(const std::string & title, const std::string & text, const std::function<void()> & onYes, const std::function<void()> & onNo = {});
};

/// Dialog that asks for a single line of text
class CTextPrompt : public CDialogWindow
{
	std::shared_ptr<CLabel> promptLabel;
	std::shared_ptr<CTextField> field;

public:
	CTextPrompt(const std::string & title, const std::string & prompt, const std::string & initialText, bool filenameOnly);

	static void show(const std::string & title, const std::string & prompt, const std::string & initialText, const std::function<void(const std::string &)> & onAccept, bool filenameOnly = false);
};

/// Dialog that asks for a number
class CNumberPrompt : public CDialogWindow
{
	std::shared_ptr<CLabel> promptLabel;
	std::shared_ptr<CNumberInput> input;

public:
	CNumberPrompt(const std::string & title, const std::string & prompt, int value, int minValue, int maxValue);

	static void show(const std::string & title, const std::string & prompt, int value, int minValue, int maxValue, const std::function<void(int)> & onAccept);
};
