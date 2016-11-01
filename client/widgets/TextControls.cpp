#include "StdInc.h"
#include "TextControls.h"

#include "Buttons.h"
#include "Images.h"

#include "../CMessage.h"
#include "../gui/CGuiHandler.h"

#include "../../lib/CGeneralTextHandler.h" //for Unicode related stuff

/*
 * TextControls.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

CLabel::CLabel(int x, int y, EFonts Font /*= FONT_SMALL*/, EAlignment Align, const SDL_Color &Color /*= Colors::WHITE*/, const std::string &Text /*= ""*/)
:CTextContainer(Align, Font, Color), text(Text)
{
	type |= REDRAW_PARENT;
	autoRedraw = true;
	pos.x += x;
	pos.y += y;
	pos.w = pos.h = 0;
	bg = nullptr;

	if (alignment == TOPLEFT) // causes issues for MIDDLE
	{
		pos.w = graphics->fonts[font]->getStringWidth(visibleText().c_str());
		pos.h = graphics->fonts[font]->getLineHeight();
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

void CLabel::setText(const std::string &Txt)
{
	text = Txt;
	if(autoRedraw)
	{
		if(bg || !parent)
			redraw();
		else
			parent->redraw();
	}
}

CMultiLineLabel::CMultiLineLabel(Rect position, EFonts Font, EAlignment Align, const SDL_Color &Color, const std::string &Text):
	CLabel(position.x, position.y, Font, Align, Color, Text),
	visibleSize(0, 0, position.w, position.h)
{
	pos.w = position.w;
	pos.h = position.h;
	splitText(Text);
}

void CMultiLineLabel::setVisibleSize(Rect visibleSize)
{
	this->visibleSize = visibleSize;
	redraw();
}

void CMultiLineLabel::scrollTextBy(int distance)
{
	scrollTextTo(visibleSize.y + distance);
}

void CMultiLineLabel::scrollTextTo(int distance)
{
	Rect size = visibleSize;
	size.y = distance;
	setVisibleSize(size);
}

void CMultiLineLabel::setText(const std::string &Txt)
{
	splitText(Txt);
	CLabel::setText(Txt);
}

void CTextContainer::blitLine(SDL_Surface *to, Rect destRect, std::string what)
{
	const IFont * f = graphics->fonts[font];
	Point where = destRect.topLeft();

	// input is rect in which given text should be placed
	// calculate proper position for top-left corner of the text
	if (alignment == TOPLEFT)
	{
		where.x += getBorderSize().x;
		where.y += getBorderSize().y;
	}

	if (alignment == CENTER)
	{
		where.x += (int(destRect.w) - int(f->getStringWidth(what))) / 2;
		where.y += (int(destRect.h) - int(f->getLineHeight())) / 2;
	}

	if (alignment == BOTTOMRIGHT)
	{
		where.x += getBorderSize().x + destRect.w - f->getStringWidth(what);
		where.y += getBorderSize().y + destRect.h - f->getLineHeight();
	}

	size_t begin = 0;
	std::string delimeters = "{}";
	size_t currDelimeter = 0;

	do
	{
		size_t end = what.find_first_of(delimeters[currDelimeter % 2], begin);
		if (begin != end)
		{
			std::string toPrint = what.substr(begin, end - begin);

			if (currDelimeter % 2) // Enclosed in {} text - set to yellow
				f->renderTextLeft(to, toPrint, Colors::YELLOW, where);
			else // Non-enclosed text, use default color
				f->renderTextLeft(to, toPrint, color, where);
			begin = end;

			where.x += f->getStringWidth(toPrint);
		}
		currDelimeter++;
	}
	while (begin++ != std::string::npos);
}

CTextContainer::CTextContainer(EAlignment alignment, EFonts font, SDL_Color color):
	alignment(alignment),
	font(font),
	color(color)
{}

void CMultiLineLabel::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	const IFont * f = graphics->fonts[font];

	// calculate which lines should be visible
	int totalLines = lines.size();
	int beginLine  = visibleSize.y;
	int endLine    = getTextLocation().h + visibleSize.y;

	if (beginLine < 0)
		beginLine = 0;
	else
		beginLine /= f->getLineHeight();

	if (endLine < 0)
		endLine = 0;
	else
		endLine /= f->getLineHeight();
	endLine++;

	// and where they should be displayed
	Point lineStart = getTextLocation().topLeft() - visibleSize + Point(0, beginLine * f->getLineHeight());
	Point lineSize  = Point(getTextLocation().w, f->getLineHeight());

	CSDL_Ext::CClipRectGuard guard(to, getTextLocation()); // to properly trim text that is too big to fit

	for (int i = beginLine; i < std::min(totalLines, endLine); i++)
	{
		if (!lines[i].empty()) //non-empty line
			blitLine(to, Rect(lineStart, lineSize), lines[i]);

		lineStart.y += f->getLineHeight();
	}
}

void CMultiLineLabel::splitText(const std::string &Txt)
{
	lines.clear();

	const IFont * f = graphics->fonts[font];
	int lineHeight =  f->getLineHeight();

	lines = CMessage::breakText(Txt, pos.w, font);

	 textSize.y = lineHeight * lines.size();
	 textSize.x = 0;
	for(const std::string &line : lines)
		vstd::amax( textSize.x, f->getStringWidth(line.c_str()));
	redraw();
}

Rect CMultiLineLabel::getTextLocation()
{
	// this method is needed for vertical alignment alignment of text
	// when height of available text is smaller than height of widget
	// in this case - we should add proper offset to display text at required position
	if (pos.h <= textSize.y)
		return pos;

	Point textSize(pos.w, graphics->fonts[font]->getLineHeight() * lines.size());
	Point textOffset(pos.w - textSize.x, pos.h - textSize.y);

	switch(alignment)
	{
	case TOPLEFT:     return Rect(pos.topLeft(), textSize);
	case CENTER:      return Rect(pos.topLeft() + textOffset / 2, textSize);
	case BOTTOMRIGHT: return Rect(pos.topLeft() + textOffset, textSize);
	}
	assert(0);
	return Rect();
}

CLabelGroup::CLabelGroup(EFonts Font, EAlignment Align, const SDL_Color &Color):
	font(Font), align(Align), color(Color)
{}

void CLabelGroup::add(int x, int y, const std::string &text)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CLabel(x, y, font, align, color, text);
}

