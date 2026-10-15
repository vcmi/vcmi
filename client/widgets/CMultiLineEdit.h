/*
 * CMultiLineEdit.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CTextInput.h"

/// Editor of a text that has multiple lines. Text is wrapped on the words that do not fit the width
class CMultiLineEdit : public CFocusable
{
	struct Segment
	{
		size_t begin;
		size_t end;
	};

	std::string text;
	std::string composedText;
	std::vector<Segment> segments;
	std::function<void(const std::string &)> callback;
	size_t caret = 0;
	int firstSegment = 0;

	int lineHeight() const;
	int visibleSegments() const;
	int textWidth(size_t begin, size_t end) const;
	size_t segmentOfCaret() const;
	size_t caretInSegment(size_t index, int x) const;
	size_t caretAt(const Point & position) const;
	void layout();
	void scrollToCaret();
	void edited();
	void moveCaretVertically(int direction);

	void onFocusGot() override;
	void onFocusLost() override;

public:
	CMultiLineEdit(const Rect & area, const std::string & text, const std::function<void(const std::string &)> & callback = {});

	const std::string & getText() const;
	/// Changes the text without calling the callback
	void setText(const std::string & newText);
	void setCallback(const std::function<void(const std::string &)> & newCallback);

	bool captureThisKey(EShortcut key) override;
	void clickPressed(const Point & cursorPosition) override;
	void wheelScrolled(int distance) override;
	void textInputted(const std::string & enteredText) override;
	void textEdited(const std::string & enteredText) override;
	void keyPressed(EShortcut key) override;
	void activate() override;
	void deactivate() override;
	void showAll(Canvas & to) override;
};
