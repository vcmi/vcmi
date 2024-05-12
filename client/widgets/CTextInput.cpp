/*
 * CTextInput.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTextInput.h"

#include "Images.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../windows/InfoWindows.h"

#include "../../lib/TextOperations.h"

std::list<CFocusable*> CFocusable::focusables;
CFocusable * CFocusable::inputWithFocus;

CTextInput::CTextInput(const Rect & Pos, EFonts font, const CFunctionList<void(const std::string &)> & CB, ETextAlignment alignment, bool giveFocusToInput)
	: CLabel(Pos.x, Pos.y, font, alignment),
	cb(CB)
{
	setRedrawParent(true);
	pos.h = Pos.h;
	pos.w = Pos.w;
	maxWidth = Pos.w;
	background.reset();
	addUsedEvents(LCLICK | SHOW_POPUP | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	if(giveFocusToInput)
		giveFocus();
#endif
}

CTextInput::CTextInput(const Rect & Pos, const Point & bgOffset, const ImagePath & bgName, const CFunctionList<void(const std::string &)> & CB)
	:cb(CB)
{
	pos += Pos.topLeft();
	pos.h = Pos.h;
	pos.w = Pos.w;
	maxWidth = Pos.w;

	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>(bgName, bgOffset.x, bgOffset.y);
	addUsedEvents(LCLICK | SHOW_POPUP | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	giveFocus();
#endif
}

CTextInput::CTextInput(const Rect & Pos, std::shared_ptr<IImage> srf)
{
	pos += Pos.topLeft();
	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>(srf, Pos);
	pos.w = background->pos.w;
	pos.h = background->pos.h;
	maxWidth = Pos.w;
	background->pos = pos;
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	giveFocus();
#endif
}

std::atomic<int> CFocusable::usageIndex(0);

void CFocusable::focusGot()
{
	GH.startTextInput(pos);
	usageIndex++;
}

void CFocusable::focusLost()
{
	if(0 == --usageIndex)
	{
		GH.stopTextInput();
	}
}

std::string CTextInput::visibleText()
{
	return focus ? text + newText + "_" : text;
}

void CTextInput::clickPressed(const Point & cursorPosition)
{
	if(!focus)
		giveFocus();
}

void CTextInput::keyPressed(EShortcut key)
{
	if(!focus)
		return;

	if(key == EShortcut::GLOBAL_MOVE_FOCUS)
	{
		moveFocus();
		return;
	}

	bool redrawNeeded = false;

	switch(key)
	{
		case EShortcut::GLOBAL_BACKSPACE:
			if(!newText.empty())
			{
				TextOperations::trimRightUnicode(newText);
				redrawNeeded = true;
			}
			else if(!text.empty())
			{
				TextOperations::trimRightUnicode(text);
				redrawNeeded = true;
			}
			break;
		default:
			break;
	}

	if(redrawNeeded)
	{
		redraw();
		cb(text);
	}
}

void CTextInput::showPopupWindow(const Point & cursorPosition)
{
	if(!helpBox.empty()) //there is no point to show window with nothing inside...
		CRClickPopup::createAndPush(helpBox);
}


void CTextInput::setText(const std::string & nText)
{
	setText(nText, false);
}

void CTextInput::setText(const std::string & nText, bool callCb)
{
	CLabel::setText(nText);
	if(callCb)
		cb(text);
}

void CTextInput::setHelpText(const std::string & text)
{
	helpBox = text;
}

void CTextInput::textInputed(const std::string & enteredText)
{
	if(!focus)
		return;
	std::string oldText = text;

	setText(getText() + enteredText);

	filters(text, oldText);
	if(text != oldText)
	{
		redraw();
		cb(text);
	}
	newText.clear();
}

void CTextInput::textEdited(const std::string & enteredText)
{
	if(!focus)
		return;

	newText = enteredText;
	redraw();
	cb(text + newText);
}

void CTextInput::filenameFilter(std::string & text, const std::string &)
{
	static const std::string forbiddenChars = "<>:\"/\\|?*\r\n"; //if we are entering a filename, some special characters won't be allowed
	size_t pos;
	while((pos = text.find_first_of(forbiddenChars)) != std::string::npos)
		text.erase(pos, 1);
}

void CTextInput::numberFilter(std::string & text, const std::string & oldText, int minValue, int maxValue)
{
	assert(minValue < maxValue);

	if(text.empty())
		text = "0";

	size_t pos = 0;
	if(text[0] == '-') //allow '-' sign as first symbol only
		pos++;

	while(pos < text.size())
	{
		if(text[pos] < '0' || text[pos] > '9')
		{
			text = oldText;
			return; //new text is not number.
		}
		pos++;
	}
	try
	{
		int value = boost::lexical_cast<int>(text);
		if(value < minValue)
			text = std::to_string(minValue);
		else if(value > maxValue)
			text = std::to_string(maxValue);
	}
	catch(boost::bad_lexical_cast &)
	{
		//Should never happen. Unless I missed some cases
		logGlobal->warn("Warning: failed to convert %s to number!", text);
		text = oldText;
	}
}

CFocusable::CFocusable()
{
	focus = false;
	focusables.push_back(this);
}

CFocusable::~CFocusable()
{
	if(hasFocus())
	{
		inputWithFocus = nullptr;
		focusLost();
	}

	focusables -= this;
}

bool CFocusable::hasFocus() const
{
	return inputWithFocus == this;
}

void CFocusable::giveFocus()
{
	focus = true;
	focusGot();
	redraw();

	if(inputWithFocus)
	{
		inputWithFocus->focus = false;
		inputWithFocus->focusLost();
		inputWithFocus->redraw();
	}

	inputWithFocus = this;
}

void CFocusable::moveFocus()
{
	auto i = vstd::find(focusables, this),
		ourIt = i;
	for(i++; i != ourIt; i++)
	{
		if(i == focusables.end())
			i = focusables.begin();

		if (*i == this)
			return;

		if((*i)->isActive())
		{
			(*i)->giveFocus();
			break;
		}
	}
}

void CFocusable::removeFocus()
{
	if(this == inputWithFocus)
	{
		focus = false;
		focusLost();
		redraw();

		inputWithFocus = nullptr;
	}
}
