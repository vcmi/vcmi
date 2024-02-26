/*
 * CMessage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMessage.h"

#include "../../lib/TextOperations.h"

#include "../gui/CGuiHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/Graphics.h"
#include "../render/IFont.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"

const int BEFORE_COMPONENTS = 30;
const int SIDE_MARGIN = 30;

static std::array<std::shared_ptr<CAnimation>, PlayerColor::PLAYER_LIMIT_I> dialogBorders;
static std::array<std::vector<std::shared_ptr<IImage>>, PlayerColor::PLAYER_LIMIT_I> piecesOfBox;

void CMessage::init()
{
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
	{
		dialogBorders[i] = GH.renderHandler().loadAnimation(AnimationPath::builtin("DIALGBOX"));
		dialogBorders[i]->preload();

		for(int j = 0; j < dialogBorders[i]->size(0); j++)
		{
			auto image = dialogBorders[i]->getImage(j, 0);
			//assume blue color initially
			if(i != 1)
				image->playerColored(PlayerColor(i));
			piecesOfBox[i].push_back(image);
		}
	}
}

void CMessage::dispose()
{
	for(auto & item : dialogBorders)
		item.reset();
}

std::vector<std::string> CMessage::breakText(std::string text, size_t maxLineWidth, EFonts font)
{
	assert(maxLineWidth != 0);
	if(maxLineWidth == 0)
		return {text};

	std::vector<std::string> ret;

	boost::algorithm::trim_right_if(text, boost::algorithm::is_any_of(std::string(" ")));

	// each iteration generates one output line
	while(text.length())
	{
		ui32 lineWidth = 0; //in characters or given char metric
		ui32 wordBreak = -1; //last position for line break (last space character)
		ui32 currPos = 0; //current position in text
		bool opened = false; //set to true when opening brace is found
		std::string color = ""; //color found

		size_t symbolSize = 0; // width of character, in bytes
		size_t glyphWidth = 0; // width of printable glyph, pixels

		// loops till line is full or end of text reached
		while(currPos < text.length() && text[currPos] != 0x0a && lineWidth < maxLineWidth)
		{
			symbolSize = TextOperations::getUnicodeCharacterSize(text[currPos]);
			glyphWidth = graphics->fonts[font]->getGlyphWidth(text.data() + currPos);

			// candidate for line break
			if(ui8(text[currPos]) <= ui8(' '))
				wordBreak = currPos;

			/* We don't count braces in string length. */
			if(text[currPos] == '{')
			{
				opened = true;

				std::smatch match;
				std::regex expr("^\\{(.*?)\\|");
				std::string tmp = text.substr(currPos);
				if(std::regex_search(tmp, match, expr))
				{
					std::string colorText = match[1].str();
					if(auto c = Colors::parseColor(colorText))
					{
						color = colorText + "|";
						currPos += colorText.length() + 1;
					}
				}
			}
			else if(text[currPos] == '}')
			{
				opened = false;
				color = "";
			}
			else
				lineWidth += (ui32)glyphWidth;
			currPos += (ui32)symbolSize;
		}

		// long line, create line break
		if(currPos < text.length() && (text[currPos] != 0x0a))
		{
			if(wordBreak != ui32(-1))
				currPos = wordBreak;
			else
				currPos -= (ui32)symbolSize;
		}

		//non-blank line
		if(currPos != 0)
		{
			ret.push_back(text.substr(0, currPos));

			if(opened)
				/* Close the brace for the current line. */
				ret.back() += '}';

			text.erase(0, currPos);
		}
		else if(text[currPos] == 0x0a)
		{
			ret.push_back(""); //add empty string, no extra actions needed
		}

		if(text.length() != 0 && text[0] == 0x0a)
		{
			/* Remove LF */
			text.erase(0, 1);
		}
		else
		{
			// trim only if line does not starts with LF
			// FIXME: necessary? All lines will be trimmed before returning anyway
			boost::algorithm::trim_left_if(text, boost::algorithm::is_any_of(std::string(" ")));
		}

		if(opened)
		{
			/* Add an opening brace for the next line. */
			if(text.length() != 0)
				text.insert(0, "{" + color);
		}
	}

	/* Trim whitespaces of every line. */
	for(auto & elem : ret)
		boost::algorithm::trim(elem);

	return ret;
}

std::string CMessage::guessHeader(const std::string & msg)
{
	size_t begin = 0;
	std::string delimeters = "{}";
	size_t start = msg.find_first_of(delimeters[0], begin);
	size_t end = msg.find_first_of(delimeters[1], start);
	if(start > msg.size() || end > msg.size())
		return "";
	return msg.substr(begin, end);
}

int CMessage::guessHeight(const std::string & txt, int width, EFonts font)
{
	const auto f = graphics->fonts[font];
	auto lines = CMessage::breakText(txt, width, font);
	int lineHeight = static_cast<int>(f->getLineHeight());
	return lineHeight * (int)lines.size();
}

