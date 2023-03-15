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

#include "Buttons.h"
#include "Images.h"

#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../windows/CMessage.h"
#include "../adventureMap/CInGameConsole.h"
#include "../renderSDL/SDL_Extensions.h"

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

void CLabel::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	if(!visibleText().empty())
		blitLine(to, pos, visibleText());

}

CLabel::CLabel(int x, int y, EFonts Font, ETextAlignment Align, const SDL_Color & Color, const std::string & Text)
	: CTextContainer(Align, Font, Color), text(Text)
{
	type |= REDRAW_PARENT;
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

void CLabel::setColor(const SDL_Color & Color)
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

CMultiLineLabel::CMultiLineLabel(Rect position, EFonts Font, ETextAlignment Align, const SDL_Color & Color, const std::string & Text) :
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

void CTextContainer::blitLine(SDL_Surface * to, Rect destRect, std::string what)
{
	const auto f = graphics->fonts[font];
	Point where = destRect.topLeft();

	// input is rect in which given text should be placed
	// calculate proper position for top-left corner of the text
	if(alignment == ETextAlignment::TOPLEFT)
	{
		where.x += getBorderSize().x;
		where.y += getBorderSize().y;
	}

	if(alignment == ETextAlignment::CENTER)
	{
		where.x += (int(destRect.w) - int(f->getStringWidth(what))) / 2;
		where.y += (int(destRect.h) - int(f->getLineHeight())) / 2;
	}

	if(alignment == ETextAlignment::BOTTOMRIGHT)
	{
		where.x += getBorderSize().x + destRect.w - (int)f->getStringWidth(what);
		where.y += getBorderSize().y + destRect.h - (int)f->getLineHeight();
	}

	size_t begin = 0;
	std::string delimeters = "{}";
	size_t currDelimeter = 0;

	do
	{
		size_t end = what.find_first_of(delimeters[currDelimeter % 2], begin);
		if(begin != end)
		{
			std::string toPrint = what.substr(begin, end - begin);

			if(currDelimeter % 2) // Enclosed in {} text - set to yellow
				f->renderTextLeft(to, toPrint, Colors::YELLOW, where);
			else // Non-enclosed text, use default color
				f->renderTextLeft(to, toPrint, color, where);
			begin = end;

			where.x += (int)f->getStringWidth(toPrint);
		}
		currDelimeter++;
	} while(begin++ != std::string::npos);
}

CTextContainer::CTextContainer(ETextAlignment alignment, EFonts font, SDL_Color color) :
	alignment(alignment),
	font(font),
	color(color)
{
}

void CMultiLineLabel::showAll(SDL_Surface * to)
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

	CSDL_Ext::CClipRectGuard guard(to, getTextLocation()); // to properly trim text that is too big to fit

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
	case ETextAlignment::CENTER:      return Rect(pos.topLeft() + textOffset / 2, textSize);
	case ETextAlignment::BOTTOMRIGHT: return Rect(pos.topLeft() + textOffset, textSize);
	}
	assert(0);
	return Rect();
}

CLabelGroup::CLabelGroup(EFonts Font, ETextAlignment Align, const SDL_Color & Color)
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

CTextBox::CTextBox(std::string Text, const Rect & rect, int SliderStyle, EFonts Font, ETextAlignment Align, const SDL_Color & Color) :
	sliderStyle(SliderStyle),
	slider(nullptr)
{
	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	label = std::make_shared<CMultiLineLabel>(rect, Font, Align, Color);

	type |= REDRAW_PARENT;
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
		label->pos.w = pos.w - 32;
		label->setText(text);
		slider->setAmount(label->textSize.y);
	}
	else if(label->textSize.y > label->pos.h)
	{
		// create slider and update widget
		label->pos.w = pos.w - 32;
		label->setText(text);

		OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255 - DISPOSE);
		slider = std::make_shared<CSlider>(Point(pos.w - 32, 0), pos.h, std::bind(&CTextBox::sliderMoved, this, _1),
			label->pos.h, label->textSize.y, 0, false, CSlider::EStyle(sliderStyle));
		slider->setScrollStep((int)graphics->fonts[label->font]->getLineHeight());
	}
}