CTextBox::CTextBox(std::string Text, const Rect &rect, int SliderStyle, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= TOPLEFT*/, const SDL_Color &Color /*= Colors::WHITE*/):
	sliderStyle(SliderStyle),
	slider(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	label = new CMultiLineLabel(rect, Font, Align, Color);

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
	if (slider)
		vstd::clear_pointer(slider); // will be recreated if needed later

	setText(label->getText()); // force refresh
}

void CTextBox::setText(const std::string &text)
{
	label->pos.w = pos.w; // reset to default before textSize.y check
	label->setText(text);
	if(label->textSize.y <= label->pos.h && slider)
	{
		// slider is no longer needed
		vstd::clear_pointer(slider);
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

		OBJ_CONSTRUCTION_CAPTURING_ALL;
		slider = new CSlider(Point(pos.w - 32, 0), pos.h, std::bind(&CTextBox::sliderMoved, this, _1),
		                     label->pos.h, label->textSize.y, 0, false, CSlider::EStyle(sliderStyle));
		slider->setScrollStep(graphics->fonts[label->font]->getLineHeight());
	}
}

void CGStatusBar::setText(const std::string & Text)
{
	if(!textLock)
		CLabel::setText(Text);
}

void CGStatusBar::clear()
{
	setText("");
}

CGStatusBar::CGStatusBar(CPicture *BG, EFonts Font /*= FONT_SMALL*/, EAlignment Align /*= CENTER*/, const SDL_Color &Color /*= Colors::WHITE*/)
: CLabel(BG->pos.x, BG->pos.y, Font, Align, Color, "")
{
	init();
	bg = BG;
	addChild(bg);
	pos = bg->pos;
	getBorderSize();
	textLock = false;
}

CGStatusBar::CGStatusBar(int x, int y, std::string name/*="ADROLLVR.bmp"*/, int maxw/*=-1*/)
: CLabel(x, y, FONT_SMALL, CENTER)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	init();
	bg = new CPicture(name);
	pos = bg->pos;
	if((unsigned int)maxw < pos.w)
	{
		vstd::amin(pos.w, maxw);
		bg->srcRect = new Rect(0, 0, maxw, pos.h);
	}
	textLock = false;
}

CGStatusBar::~CGStatusBar()
{
	GH.statusbar = oldStatusBar;
}

void CGStatusBar::show(SDL_Surface * to)
{
	showAll(to);
}

void CGStatusBar::init()
{
	oldStatusBar = GH.statusbar;
	GH.statusbar = this;
}

Point CGStatusBar::getBorderSize()
{
	//Width of borders where text should not be printed
	static const Point borderSize(5,1);

	switch(alignment)
	{
	case TOPLEFT:     return Point(borderSize.x, borderSize.y);
	case CENTER:      return Point(pos.w/2, pos.h/2);
	case BOTTOMRIGHT: return Point(pos.w - borderSize.x, pos.h - borderSize.y);
	}
	assert(0);
	return Point();
}

void CGStatusBar::lock(bool shouldLock)
{
	textLock = shouldLock;
}

CTextInput::CTextInput(const Rect &Pos, EFonts font, const CFunctionList<void(const std::string &)> &CB):
	CLabel(Pos.x, Pos.y, font, CENTER),
	cb(CB)
{
	type |= REDRAW_PARENT;
	focus = false;
	pos.h = Pos.h;
	pos.w = Pos.w;
	captureAllKeys = true;
	bg = nullptr;
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);
	giveFocus();
}

