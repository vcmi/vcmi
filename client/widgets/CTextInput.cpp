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
#include "TextControls.h"

#include "../GameEngine.h"
#include "../eventsSDL/InputHandler.h"
#include "../gui/Shortcut.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"

#include "../../lib/texts/TextOperations.h"

#include <boost/lexical_cast.hpp>

std::list<CFocusable *> CFocusable::focusables;
CFocusable * CFocusable::inputWithFocus;

CTextInput::CTextInput(const Rect & Pos)
	:originalAlignment(ETextAlignment::CENTERLEFT)
{
	pos += Pos.topLeft();
	pos.h = Pos.h;
	pos.w = Pos.w;

	addUsedEvents(LCLICK | SHOW_POPUP | KEYBOARD | TEXTINPUT);
}

void CTextInput::createLabel(bool giveFocusToInput)
{
	OBJECT_CONSTRUCTION;
	label = std::make_shared<CLabel>();
	label->pos = pos;
	label->alignment = originalAlignment;

#if !defined(VCMI_MOBILE)
	if(giveFocusToInput)
		giveFocus();
#endif
}

CTextInput::CTextInput(const Rect & Pos, EFonts font, ETextAlignment alignment, bool giveFocusToInput)
	: CTextInput(Pos)
{
	originalAlignment = alignment;
	setRedrawParent(true);
	createLabel(giveFocusToInput);
	setFont(font);
	setAlignment(alignment);
}

CTextInput::CTextInput(const Rect & Pos, const Point & bgOffset, const ImagePath & bgName)
	: CTextInput(Pos)
{
	OBJECT_CONSTRUCTION;
	if (!bgName.empty())
		background = std::make_shared<CPicture>(bgName, bgOffset.x, bgOffset.y);
	else
		setRedrawParent(true);

	createLabel(true);
}

CTextInput::CTextInput(const Rect & Pos, std::shared_ptr<IImage> srf)
	: CTextInput(Pos)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(srf, Pos);
	pos.w = background->pos.w;
	pos.h = background->pos.h;
	background->pos = pos;
	createLabel(true);
}

void CTextInput::setFont(EFonts font)
{
	label->font = font;
}

void CTextInput::setColor(const ColorRGBA & color)
{
	label->color = color;
}

void CTextInput::setAlignment(ETextAlignment alignment)
{
	originalAlignment = alignment;
	label->alignment = alignment;
}

const std::string & CTextInput::getText() const
{
	return currentText;
}

void CTextInput::setCallback(const TextEditedCallback & cb)
{
	assert(!onTextEdited);
	onTextEdited = cb;
}

void CTextInput::setPopupCallback(const std::function<void()> & cb)
{
	callbackPopup = cb;
}

void CTextInput::setFilterFilename()
{
	assert(!onTextFiltering);
	onTextFiltering = std::bind(&CTextInput::filenameFilter, _1, _2);
}

void CTextInput::setFilterNumber(int minValue, int maxValue)
{
	onTextFiltering = std::bind(&CTextInput::numberFilter, _1, _2, minValue, maxValue);
}

std::string CTextInput::getVisibleText() const
{
	return hasFocus() ? currentText + composedText + "_" : currentText;
}

void CTextInput::showPopupWindow(const Point & cursorPosition)
{
	if(callbackPopup)
		callbackPopup();
}

void CTextInput::clickPressed(const Point & cursorPosition)
{
	// attempt to give focus unconditionally, even if we already have it
	// this forces on-screen keyboard to show up again, even if player have closed it before
	giveFocus();
}

void CTextInput::keyPressed(EShortcut key)
{
	if(!hasFocus())
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
			if(!composedText.empty())
			{
				TextOperations::trimRightUnicode(composedText);
				redrawNeeded = true;
			}
			else if(!currentText.empty())
			{
				TextOperations::trimRightUnicode(currentText);
				redrawNeeded = true;
			}
			break;
		default:
			break;
	}

	if(redrawNeeded)
	{
		updateLabel();
		if(onTextEdited)
			onTextEdited(currentText);
	}
}

void CTextInput::setText(const std::string & nText)
{
	currentText = nText;
	updateLabel();
}

void CTextInput::updateLabel()
{
	std::string visibleText = getVisibleText();

	label->alignment = originalAlignment;
	const auto & font = ENGINE->renderHandler().loadFont(label->font);

	while (font->getStringWidth(visibleText) > pos.w)
	{
		label->alignment = ETextAlignment::CENTERRIGHT;
		visibleText = visibleText.substr(TextOperations::getUnicodeCharacterSize(visibleText[0]));
	}

	label->setText(visibleText);
}

void CTextInput::textInputted(const std::string & enteredText)
{
	if(!hasFocus())
		return;
	std::string oldText = currentText;

	setText(getText() + enteredText);

	if(onTextFiltering)
		onTextFiltering(currentText, oldText);

	if(currentText != oldText)
	{
		updateLabel();
		if(onTextEdited)
			onTextEdited(currentText);
	}
	composedText.clear();
}

void CTextInput::textEdited(const std::string & enteredText)
{
	if(!hasFocus())
		return;

	composedText = enteredText;
	updateLabel();
}

void CTextInput::filenameFilter(std::string & text, const std::string &oldText)
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

void CTextInput::activate()
{
	CFocusable::activate();
	if (hasFocus())
	{
#if defined(VCMI_MOBILE)
		//giveFocus();
#else
		ENGINE->input().startTextInput(pos);
#endif
	}
}

void CTextInput::deactivate()
{
	CFocusable::deactivate();
	if (hasFocus())
	{
#if defined(VCMI_MOBILE)
		removeFocus();
#else
		ENGINE->input().stopTextInput();
#endif
	}
}

void CTextInput::onFocusGot()
{
	updateLabel();
}

void CTextInput::onFocusLost()
{
	updateLabel();
}

void CFocusable::focusGot()
{
	if (isActive())
		ENGINE->input().startTextInput(pos);
	onFocusGot();
}

void CFocusable::focusLost()
{
	if (isActive())
		ENGINE->input().stopTextInput();
	onFocusLost();
}

CFocusable::CFocusable()
{
	focusables.push_back(this);
}

CFocusable::~CFocusable()
{
	if(hasFocus())
		inputWithFocus = nullptr;

	focusables -= this;
}

bool CFocusable::hasFocus() const
{
	return inputWithFocus == this;
}

void CFocusable::giveFocus()
{
	auto previousInput = inputWithFocus;
	inputWithFocus = this;

	if(previousInput)
		previousInput->focusLost();

	focusGot();
}

void CFocusable::moveFocus()
{
	auto i = vstd::find(focusables, this);
	auto ourIt = i;

	for(i++; i != ourIt; i++)
	{
		if(i == focusables.end())
			i = focusables.begin();

		if(*i == this)
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
		inputWithFocus = nullptr;
		focusLost();
	}
}
