/*
 * CMultiLineEdit.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMultiLineEdit.h"

#include "UiTheme.h"

#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "events/InputHandler.h"
#include "render/Canvas.h"
#include "render/IFont.h"
#include "render/IRenderHandler.h"

#include "../../lib/texts/TextOperations.h"

namespace
{
constexpr int TEXT_MARGIN = 4;
}

CMultiLineEdit::CMultiLineEdit(const Rect & area, const std::string & text, const std::function<void(const std::string &)> & callback)
	: text(text)
	, callback(callback)
{
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT | WHEEL);
	setRedrawParent(true);
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = area.h;

	caret = this->text.size();
	layout();
	scrollToCaret();
}

int CMultiLineEdit::lineHeight() const
{
	return static_cast<int>(ENGINE->renderHandler().loadFont(UiTheme::FONT)->getLineHeight());
}

int CMultiLineEdit::visibleSegments() const
{
	return std::max(1, (pos.h - 2 * TEXT_MARGIN) / lineHeight());
}

int CMultiLineEdit::textWidth(size_t begin, size_t end) const
{
	const auto font = ENGINE->renderHandler().loadFont(UiTheme::FONT);
	int width = 0;
	for(size_t i = begin; i < end; i += TextOperations::getUnicodeCharacterSize(text[i]))
		width += static_cast<int>(font->getGlyphWidth(&text[i]));
	return width;
}

void CMultiLineEdit::layout()
{
	segments.clear();

	const int maxWidth = pos.w - 2 * TEXT_MARGIN;
	const auto font = ENGINE->renderHandler().loadFont(UiTheme::FONT);
	size_t lineBegin = 0;

	while(true)
	{
		size_t lineEnd = text.find('\n', lineBegin);
		const bool lastLine = lineEnd == std::string::npos;
		if(lastLine)
			lineEnd = text.size();

		if(lineBegin == lineEnd)
			segments.push_back({lineBegin, lineEnd});

		size_t begin = lineBegin;
		while(begin < lineEnd)
		{
			size_t end = begin;
			size_t lastWordEnd = std::string::npos;
			int width = 0;

			while(end < lineEnd)
			{
				const size_t charSize = TextOperations::getUnicodeCharacterSize(text[end]);
				const int glyph = static_cast<int>(font->getGlyphWidth(&text[end]));

				if(width + glyph > maxWidth && end > begin)
				{
					if(lastWordEnd != std::string::npos)
						end = lastWordEnd;
					break;
				}

				width += glyph;
				end += charSize;

				if(text[end - charSize] == ' ')
					lastWordEnd = end;
			}

			segments.push_back({begin, end});
			begin = end;
		}

		if(lastLine)
			break;

		lineBegin = lineEnd + 1;
	}
}

size_t CMultiLineEdit::segmentOfCaret() const
{
	for(size_t i = 0; i < segments.size(); ++i)
	{
		const bool lastSegment = i + 1 == segments.size();
		if(caret >= segments[i].begin && (caret < segments[i].end || lastSegment))
			return i;

		// the newline that ends the line belongs to the end of that line
		if(!lastSegment && caret == segments[i].end && segments[i + 1].begin > segments[i].end)
			return i;
	}
	return segments.empty() ? 0 : segments.size() - 1;
}

size_t CMultiLineEdit::caretInSegment(size_t index, int x) const
{
	if(segments.empty())
		return 0;

	const Segment & segment = segments[index];
	const auto font = ENGINE->renderHandler().loadFont(UiTheme::FONT);
	int current = pos.x + TEXT_MARGIN;
	size_t result = segment.begin;

	while(result < segment.end)
	{
		const size_t charSize = TextOperations::getUnicodeCharacterSize(text[result]);
		const int glyph = static_cast<int>(font->getGlyphWidth(&text[result]));
		if(x < current + glyph / 2)
			break;

		current += glyph;
		result += charSize;
	}
	return result;
}

size_t CMultiLineEdit::caretAt(const Point & position) const
{
	if(segments.empty())
		return 0;

	const int row = std::clamp((position.y - pos.y - TEXT_MARGIN) / lineHeight(), 0, visibleSegments() - 1);
	const size_t index = std::min(segments.size() - 1, static_cast<size_t>(firstSegment + row));
	return caretInSegment(index, position.x);
}

void CMultiLineEdit::scrollToCaret()
{
	const int row = static_cast<int>(segmentOfCaret());
	const int visible = visibleSegments();

	if(row < firstSegment)
		firstSegment = row;
	else if(row >= firstSegment + visible)
		firstSegment = row - visible + 1;

	firstSegment = std::clamp(firstSegment, 0, std::max(0, static_cast<int>(segments.size()) - visible));
}

void CMultiLineEdit::edited()
{
	layout();
	scrollToCaret();
	redraw();

	if(callback)
		callback(text);
}

void CMultiLineEdit::moveCaretVertically(int direction)
{
	const size_t current = segmentOfCaret();
	const int row = static_cast<int>(current) + direction;
	if(row < 0 || row >= static_cast<int>(segments.size()))
		return;

	const int x = pos.x + TEXT_MARGIN + textWidth(segments[current].begin, caret);
	caret = caretInSegment(static_cast<size_t>(row), x);
	scrollToCaret();
}

const std::string & CMultiLineEdit::getText() const
{
	return text;
}

void CMultiLineEdit::setText(const std::string & newText)
{
	text = newText;
	caret = text.size();
	layout();
	scrollToCaret();
	redraw();
}

void CMultiLineEdit::setCallback(const std::function<void(const std::string &)> & newCallback)
{
	callback = newCallback;
}

bool CMultiLineEdit::captureThisKey(EShortcut key)
{
	if(!hasFocus())
		return false;

	switch(key)
	{
		case EShortcut::GLOBAL_ACCEPT:
		case EShortcut::GLOBAL_BACKSPACE:
		case EShortcut::MOVE_LEFT:
		case EShortcut::MOVE_RIGHT:
		case EShortcut::MOVE_UP:
		case EShortcut::MOVE_DOWN:
		case EShortcut::MOVE_FIRST:
		case EShortcut::MOVE_LAST:
			return true;
		default:
			return false;
	}
}

void CMultiLineEdit::clickPressed(const Point & cursorPosition)
{
#ifdef VCMI_SWITCH
	if(auto edited = CTextInput::showNativeKeyboard(text))
	{
		setText(*edited);
		if(callback)
			callback(text);
		return;
	}
#endif
	giveFocus();
	caret = caretAt(cursorPosition);
	redraw();
}

void CMultiLineEdit::wheelScrolled(int distance)
{
	firstSegment = std::clamp(firstSegment - distance * UiTheme::SCROLL_STEP, 0, std::max(0, static_cast<int>(segments.size()) - visibleSegments()));
	redraw();
}

void CMultiLineEdit::textInputted(const std::string & enteredText)
{
	if(!hasFocus())
		return;

	text.insert(caret, enteredText);
	caret += enteredText.size();
	composedText.clear();
	edited();
}

void CMultiLineEdit::textEdited(const std::string & enteredText)
{
	if(!hasFocus())
		return;

	composedText = enteredText;
	redraw();
}

void CMultiLineEdit::keyPressed(EShortcut key)
{
	if(!hasFocus())
		return;

	switch(key)
	{
		case EShortcut::GLOBAL_ACCEPT:
			text.insert(caret, "\n");
			++caret;
			edited();
			break;

		case EShortcut::GLOBAL_BACKSPACE:
		{
			if(!composedText.empty())
			{
				TextOperations::trimRightUnicode(composedText);
				redraw();
			}
			else if(caret > 0)
			{
				size_t start = caret - 1;
				while(start > 0 && (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80)
					--start;

				text.erase(start, caret - start);
				caret = start;
				edited();
			}
			break;
		}

		case EShortcut::MOVE_LEFT:
		{
			if(caret > 0)
			{
				--caret;
				while(caret > 0 && (static_cast<unsigned char>(text[caret]) & 0xC0) == 0x80)
					--caret;
			}
			scrollToCaret();
			redraw();
			break;
		}

		case EShortcut::MOVE_RIGHT:
		{
			if(caret < text.size())
				caret += TextOperations::getUnicodeCharacterSize(text[caret]);
			caret = std::min(caret, text.size());
			scrollToCaret();
			redraw();
			break;
		}

		case EShortcut::MOVE_UP:
			moveCaretVertically(-1);
			redraw();
			break;

		case EShortcut::MOVE_DOWN:
			moveCaretVertically(1);
			redraw();
			break;

		case EShortcut::MOVE_FIRST:
			caret = segments[segmentOfCaret()].begin;
			redraw();
			break;

		case EShortcut::MOVE_LAST:
			caret = segments[segmentOfCaret()].end;
			redraw();
			break;

		default:
			break;
	}
}

void CMultiLineEdit::onFocusGot()
{
	redraw();
}

void CMultiLineEdit::onFocusLost()
{
	composedText.clear();
	redraw();
}

void CMultiLineEdit::activate()
{
	CFocusable::activate();

#if !defined(VCMI_MOBILE)
	if(hasFocus())
		ENGINE->input().startTextInput(pos);
#endif
}

void CMultiLineEdit::deactivate()
{
	CFocusable::deactivate();

	if(hasFocus())
	{
#if defined(VCMI_MOBILE)
		removeFocus();
#else
		ENGINE->input().stopTextInput();
#endif
	}
}

void CMultiLineEdit::showAll(Canvas & to)
{
	to.drawColorBlended(pos, UiTheme::FIELD_FILL);
	to.drawBorder(pos, hasFocus() ? UiTheme::BORDER_HIGHLIGHT : UiTheme::BORDER);

	CanvasClipRectGuard guard(to, Rect(pos.x + 1, pos.y + 1, pos.w - 2, pos.h - 2));

	const int height = lineHeight();
	const size_t caretSegment = segmentOfCaret();
	const int last = std::min(static_cast<int>(segments.size()), firstSegment + visibleSegments());

	for(int i = firstSegment; i < last; ++i)
	{
		const Segment & segment = segments[i];
		const int y = pos.y + TEXT_MARGIN + (i - firstSegment) * height;

		std::string line = text.substr(segment.begin, segment.end - segment.begin);
		if(hasFocus() && caretSegment == static_cast<size_t>(i))
		{
			const size_t offset = caret - segment.begin;
			line.insert(offset, composedText + "_");
		}

		to.drawText(Point(pos.x + TEXT_MARGIN, y), UiTheme::FONT, UiTheme::TEXT, ETextAlignment::TOPLEFT, line);
	}
}
