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
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../windows/CMessage.h"
#include "../windows/InfoWindows.h"
#include "../adventureMap/CInGameConsole.h"
#include "../eventsSDL/InputHandler.h"
#include "../render/Canvas.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"

#include "../../lib/texts/TextOperations.h"

#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#endif

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

CLabel::CLabel(int x, int y, EFonts Font, ETextAlignment Align, const ColorRGBA & Color, const std::string & Text, int maxWidth)
	: CTextContainer(Align, Font, Color), text(Text), maxWidth(maxWidth)
{
	setRedrawParent(true);
	autoRedraw = true;
	pos.x += x;
	pos.y += y;
	pos.w = pos.h = 0;

	trimText();

	if(alignment == ETextAlignment::TOPLEFT) // causes issues for MIDDLE
	{
		const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
		pos.w = fontPtr->getStringWidth(visibleText().c_str());
		pos.h = fontPtr->getLineHeight();
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
	assert(TextOperations::isValidUnicodeString(Txt));

	text = Txt;
	
	trimText();

	if(autoRedraw)
	{
		if(background || !parent)
			redraw();
		else
			parent->redraw();
	}
}

void CLabel::clear()
{
	text.clear();

	if(autoRedraw)
	{
		if(background || !parent)
			redraw();
		else
			parent->redraw();
	}
}

void CLabel::setMaxWidth(int width)
{
	maxWidth = width;
}

void CLabel::trimText()
{
	if(maxWidth > 0)
	{
		const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

		while (fontPtr->getStringWidth(visibleText().c_str()) > maxWidth)
			TextOperations::trimRightUnicode(text);
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
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
	return fontPtr->getStringWidth(visibleText());
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

std::vector<std::string> CMultiLineLabel::getLines()
{
	return lines;
}

void CTextContainer::blitLine(Canvas & to, Rect destRect, std::string what)
{
	const auto f = ENGINE->renderHandler().loadFont(font);
	Point where = destRect.topLeft();
	const std::string delimiters = "{}";
	auto delimitersCount = std::count_if(what.cbegin(), what.cend(), [&delimiters](char c)
	{
		return delimiters.find(c) != std::string::npos;
	});
	//We should count delimiters length from string to correct centering later.
	delimitersCount *= f->getStringWidth(delimiters)/2;

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

	if(alignment == ETextAlignment::TOPLEFT || alignment == ETextAlignment::CENTERLEFT  || alignment == ETextAlignment::BOTTOMLEFT)
		where.x += getBorderSize().x;

	if(alignment == ETextAlignment::CENTER || alignment == ETextAlignment::TOPCENTER || alignment == ETextAlignment::BOTTOMCENTER)
		where.x += (destRect.w - (static_cast<int>(f->getStringWidth(what)) - delimitersCount)) / 2;

	if(alignment == ETextAlignment::TOPRIGHT || alignment == ETextAlignment::BOTTOMRIGHT || alignment == ETextAlignment::CENTERRIGHT)
		where.x += getBorderSize().x + destRect.w - (static_cast<int>(f->getStringWidth(what)) - delimitersCount);

	if(alignment == ETextAlignment::TOPLEFT || alignment == ETextAlignment::TOPCENTER || alignment == ETextAlignment::TOPRIGHT)
		where.y += getBorderSize().y;

	if(alignment == ETextAlignment::CENTERLEFT || alignment == ETextAlignment::CENTER || alignment == ETextAlignment::CENTERRIGHT)
		where.y += (destRect.h - static_cast<int>(f->getLineHeight())) / 2;

	if(alignment == ETextAlignment::BOTTOMLEFT || alignment == ETextAlignment::BOTTOMCENTER || alignment == ETextAlignment::BOTTOMRIGHT)
		where.y += getBorderSize().y + destRect.h - static_cast<int>(f->getLineHeight());

	size_t begin = 0;
	size_t currDelimiter = 0;

	do
	{
		size_t end = what.find_first_of(delimiters[currDelimiter % 2], begin);
		if(begin != end)
		{
			std::string toPrint = what.substr(begin, end - begin);

			if(currDelimiter % 2) // Enclosed in {} text - set to yellow or defined color
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
		currDelimiter++;
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

	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	// calculate which lines should be visible
	int totalLines = static_cast<int>(lines.size());
	int beginLine = visibleSize.y;
	int endLine = getTextLocation().h + visibleSize.y;

	if(beginLine < 0)
		beginLine = 0;
	else
		beginLine /= fontPtr->getLineHeight();

	if(endLine < 0)
		endLine = 0;
	else
		endLine /= fontPtr->getLineHeight();
	endLine++;

	// and where they should be displayed
	Point lineStart = getTextLocation().topLeft() - visibleSize + Point(0, beginLine * fontPtr->getLineHeight());
	Point lineSize = Point(getTextLocation().w, fontPtr->getLineHeight());

	CanvasClipRectGuard guard(to, getTextLocation()); // to properly trim text that is too big to fit

	for(int i = beginLine; i < std::min(totalLines, endLine); i++)
	{
		if(!lines[i].empty()) //non-empty line
			blitLine(to, Rect(lineStart, lineSize), lines[i]);

		lineStart.y += fontPtr->getLineHeight();
	}
}

void CMultiLineLabel::splitText(const std::string & Txt, bool redrawAfter)
{
	lines.clear();

	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
	int lineHeight = fontPtr->getLineHeight();

	lines = CMessage::breakText(Txt, pos.w, font);

	textSize.y = lineHeight * (int)lines.size();
	textSize.x = 0;
	for(const std::string & line : lines)
		vstd::amax(textSize.x, fontPtr->getStringWidth(line.c_str()));
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

	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
	Point textSizeComputed(pos.w, fontPtr->getLineHeight() * lines.size()); //FIXME: how is this different from textSize member?
	Point textOffset(pos.w - textSizeComputed.x, pos.h - textSizeComputed.y);

	switch(alignment)
	{
	case ETextAlignment::TOPLEFT:     return Rect(pos.topLeft(), textSizeComputed);
	case ETextAlignment::TOPCENTER:   return Rect(pos.topLeft(), textSizeComputed);
	case ETextAlignment::CENTER:      return Rect(pos.topLeft() + textOffset / 2, textSizeComputed);
	case ETextAlignment::CENTERLEFT:  return Rect(pos.topLeft() + Point(0, textOffset.y / 2), textSizeComputed);
	case ETextAlignment::CENTERRIGHT: return Rect(pos.topLeft() + Point(textOffset.x, textOffset.y / 2), textSizeComputed);
	case ETextAlignment::BOTTOMRIGHT: return Rect(pos.topLeft() + textOffset, textSizeComputed);
	}
	assert(0);
	return Rect();
}

CLabelGroup::CLabelGroup(EFonts Font, ETextAlignment Align, const ColorRGBA & Color)
	: font(Font), align(Align), color(Color)
{
}

void CLabelGroup::add(int x, int y, const std::string & text)
{
	OBJECT_CONSTRUCTION;
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
	OBJECT_CONSTRUCTION;
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

void CTextBox::trimToFit()
{
	if (slider)
		return;

	pos.w = label->textSize.x;
	pos.h = label->textSize.y;
	label->pos.w = label->textSize.x;
	label->pos.h = label->textSize.y;
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
		label->scrollTextTo(0);
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
		const auto & fontPtr = ENGINE->renderHandler().loadFont(label->font);

		OBJECT_CONSTRUCTION;
		slider = std::make_shared<CSlider>(Point(pos.w - 16, 0), pos.h, std::bind(&CTextBox::sliderMoved, this, _1),
			label->pos.h, label->textSize.y, 0, Orientation::VERTICAL, CSlider::EStyle(sliderStyle));
		slider->setScrollStep(fontPtr->getLineHeight());
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
		ENGINE->input().startTextInput(pos);
		setText(consoleText);
	}
	else
	{
		//assert(enteringText == true);
		alignment = ETextAlignment::CENTER;
		ENGINE->input().stopTextInput();
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
	: CLabel(background_->pos.x, background_->pos.y, Font, Align, Color, "", background_->pos.w)
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

	OBJECT_CONSTRUCTION;

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
	assert(ENGINE->statusbar().get() != this);
}

void CGStatusBar::show(Canvas & to)
{
	showAll(to);
}

void CGStatusBar::clickPressed(const Point & cursorPosition)
{
	if(GAME->interface() && GAME->interface()->cingconsole->isActive())
		GAME->interface()->cingconsole->startEnteringText();
}

void CGStatusBar::activate()
{
	ENGINE->setStatusbar(shared_from_this());
	CIntObject::activate();
}

void CGStatusBar::deactivate()
{
	ENGINE->setStatusbar(nullptr);

	if (enteringText)
		GAME->interface()->cingconsole->endEnteringText(false);

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