int CMessage::getEstimatedComponentHeight(int numComps)
{
	if(numComps > 8) //Bigger than 8 components - return invalid value
		return std::numeric_limits<int>::max();
	else if(numComps > 2)
		return 160; // 32px * 1 row + 20 to offset
	else if(numComps)
		return 118; // 118 px to offset
	return 0;
}

void CMessage::drawIWindow(CInfoWindow * ret, std::string text, PlayerColor player)
{
	constexpr std::array textAreaSizes = {
		Point(300, 200), // if message is small, h3 will use 300px-wide text box with up to 200px height
		Point(400, 200), // once text no longer fits into 300x200 box, h3 will start using 400px - wide boxes
		Point(600, 200) // if 400px is not enough either, h3 will use largest, 600px-wide textbox, potentially with slider
	};

	assert(ret && ret->text);

	for(const auto & area : textAreaSizes)
	{
		ret->text->resize(area);
		if(!ret->text->slider)
			break; // suitable size found, use it
	}

	if(ret->text->slider)
		ret->text->slider->addUsedEvents(CIntObject::WHEEL | CIntObject::KEYBOARD);

	ret->text->trimToFit();

	Point winSize(ret->text->pos.w, ret->text->pos.h); //start with text size

	if(ret->components)
		winSize.y += 10 + ret->components->pos.h; //space to first component

	int bw = 0;
	if(ret->buttons.size())
	{
		int bh = 0;
		// Compute total width of buttons
		bw = 20 * ((int)ret->buttons.size() - 1); // space between all buttons
		for(auto & elem : ret->buttons) //and add buttons width
		{
			bw += elem->pos.w;
			vstd::amax(bh, elem->pos.h);
		}
		winSize.y += 20 + bh; //before button + button
	}

	// Clip window size
	vstd::amax(winSize.y, 50);
	vstd::amax(winSize.x, 80);
	if(ret->components)
		vstd::amax(winSize.x, ret->components->pos.w);
	vstd::amax(winSize.x, bw);

	vstd::amin(winSize.x, GH.screenDimensions().x - 150);

	ret->pos.h = winSize.y + 2 * SIDE_MARGIN;
	ret->pos.w = winSize.x + 2 * SIDE_MARGIN;
	ret->center();
	ret->backgroundTexture->pos = ret->pos;

	int curh = SIDE_MARGIN;
	int xOffset = (ret->pos.w - ret->text->pos.w) / 2;

	if(ret->buttons.empty() && !ret->components) //improvement for very small text only popups -> center text vertically
	{
		if(ret->pos.h > ret->text->pos.h + 2 * SIDE_MARGIN)
			curh = (ret->pos.h - ret->text->pos.h) / 2;
	}

	ret->text->moveBy(Point(xOffset, curh));

	curh += ret->text->pos.h;

	if(ret->components)
	{
		curh += BEFORE_COMPONENTS;
		curh += ret->components->pos.h;
	}

	if(ret->buttons.size())
	{
		// Position the buttons at the bottom of the window
		bw = (ret->pos.w / 2) - (bw / 2);
		curh = ret->pos.h - SIDE_MARGIN - ret->buttons[0]->pos.h;

		for(auto & elem : ret->buttons)
		{
			elem->moveBy(Point(bw, curh));
			bw += elem->pos.w + 20;
		}
	}
	if(ret->components)
		ret->components->moveBy(Point(ret->pos.x, ret->pos.y));
}

void CMessage::drawBorder(PlayerColor playerColor, Canvas & to, int w, int h, int x, int y)
{
	if(playerColor.isSpectator())
		playerColor = PlayerColor(1);
	auto & box = piecesOfBox.at(playerColor.getNum());

	// Note: this code assumes that the corner dimensions are all the same.

	// Horizontal borders
	int start_x = x + box[0]->width();
	const int stop_x = x + w - box[1]->width();
	const int bottom_y = y + h - box[7]->height() + 1;
	while(start_x < stop_x)
	{

		// Top border
		to.draw(box[6], Point(start_x, y));
		// Bottom border
		to.draw(box[7], Point(start_x, bottom_y));

		start_x += box[6]->width();
	}

	// Vertical borders
	int start_y = y + box[0]->height();
	const int stop_y = y + h - box[2]->height() + 1;
	const int right_x = x + w - box[5]->width();
	while(start_y < stop_y)
	{

		// Left border
		to.draw(box[4], Point(x, start_y));
		// Right border
		to.draw(box[5], Point(right_x, start_y));

		start_y += box[4]->height();
	}

	//corners
	to.draw(box[0], Point(x, y));
	to.draw(box[1], Point(x + w - box[1]->width(), y));
	to.draw(box[2], Point(x, y + h - box[2]->height() + 1));
	to.draw(box[3], Point(x + w - box[3]->width(), y + h - box[3]->height() + 1));
}