void CGStatusBar::setEnteringMode(bool on)
{
	consoleText.clear();

	if (on)
	{
		assert(enteringText == false);
		alignment = ETextAlignment::TOPLEFT;
		GH.startTextInput(pos);
		setText(consoleText);
	}
	else
	{
		assert(enteringText == true);
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

CGStatusBar::CGStatusBar(std::shared_ptr<CPicture> background_, EFonts Font, ETextAlignment Align, const SDL_Color & Color)
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

CGStatusBar::CGStatusBar(int x, int y, std::string name, int maxw)
	: CLabel(x, y, FONT_SMALL, ETextAlignment::CENTER)
	, enteringText(false)
{
	addUsedEvents(LCLICK);

	OBJECT_CONSTRUCTION_CAPTURING(255 - DISPOSE);
	background = std::make_shared<CPicture>(name);
	pos = background->pos;

	if((unsigned)maxw < (unsigned)pos.w) //(insigned)-1 > than any correct value of pos.w
	{
		//execution of this block when maxw is incorrect breaks text centralization (issue #3151)
		vstd::amin(pos.w, maxw);
		background->srcRect = Rect(0, 0, maxw, pos.h);
	}
	autoRedraw = false;
}

void CGStatusBar::show(SDL_Surface * to)
{
	showAll(to);
}

void CGStatusBar::init()
{
	GH.statusbar = shared_from_this();
}

void CGStatusBar::clickLeft(tribool down, bool previousState)
{
	if(!down)
	{
		if(LOCPLINT && LOCPLINT->cingconsole->active)
			LOCPLINT->cingconsole->startEnteringText();
	}
}

void CGStatusBar::deactivate()
{
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
	case ETextAlignment::CENTER:      return Point(pos.w / 2, pos.h / 2);
	case ETextAlignment::BOTTOMRIGHT: return Point(pos.w - borderSize.x, pos.h - borderSize.y);
	}
	assert(0);
	return Point();
}

CTextInput::CTextInput(const Rect & Pos, EFonts font, const CFunctionList<void(const std::string &)> & CB)
	: CLabel(Pos.x, Pos.y, font, ETextAlignment::CENTER),
	cb(CB),
	CFocusable(std::make_shared<CKeyboardFocusListener>(this))
{
	type |= REDRAW_PARENT;
	pos.h = Pos.h;
	pos.w = Pos.w;
	captureAllKeys = true;
	background.reset();
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	giveFocus();
#endif
}

CTextInput::CTextInput(const Rect & Pos, const Point & bgOffset, const std::string & bgName, const CFunctionList<void(const std::string &)> & CB)
	:cb(CB), 	CFocusable(std::make_shared<CKeyboardFocusListener>(this))
{
	pos += Pos.topLeft();
	pos.h = Pos.h;
	pos.w = Pos.w;

	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>(bgName, bgOffset.x, bgOffset.y);
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);

#if !defined(VCMI_MOBILE)
	giveFocus();
#endif
}

CTextInput::CTextInput(const Rect & Pos, std::shared_ptr<IImage> srf)
	:CFocusable(std::make_shared<CKeyboardFocusListener>(this))
{
	pos += Pos.topLeft();
	captureAllKeys = true;
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

void CTextInput::clickLeft(tribool down, bool previousState)
{
	if(down && !focus)
		giveFocus();
}

void CTextInput::keyPressed(const SDL_Keycode & key)
{
	if(!focus)
		return;

	if(key == SDLK_TAB)
	{
		moveFocus();
		GH.breakEventHandling();
		return;
	}

	bool redrawNeeded = false;

	switch(key)
	{
	case SDLK_DELETE: // have index > ' ' so it won't be filtered out by default section
		return;
	case SDLK_BACKSPACE:
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

bool CTextInput::captureThisKey(const SDL_Keycode & key)
{
	if(key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_ESCAPE)
		return false;

	return true;
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

		if((*i)->active)
		{
			(*i)->giveFocus();
			break;
		}
	}
}
