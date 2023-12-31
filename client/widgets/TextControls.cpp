/*
 * TextControls.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TextControls.h"

#include "Slider.h"
#include "Images.h"

#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../windows/CMessage.h"
#include "../windows/InfoWindows.h"
#include "../adventureMap/CInGameConsole.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../render/Canvas.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"

#include "../../lib/TextOperations.h"

#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#endif

std::list<CFocusable*> CFocusable::focusables;
CFocusable * CFocusable::inputWithFocus;

std::string CLabel::visibleText()
{
	return text;
}

void CLabel::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	if(!visibleText().empty())
		blitLine(to, pos, visibleText());

}

CLabel::CLabel(int x, int y, EFonts Font, ETextAlignment Align, const ColorRGBA & Color, const std::string & Text)
	: CTextContainer(Align, Font, Color), text(Text)
{
	setRedrawParent(true);
	autoRedraw = true;
	pos.x += x;
	pos.y += y;
	pos.w = pos.h = 0;

	if(alignment == ETextAlignment::TOPLEFT) // causes issues for MIDDLE
	{
		pos.w = (int)graphics->fonts[font]->getStringWidth(visibleText().c_str());
		pos.h = (int)graphics->fonts[font]->getLineHeight();
	}
}

Point CLabel::getBorderSize()
{
	return Point(0, 0);
}

std::string CLabel::getText()
{
	return text;
}

void CLabel::setAutoRedraw(bool value)
{
	autoRedraw = value;
}

void CLabel::setText(const std::string & Txt)
{
	text = Txt;
	if(autoRedraw)
	{
		if(background || !parent)
			redraw();
		else
			parent->redraw();
	}
}

void CLabel::setColor(const ColorRGBA & Color)
{
	color = Color;
	if(autoRedraw)
	{
		if(background || !parent)
			redraw();
		else
			parent->redraw();
	}
}

size_t CLabel::getWidth()
{
	return graphics->fonts[font]->getStringWidth(visibleText());;
}

CMultiLineLabel::CMultiLineLabel(Rect position, EFonts Font, ETextAlignment Align, const ColorRGBA & Color, const std::string & Text) :
	CLabel(position.x, position.y, Font, Align, Color, Text),
	visibleSize(0, 0, position.w, position.h)
{
	pos.w = position.w;
	pos.h = position.h;
	splitText(Text, true);
}

void CMultiLineLabel::setVisibleSize(Rect visibleSize, bool redrawElement)
{
	this->visibleSize = visibleSize;
	if(redrawElement)
		redraw();
}

void CMultiLineLabel::scrollTextBy(int distance)
{
	scrollTextTo(visibleSize.y + distance);
}

void CMultiLineLabel::scrollTextTo(int distance, bool redrawAfterScroll)
{
	Rect size = visibleSize;
	size.y = distance;
	setVisibleSize(size, redrawAfterScroll);
}

void CMultiLineLabel::setText(const std::string & Txt)
{
	splitText(Txt, false); //setText used below can handle redraw
	CLabel::setText(Txt);
}

void CTextContainer::blitLine(Canvas & to, Rect destRect, std::string what)
{
	const auto f = graphics->fonts[font];
	Point where = destRect.topLeft();
	const std::string delimeters = "{}";
	auto delimitersCount = std::count_if(what.cbegin(), what.cend(), [&delimeters](char c)
	{
		return delimeters.find(c) != std::string::npos;
	});
	//We should count delimiters length from string to correct centering later.
	delimitersCount *= f->getStringWidth(delimeters)/2;

	std::smatch match;
	std::regex expr("\\{(.*?)\\|");
	std::string::const_iterator searchStart( what.cbegin() );
	while(std::regex_search(searchStart, what.cend(), match, expr))
	{
		std::string colorText = match[1].str();
		if(auto c = Colors::parseColor(colorText))
			delimitersCount += f->getStringWidth(colorText + "|");
		searchStart = match.suffix().first;
	}

	// input is rect in which given text should be placed
	// calculate proper position for top-left corner of the text
	if(alignment == ETextAlignment::TOPLEFT)
	{
		where.x += getBorderSize().x;
		where.y += getBorderSize().y;
	}

	if(alignment == ETextAlignment::TOPCENTER)
	{
		where.x += (int(destRect.w) - int(f->getStringWidth(what) - delimitersCount)) / 2;
		where.y += getBorderSize().y;
	}

	if(alignment == ETextAlignment::CENTER)
	{
		where.x += (int(destRect.w) - int(f->getStringWidth(what) - delimitersCount)) / 2;
		where.y += (int(destRect.h) - int(f->getLineHeight())) / 2;
	}

	if(alignment == ETextAlignment::BOTTOMRIGHT)
	{
		where.x += getBorderSize().x + destRect.w - ((int)f->getStringWidth(what) - delimitersCount);
		where.y += getBorderSize().y + destRect.h - (int)f->getLineHeight();
	}

	size_t begin = 0;
	size_t currDelimeter = 0;

	do
	{
		size_t end = what.find_first_of(delimeters[currDelimeter % 2], begin);
		if(begin != end)
		{
			std::string toPrint = what.substr(begin, end - begin);

			if(currDelimeter % 2) // Enclosed in {} text - set to yellow or defined color
			{
				std::smatch match;
   				std::regex expr("^(.*?)\\|");
				if(std::regex_search(toPrint, match, expr))
				{
					std::string colorText = match[1].str();
					
					if(auto color = Colors::parseColor(colorText))
					{
						toPrint = toPrint.substr(colorText.length() + 1, toPrint.length() - colorText.length());
						to.drawText(where, font, *color, ETextAlignment::TOPLEFT, toPrint);
					}
					else
						to.drawText(where, font, Colors::YELLOW, ETextAlignment::TOPLEFT, toPrint);
				}
				else
					to.drawText(where, font, Colors::YELLOW, ETextAlignment::TOPLEFT, toPrint);
			}
			else // Non-enclosed text, use default color
				to.drawText(where, font, color, ETextAlignment::TOPLEFT, toPrint);

			begin = end;

			where.x += (int)f->getStringWidth(toPrint);
		}
		currDelimeter++;
	} while(begin++ != std::string::npos);
}

CTextContainer::CTextContainer(ETextAlignment alignment, EFonts font, ColorRGBA color) :
	alignment(alignment),
	font(font),
	color(color)
{
}

void CMultiLineLabel::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	const auto f = graphics->fonts[font];

	// calculate which lines should be visible
	int totalLines = static_cast<int>(lines.size());
	int beginLine = visibleSize.y;
	int endLine = getTextLocation().h + visibleSize.y;

	if(beginLine < 0)
		beginLine = 0;
	else
		beginLine /= (int)f->getLineHeight();

	if(endLine < 0)
		endLine = 0;
	else
		endLine /= (int)f->getLineHeight();
	endLine++;

	// and where they should be displayed
	Point lineStart = getTextLocation().topLeft() - visibleSize + Point(0, beginLine * (int)f->getLineHeight());
	Point lineSize = Point(getTextLocation().w, (int)f->getLineHeight());

	CSDL_Ext::CClipRectGuard guard(to.getInternalSurface(), getTextLocation()); // to properly trim text that is too big to fit

	for(int i = beginLine; i < std::min(totalLines, endLine); i++)
	{
		if(!lines[i].empty()) //non-empty line
			blitLine(to, Rect(lineStart, lineSize), lines[i]);

		lineStart.y += (int)f->getLineHeight();
	}
}

void CMultiLineLabel::splitText(const std::string & Txt, bool redrawAfter)
{
	lines.clear();

	const auto f = graphics->fonts[font];
	int lineHeight = static_cast<int>(f->getLineHeight());

	lines = CMessage::breakText(Txt, pos.w, font);

	textSize.y = lineHeight * (int)lines.size();
	textSize.x = 0;
	for(const std::string & line : lines)
		vstd::amax(textSize.x, f->getStringWidth(line.c_str()));
	if(redrawAfter)
		redraw();
}

Rect CMultiLineLabel::getTextLocation()
{
	// this method is needed for vertical alignment alignment of text
	// when height of available text is smaller than height of widget
	// in this case - we should add proper offset to display text at required position
	if(pos.h <= textSize.y)
		return pos;

	Point textSize(pos.w, (int)graphics->fonts[font]->getLineHeight() * (int)lines.size());
	Point textOffset(pos.w - textSize.x, pos.h - textSize.y);

	switch(alignment)
	{
	case ETextAlignment::TOPLEFT:     return Rect(pos.topLeft(), textSize);
	case ETextAlignment::TOPCENTER:   return Rect(pos.topLeft(), textSize);
	case ETextAlignment::CENTER:      return Rect(pos.topLeft() + textOffset / 2, textSize);
	case ETextAlignment::BOTTOMRIGHT: return Rect(pos.topLeft() + textOffset, textSize);
	}
	assert(0);
	return Rect();
}

CLabelGroup::CLabelGroup(EFonts Font, ETextAlignment Align, const ColorRGBA & Color)
	: font(Font), align(Align), color(Color)
{
	defActions = 255 - DISPOSE;
}

void CLabelGroup::add(int x, int y, const std::string & text)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);
	labels.push_back(std::make_shared<CLabel>(x, y, font, align, color, text));
}

size_t CLabelGroup::currentSize() const
{
	return labels.size();
}

CTextBox::CTextBox(std::string Text, const Rect & rect, int SliderStyle, EFonts Font, ETextAlignment Align, const ColorRGBA & Color) :
	sliderStyle(SliderStyle),
	slider(nullptr)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	label = std::make_shared<CMultiLineLabel>(rect, Font, Align, Color);

	setRedrawParent(true);
	pos.x += rect.x;
	pos.y += rect.y;
	pos.h = rect.h;
	pos.w = rect.w;

	assert(pos.w >= 40); //we need some space
	setText(Text);
}

void CTextBox::sliderMoved(int to)
{
	label->scrollTextTo(to);
}

void CTextBox::resize(Point newSize)
{
	pos.w = newSize.x;
	pos.h = newSize.y;
	label->pos.w = pos.w;
	label->pos.h = pos.h;

	slider.reset();
	setText(label->getText()); // force refresh
}

void CTextBox::setText(const std::string & text)
{
	label->pos.w = pos.w; // reset to default before textSize.y check
	label->setText(text);
	if(label->textSize.y <= label->pos.h && slider)
	{
		// slider is no longer needed
		slider.reset();
	}
	else if(slider)
	{
		// decrease width again if slider still used
		label->pos.w = pos.w - 16;
		assert(label->pos.w > 0);
		label->setText(text);
		slider->setAmount(label->textSize.y);
	}
	else if(label->textSize.y > label->pos.h)
	{
		// create slider and update widget
		label->pos.w = pos.w - 16;
		assert(label->pos.w > 0);
		label->setText(text);

		OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);
		slider = std::make_shared<CSlider>(Point(pos.w - 16, 0), pos.h, std::bind(&CTextBox::sliderMoved, this, _1),
			label->pos.h, label->textSize.y, 0, Orientation::VERTICAL, CSlider::EStyle(sliderStyle));
		slider->setScrollStep((int)graphics->fonts[label->font]->getLineHeight());
		slider->setPanningStep(1);
		slider->setScrollBounds(pos - slider->pos.topLeft());
	}
}

void CGStatusBar::setEnteringMode(bool on)
{
	consoleText.clear();

	if (on)
	{
		//assert(enteringText == false);
		alignment = ETextAlignment::TOPLEFT;
		GH.startTextInput(pos);
		setText(consoleText);
	}
	else
	{
		//assert(enteringText == true);
		alignment = ETextAlignment::CENTER;
		GH.stopTextInput();
		setText(hoverText);
	}
	enteringText = on;
}

void CGStatusBar::setEnteredText(const std::string & text)
{
	assert(enteringText == true);
	consoleText = text;
	setText(text);
}

void CGStatusBar::write(const std::string & Text)
{
	hoverText = Text;

	if (enteringText == false)
		setText(hoverText);
}

void CGStatusBar::clearIfMatching(const std::string & Text)
{
	if (hoverText == Text)
		clear();
}

void CGStatusBar::clear()
{
	write({});
}

CGStatusBar::CGStatusBar(std::shared_ptr<CIntObject> background_, EFonts Font, ETextAlignment Align, const ColorRGBA & Color)
	: CLabel(background_->pos.x, background_->pos.y, Font, Align, Color, "")
	, enteringText(false)
{
	addUsedEvents(LCLICK);

	background = background_;
	addChild(background.get());
	pos = background->pos;
	getBorderSize();
	autoRedraw = false;
}

CGStatusBar::CGStatusBar(int x, int y)
	: CLabel(x, y, FONT_SMALL, ETextAlignment::CENTER)
	, enteringText(false)
{
	 // without background
	addUsedEvents(LCLICK);
}

CGStatusBar::CGStatusBar(int x, int y, const ImagePath & name, int maxw)
	: CLabel(x, y, FONT_SMALL, ETextAlignment::CENTER)
	, enteringText(false)
{
	addUsedEvents(LCLICK);

	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);

	auto backgroundImage = std::make_shared<CPicture>(name);
	background = backgroundImage;
	pos = background->pos;

	if((unsigned)maxw < (unsigned)pos.w) //(insigned)-1 > than any correct value of pos.w
	{
		//execution of this block when maxw is incorrect breaks text centralization (issue #3151)
		vstd::amin(pos.w, maxw);
		backgroundImage->srcRect = Rect(0, 0, maxw, pos.h);
	}
	autoRedraw = false;
}

CGStatusBar::~CGStatusBar()
{
	assert(GH.statusbar().get() != this);
}

void CGStatusBar::show(Canvas & to)
{
	showAll(to);
}

void CGStatusBar::clickPressed(const Point & cursorPosition)
{
	if(LOCPLINT && LOCPLINT->cingconsole->isActive())
		LOCPLINT->cingconsole->startEnteringText();
}

void CGStatusBar::activate()
{
	GH.setStatusbar(shared_from_this());
	CIntObject::activate();
}

void CGStatusBar::deactivate()
{
	assert(GH.statusbar().get() == this);
	GH.setStatusbar(nullptr);

	if (enteringText)
		LOCPLINT->cingconsole->endEnteringText(false);

	CIntObject::deactivate();
}

Point CGStatusBar::getBorderSize()
{
	//Width of borders where text should not be printed
	static const Point borderSize(5, 1);

	switch(alignment)
	{
	case ETextAlignment::TOPLEFT:     return Point(borderSize.x, borderSize.y);
	case ETextAlignment::TOPCENTER:   return Point(pos.w / 2, borderSize.y);
	case ETextAlignment::CENTER:      return Point(pos.w / 2, pos.h / 2);
	case ETextAlignment::BOTTOMRIGHT: return Point(pos.w - borderSize.x, pos.h - borderSize.y);
	}
	assert(0);
	return Point();
}

CTextInput::CTextInput(const Rect & Pos, EFonts font, const CFunctionList<void(const std::string &)> & CB, bool giveFocusToInput)
	: CLabel(Pos.x, Pos.y, font, ETextAlignment::CENTER),
	cb(CB),
	CFocusable(std::make_shared<CKeyboardFocusListener>(this))
{
	setRedrawParent(true);
	pos.h = Pos.h;
	pos.w = Pos.w;
	background.reset();
	addUsedEvents(LCLICK | SHOW_POPUP | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	if(giveFocusToInput)
		giveFocus();
#endif
}

CTextInput::CTextInput(const Rect & Pos, const Point & bgOffset, const ImagePath & bgName, const CFunctionList<void(const std::string &)> & CB)
	:cb(CB), 	CFocusable(std::make_shared<CKeyboardFocusListener>(this))
{
	pos += Pos.topLeft();
	pos.h = Pos.h;
	pos.w = Pos.w;

	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>(bgName, bgOffset.x, bgOffset.y);
	addUsedEvents(LCLICK | SHOW_POPUP | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	giveFocus();
#endif
}

CTextInput::CTextInput(const Rect & Pos, std::shared_ptr<IImage> srf)
	:CFocusable(std::make_shared<CKeyboardFocusListener>(this))
{
	pos += Pos.topLeft();
	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>(srf, Pos);
	pos.w = background->pos.w;
	pos.h = background->pos.h;
	background->pos = pos;
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	giveFocus();
#endif
}

std::atomic<int> CKeyboardFocusListener::usageIndex(0);

CKeyboardFocusListener::CKeyboardFocusListener(CTextInput * textInput)
	:textInput(textInput)
{
}

void CKeyboardFocusListener::focusGot()
{
	GH.startTextInput(textInput->pos);
	usageIndex++;
}

void CKeyboardFocusListener::focusLost()
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

	text += enteredText;

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
	:CFocusable(std::make_shared<IFocusListener>())
{
}

CFocusable::CFocusable(std::shared_ptr<IFocusListener> focusListener)
	: focusListener(focusListener)
{
	focus = false;
	focusables.push_back(this);
}

CFocusable::~CFocusable()
{
	if(hasFocus())
	{
		inputWithFocus = nullptr;
		focusListener->focusLost();
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
	focusListener->focusGot();
	redraw();

	if(inputWithFocus)
	{
		inputWithFocus->focus = false;
		inputWithFocus->focusListener->focusLost();
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
		focusListener->focusLost();
		redraw();

		inputWithFocus = nullptr;
	}
}
