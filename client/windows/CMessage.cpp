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

#include "../../lib/texts/TextOperations.h"

constexpr int RIGHT_CLICK_POPUP_MIN_SIZE = 100;
constexpr int SIDE_MARGIN = 11;
constexpr int TOP_MARGIN = 20;
constexpr int BOTTOM_MARGIN = 16;
constexpr int INTERVAL_BETWEEN_BUTTONS = 18;
constexpr int INTERVAL_BETWEEN_TEXT_AND_BUTTONS = 24;

static std::array<std::shared_ptr<CAnimation>, PlayerColor::PLAYER_LIMIT_I> dialogBorders;
static std::array<std::vector<std::shared_ptr<IImage>>, PlayerColor::PLAYER_LIMIT_I> piecesOfBox;

void CMessage::init()
{
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
	{
		dialogBorders[i] = GH.renderHandler().loadAnimation(AnimationPath::builtin("DIALGBOX"), EImageBlitMode::COLORKEY);

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

std::vector<size_t> CMessage::getAllPositionsInStr(const char & targetChar, const std::string_view & str)
{
	std::vector<size_t> results;
	for(size_t pos = 0; pos < str.size(); pos++)
	{
		if(str[pos] == targetChar)
			results.push_back(pos);
	}
	return results;
};

bool CMessage::validateColorTags(const std::string & text)
{
	const auto validateTagsPositions = [](const auto & openingTags, const auto & closingTags) -> bool
		{
			if(openingTags.size() != closingTags.size())
			{
				logGlobal->error("CMessage: Not Equal number opening and closing tags");
				return false;
			}

			if(openingTags.empty())
				return true;

			auto closingPos = closingTags.begin();
			size_t closingPrevPos = 0;
			for(const auto & openingPos : openingTags)
			{
				if(openingPos >= *closingPos)
				{
					logGlobal->error("CMessage: Closing tag before opening tag");
					return false;
				}
				if(openingPos < closingPrevPos)
				{
					logGlobal->error("CMessage: Tags inside another tags");
					return false;
				}
				closingPrevPos = *closingPos++;
			}
			return true;
		};

	if(!validateTagsPositions(getAllPositionsInStr('{', text), getAllPositionsInStr('}', text)))
	{
		logGlobal->error("CMessage: Line validation Invalid: %s", text);
		return false;
	}
	if(!validateTagsPositions(getAllPositionsInStr('#', text), getAllPositionsInStr('|', text)))
	{
		logGlobal->error("CMessage: Line validation Invalid: %s", text);
		return false;
	}
	return true;
}

std::vector<CMessage::coloredline> CMessage::getPossibleLines(
	const std::string & line, size_t maxLineWidth, const EFonts font, const char & splitSymbol)
{
	const auto & fontPtr = GH.renderHandler().loadFont(font);
	std::vector<coloredline> result;
	std::string_view lastFoundColor;
	const std::function<size_t(const std::string_view&)> findColorTags = [&](const std::string_view & subLine) -> size_t
		{
			size_t foundWidth = 0;
			if(lastFoundColor.empty())
			{
				if(const auto colorTextBegin = subLine.find('{'); colorTextBegin != std::string::npos)
				{
					foundWidth += fontPtr->getStringWidth("{");
					// Opened color tag brace found. Try to find color code.
					if(const auto colorTagBegin = colorTextBegin + 1; subLine[colorTagBegin] == '#')
					{
						const auto colorTagEnd = subLine.find('|');
						assert(colorTagEnd != std::string::npos);
						assert(subLine.find('}') > colorTagEnd);
						foundWidth += fontPtr->getStringWidth(std::string(subLine.substr(colorTextBegin, colorTextBegin - colorTagEnd + 1)));
						lastFoundColor = subLine.substr(colorTextBegin, colorTextBegin - colorTagEnd + 1);
					}
					else
					{
						lastFoundColor = subLine.substr(colorTextBegin, 1);
					}
				}
				else
				{
					return foundWidth;
				}
			}
			if(const auto colorTextEnd = subLine.find('}'); colorTextEnd != std::string::npos)
			{
				foundWidth += fontPtr->getStringWidth("}");
				lastFoundColor = std::string_view();
				foundWidth += findColorTags(subLine.substr(colorTextEnd + 1, subLine.length() - colorTextEnd - 1));
			}
			return foundWidth;
		};

	const std::function<void(const std::string_view&)> splitBySymbol = [&](const std::string_view & subLine)
		{
			// Trim splitSymbols from beginning and ending
			const auto subLineBeginPos = subLine.find_first_not_of(splitSymbol);
			if(subLineBeginPos == std::string::npos)
				return;
			const auto subLineEndPos = subLine.find_last_not_of(splitSymbol);
			if(subLineEndPos == std::string::npos)
				return;

			const auto trimmedLine = subLine.substr(subLineBeginPos, subLineEndPos - subLineBeginPos + 1);
			size_t wordBegin = 0;
			const auto splitPossibles = getAllPositionsInStr(splitSymbol, trimmedLine);
			if(splitPossibles.empty())
			{
				result.back().line = std::make_pair(trimmedLine.data() - line.data(), trimmedLine.length());
				return;
			}

			size_t lineWidth = 0;
			const auto curWord = trimmedLine.substr(wordBegin, splitPossibles.front());
			size_t nextWordWidth = fontPtr->getStringWidth(std::string(curWord)) - findColorTags(curWord);

			for(auto splitPos = splitPossibles.cbegin(); splitPos < splitPossibles.cend(); splitPos = std::next(splitPos))
			{
				lineWidth += nextWordWidth;
				const auto curColor = lastFoundColor;
				const auto wordEnd = *splitPos;
				const auto nextSplitPos = std::next(splitPos);
				const auto nextWord = nextSplitPos == splitPossibles.end() ? trimmedLine.substr(wordEnd, trimmedLine.length() - wordEnd)
					 : trimmedLine.substr(wordEnd, *nextSplitPos - wordEnd);
				nextWordWidth = fontPtr->getStringWidth(std::string(nextWord)) - findColorTags(nextWord);

				if(lineWidth + nextWordWidth > maxLineWidth)
				{
					const auto resultLine = trimmedLine.substr(0, wordEnd);
					// Trim splitSymbols from ending and place result line into result
					result.back().line = std::make_pair(resultLine.data() - line.data(), resultLine.find_last_not_of(splitSymbol) + 1);
					if(curColor.empty())
					{
						result.emplace_back();
					}
					else
					{
						result.back().closingTagNeeded = true;
						result.emplace_back(std::optional<std::pair<size_t, size_t>>({ curColor.data() - line.data(), curColor.length() }));
					}
					splitBySymbol(trimmedLine.substr(wordEnd, trimmedLine.length() - wordEnd));
					return;
				}
				else
				{
					if(wordBegin == wordEnd)
						lineWidth += fontPtr->getStringWidth(std::string(1, splitSymbol));
					wordBegin = wordEnd + 1;
				}
			}
			result.back().line = std::make_pair(trimmedLine.data() - line.data(), trimmedLine.length());
		};

	result.emplace_back();
	splitBySymbol(line);

	return result;
}

std::vector<std::string> CMessage::breakText(const std::string & text, size_t maxLineWidth, const EFonts font)
{
	std::vector<std::string> result;
	if(maxLineWidth == 0)
		return result;
	if(!validateColorTags(text))
		return result;

	// Detect end of line symbol. If '\r\n', remove '\r'.
	// Texts with '\r\n' are very rare (Credits case), so rare string copying should not be a problem.
	std::string preparedText;
	auto preparedTextPtr = &text;
	auto breakSymbol = '\n';
	if(text.find("\r\n") != std::string::npos)
	{
		preparedText = text;
		preparedTextPtr = &preparedText;
		preparedText.erase(std::remove(preparedText.begin(), preparedText.end(), '\r'), preparedText.end());
	}
	else if(text.find('\r') != std::string::npos)
	{
		breakSymbol = '\r';
	}

	// Split text by lines
	auto possibleLines = getPossibleLines(*preparedTextPtr, 0, font, breakSymbol);

	// Add additional empty lines if needed. In Case "....\n\n...."
	const auto possibleLinesOrigin = possibleLines;
	auto curLine = possibleLinesOrigin.cbegin();
	auto insertionIterator = possibleLines.cbegin();
	size_t nextLineBeginEstimate = 0;
	auto newLines = curLine->line.first - nextLineBeginEstimate;

	for(auto nextLine = std::next(curLine);; nextLine = std::next(nextLine))
	{
		for(size_t newBreak = 0; newBreak < newLines; newBreak++)
		{
			const auto newLine = possibleLines.emplace(insertionIterator);
			newLine->line.first = nextLineBeginEstimate + newBreak;
			insertionIterator = std::next(newLine);
		}
		if(nextLine >= possibleLinesOrigin.cend())
			break;

		nextLineBeginEstimate = curLine->line.first + curLine->line.second + 1;
		newLines = nextLine->line.first - nextLineBeginEstimate;
		assert(nextLineBeginEstimate <= nextLine->line.first);
		curLine = nextLine;
		insertionIterator = std::next(insertionIterator);
	}

	// Split lines by spaces and fill result
	const auto makeNewLine = [](const auto & lineBreak, const auto & srcStr, auto & emptyLine)
		{
			const auto colorTag = lineBreak.colorTag ?
				std::string_view(srcStr).substr(lineBreak.colorTag.value().first, lineBreak.colorTag.value().second)
				: std::string_view();
			emptyLine = std::string(colorTag) + srcStr.substr(lineBreak.line.first, lineBreak.line.second);
			if(lineBreak.closingTagNeeded)
				emptyLine.append("}");
		};

	for(const auto & lineBreak : possibleLines)
	{	
		std::string line;
		makeNewLine(lineBreak, *preparedTextPtr, line);
		for(const auto & breakBySpace : getPossibleLines(line, maxLineWidth, font, ' '))
			makeNewLine(breakBySpace, line, result.emplace_back());
	}
	return result;
}

std::string CMessage::guessHeader(const std::string & msg)
{
	size_t begin = 0;
	std::string delimiters = "{}";
	size_t start = msg.find_first_of(delimiters[0], begin);
	size_t end = msg.find_first_of(delimiters[1], start);
	if(start > msg.size() || end > msg.size())
		return "";
	return msg.substr(begin, end);
}

int CMessage::guessHeight(const std::string & txt, int width, EFonts font)
{
	const auto & fontPtr = GH.renderHandler().loadFont(font);
	const auto lines = CMessage::breakText(txt, width, font);
	size_t lineHeight = fontPtr->getLineHeight();
	return lineHeight * lines.size();
}

int CMessage::getEstimatedComponentHeight(int numComps)
{
	if(numComps > 8) //Bigger than 8 components - return invalid value
		return std::numeric_limits<int>::max();
	if(numComps > 2)
		return 160; // 32px * 1 row + 20 to offset
	if(numComps > 0)
		return 118; // 118 px to offset
	return 0;
}

void CMessage::drawIWindow(CInfoWindow * ret, std::string text, PlayerColor player)
{
	// possible sizes of text boxes section of window
	// game should pick smallest one that can fit text without slider
	// or, if not possible - pick last one and use slider
	constexpr std::array textAreaSizes = {
		// FIXME: this size should only be used for single-line texts: Point(206, 72),
		Point(270, 72),
		Point(270, 136),
		Point(270, 200),
		Point(400, 136),
		Point(400, 200),
		Point(590, 200)
	};

	assert(ret && ret->text);

	// STEP 1: DETERMINE SIZE OF ALL ELEMENTS

	for(const auto & area : textAreaSizes)
	{
		ret->text->resize(area);
		if(!ret->text->slider)
			break; // suitable size found, use it
	}

	int textHeight = ret->text->pos.h;

	if(ret->text->slider)
		ret->text->slider->addUsedEvents(CIntObject::WHEEL | CIntObject::KEYBOARD);

	int buttonsWidth = 0;
	int buttonsHeight = 0;
	if(!ret->buttons.empty())
	{
		// Compute total width of buttons
		buttonsWidth = INTERVAL_BETWEEN_BUTTONS * (ret->buttons.size() - 1); // space between all buttons
		for(const auto & elem : ret->buttons) //and add buttons width
		{
			buttonsWidth += elem->pos.w;
			vstd::amax(buttonsHeight, elem->pos.h);
		}
	}

	// STEP 2: COMPUTE WINDOW SIZE

	if(ret->buttons.empty() && !ret->components)
	{
		// use more compact form for right-click popup with no buttons / components

		ret->pos.w = std::max(RIGHT_CLICK_POPUP_MIN_SIZE, ret->text->label->textSize.x + 2 * SIDE_MARGIN);
		ret->pos.h = std::max(RIGHT_CLICK_POPUP_MIN_SIZE, ret->text->label->textSize.y + TOP_MARGIN + BOTTOM_MARGIN);
	}
	else
	{
		int windowContentWidth = ret->text->pos.w;
		int windowContentHeight = ret->text->pos.h;
		if(ret->components)
		{
			vstd::amax(windowContentWidth, ret->components->pos.w);
			windowContentHeight += INTERVAL_BETWEEN_TEXT_AND_BUTTONS + ret->components->pos.h;
		}
		if(!ret->buttons.empty())
		{
			vstd::amax(windowContentWidth, buttonsWidth);
			windowContentHeight += INTERVAL_BETWEEN_TEXT_AND_BUTTONS + buttonsHeight;
		}

		ret->pos.w = windowContentWidth + 2 * SIDE_MARGIN;
		ret->pos.h = windowContentHeight + TOP_MARGIN + BOTTOM_MARGIN;
	}

	// STEP 3: MOVE ALL ELEMENTS IN PLACE

	if(ret->buttons.empty() && !ret->components)
	{
		ret->text->trimToFit();
		ret->text->center(ret->pos.center());
	}
	else
	{
		if(ret->components)
			ret->components->moveBy(Point((ret->pos.w - ret->components->pos.w) / 2, TOP_MARGIN + ret->text->pos.h + INTERVAL_BETWEEN_TEXT_AND_BUTTONS));

		ret->text->trimToFit();
		ret->text->moveBy(Point((ret->pos.w - ret->text->pos.w) / 2, TOP_MARGIN + (textHeight - ret->text->pos.h) / 2 ));

		if(!ret->buttons.empty())
		{
			int buttonPosX = ret->pos.w / 2 - buttonsWidth / 2;
			int buttonPosY = ret->pos.h - BOTTOM_MARGIN - ret->buttons[0]->pos.h;

			for(const auto & elem : ret->buttons)
			{
				elem->moveBy(Point(buttonPosX, buttonPosY));
				buttonPosX += elem->pos.w + INTERVAL_BETWEEN_BUTTONS;
			}
		}
	}

	ret->backgroundTexture->pos = ret->pos;
	ret->center();
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