CTextInput::CTextInput( const Rect &Pos, const Point &bgOffset, const std::string &bgName, const CFunctionList<void(const std::string &)> &CB )
:cb(CB)
{
	focus = false;
	pos += Pos;
	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	bg = new CPicture(bgName, bgOffset.x, bgOffset.y);
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);
	giveFocus();
}

CTextInput::CTextInput(const Rect &Pos, SDL_Surface *srf)
{
	focus = false;
	pos += Pos;
	captureAllKeys = true;
	OBJ_CONSTRUCTION;
	bg = new CPicture(Pos, 0, true);
	Rect hlp = Pos;
	if(srf)
		CSDL_Ext::blitSurface(srf, &hlp, *bg, nullptr);
	else
		SDL_FillRect(*bg, nullptr, 0);
	pos.w = bg->pos.w;
	pos.h = bg->pos.h;
	bg->pos = pos;
	addUsedEvents(LCLICK | KEYBOARD | TEXTINPUT);
	giveFocus();
}

void CTextInput::focusGot()
{
	CSDL_Ext::startTextInput(&pos);	
}

void CTextInput::focusLost()
{
	CSDL_Ext::stopTextInput();
}


std::string CTextInput::visibleText()
{
	return focus ? text + newText + "_" : text;
}

void CTextInput::clickLeft( tribool down, bool previousState )
{
	if(down && !focus)
		giveFocus();
}

void CTextInput::keyPressed( const SDL_KeyboardEvent & key )
{

	if(!focus || key.state != SDL_PRESSED)
		return;

	if(key.keysym.sym == SDLK_TAB)
	{
		moveFocus();
		GH.breakEventHandling();
		return;
	}

	bool redrawNeeded = false;
	
	switch(key.keysym.sym)
	{
	case SDLK_DELETE: // have index > ' ' so it won't be filtered out by default section
		return;
	case SDLK_BACKSPACE:
		if(!newText.empty())
		{
			Unicode::trimRight(newText);
			redrawNeeded = true;
		}
		else if(!text.empty())
		{
			Unicode::trimRight(text);
			redrawNeeded = true;
		}			
		break;
	default:
		break;
	}

	if (redrawNeeded)
	{
		redraw();
		cb(text);
	}	
}

void CTextInput::setText( const std::string &nText, bool callCb )
{
	CLabel::setText(nText);
	if(callCb)
		cb(text);
}

bool CTextInput::captureThisEvent(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_RETURN || key.keysym.sym == SDLK_KP_ENTER || key.keysym.sym == SDLK_ESCAPE)
		return false;
	
	return true;
}

void CTextInput::textInputed(const SDL_TextInputEvent & event)
{
	if(!focus)
		return;
	std::string oldText = text;
	
	text += event.text;	
	
	filters(text,oldText);
	if (text != oldText)
	{
		redraw();
		cb(text);
	}	
	newText = "";
}

void CTextInput::textEdited(const SDL_TextEditingEvent & event)
{
	if(!focus)
		return;
		
	newText = event.text;
	redraw();
	cb(text+newText);	
}

void CTextInput::filenameFilter(std::string & text, const std::string &)
{
	static const std::string forbiddenChars = "<>:\"/\\|?*\r\n"; //if we are entering a filename, some special characters won't be allowed
	size_t pos;
	while ((pos = text.find_first_of(forbiddenChars)) != std::string::npos)
		text.erase(pos, 1);
}

void CTextInput::numberFilter(std::string & text, const std::string & oldText, int minValue, int maxValue)
{
	assert(minValue < maxValue);

	if (text.empty())
		text = "0";

	size_t pos = 0;
	if (text[0] == '-') //allow '-' sign as first symbol only
		pos++;

	while (pos < text.size())
	{
		if (text[pos] < '0' || text[pos] > '9')
		{
			text = oldText;
			return; //new text is not number.
		}
		pos++;
	}
	try
	{
		int value = boost::lexical_cast<int>(text);
		if (value < minValue)
			text = boost::lexical_cast<std::string>(minValue);
		else if (value > maxValue)
			text = boost::lexical_cast<std::string>(maxValue);
	}
	catch(boost::bad_lexical_cast &)
	{
		//Should never happen. Unless I missed some cases
		logGlobal->warnStream() << "Warning: failed to convert "<< text << " to number!";
		text = oldText;
	}
}

CFocusable::CFocusable()
{
	focusables.push_back(this);
}

CFocusable::~CFocusable()
{
	if(inputWithFocus == this)
	{
		focusLost();
		inputWithFocus = nullptr;
	}	

	focusables -= this;
}
void CFocusable::giveFocus()
{
	if(inputWithFocus)
	{
		inputWithFocus->focus = false;
		inputWithFocus->focusLost();
		inputWithFocus->redraw();
	}

	focus = true;
	inputWithFocus = this;
	focusGot();
	redraw();	
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
			break;;
		}
	}
}
