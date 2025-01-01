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

bool CMessage::validateTags(
	const std::vector<std::string_view::const_iterator> & openingTags,
	const std::vector<std::string_view::const_iterator> & closingTags)
{
	if(openingTags.size() == closingTags.size())
	{
		if(!openingTags.empty())
		{
			auto closingPos = closingTags.begin();
			auto closingPrevPos = openingTags.front();
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
		}
	}
	else
	{
		logGlobal->error("CMessage: Not Equal number opening and closing tags");
		return false;
	}
	return true;
}

std::vector<CMessage::coloredline> CMessage::splitLineBySpaces(const std::string_view & line, size_t maxLineWidth, const EFonts font)
{
	const auto & fontPtr = GH.renderHandler().loadFont(font);
	std::vector<coloredline> result;

	const auto findInStr =
		[](const char & targetChar, const std::string_view & str) -> std::vector<std::string_view::const_iterator>
		{
			std::vector<std::string_view::const_iterator> results;
			for(auto c = str.cbegin(); c < str.cend(); c++)
			{
				if(*c == targetChar)
					results.push_back(c);
			}
			return results;
		};

	auto openingBraces = findInStr('{', line);
	auto closingBraces = findInStr('}', line);
	if(!validateTags(openingBraces, closingBraces))
	{
		logGlobal->error("CMessage: Line validation Invalid: %s", line);
		return result;
	}
	if(!validateTags(findInStr('#', line), findInStr('|', line)))
	{
		logGlobal->error("CMessage: Line validation Invalid: %s", line);
		return result;
	}

	const auto isIteratorInStr = [](const std::string_view::const_iterator & pos, const std::string_view & str) -> bool
		{
			const auto strBegin = str.data();
			const auto strEnd = strBegin + str.size();
			return strBegin <= &(*pos) && &(*pos) < strEnd;
		};

	std::string_view lastFoundColor;
	const std::function<size_t(const std::string_view&)> findColorTags = [&](const std::string_view & str) -> size_t
		{
			if(closingBraces.empty())
				return 0;

			size_t foundWidth = 0;
			if(openingBraces.size() == closingBraces.size())
			{
				if(isIteratorInStr(openingBraces.front(), str))
				{
					foundWidth += fontPtr->getStringWidth("{");
					const auto colorTagBegin = std::next(openingBraces.front());
					// Opened color tag brace found. Try to find color code.
					if(*colorTagBegin == '#')
					{
						const auto colorTagEnd = std::find(colorTagBegin, closingBraces.front(), '|');
						assert(colorTagEnd != closingBraces.front());
						foundWidth += fontPtr->getStringWidth(std::string(&(*colorTagBegin), std::distance(colorTagBegin, std::next(colorTagEnd))));
						//lastFoundColor = std::string_view(openingBracers.front(), std::next(colorTagEnd)); // Use this if C++20 is available
						lastFoundColor = std::string_view(&(*openingBraces.front()), std::distance(openingBraces.front(), std::next(colorTagEnd)));
					}
					else
					{
						//lastFoundColor = std::string_view(openingBracers.front(), std::next(openingBracers.front())); // Use this if C++20 is available
						lastFoundColor = std::string_view(&(*openingBraces.front()), std::distance(openingBraces.front(), std::next(openingBraces.front())));
					}
					openingBraces.erase(openingBraces.begin());
				}
				else
				{
					return foundWidth;
				}
			}

			if(isIteratorInStr(closingBraces.front(), str))
			{
				closingBraces.erase(closingBraces.begin());
				foundWidth += fontPtr->getStringWidth("}");
				foundWidth += findColorTags(str);
				lastFoundColor = std::string_view();
			}
			return foundWidth;
		};

	const std::function<void(const std::string_view&)> splitBySpaces = [&](const std::string_view & subLine)
		{
			// Trim spaces from beginning
			const auto subLineBeginPos = subLine.find_first_not_of(' ');
			if(subLineBeginPos == std::string::npos)
				return;

			const auto trimmedLine = subLine.substr(subLineBeginPos, subLine.length() - subLineBeginPos);
			const auto lineBegin = trimmedLine.cbegin();
			auto wordBegin = lineBegin;
			const auto spaces = findInStr(' ', trimmedLine);
			if(spaces.empty())
			{
				result.back().line = subLine;
				return;
			}

			size_t lineWidth = 0;
			//const auto curWord = std::string_view(wordBegin, spaces.front()); // Use this if C++20 is available
			const auto curWord = std::string_view(&(*wordBegin), std::distance(wordBegin, spaces.front()));
			// TODO std::string should be optimized to std::string_view
			size_t nextWordWidth = fontPtr->getStringWidth(std::string(curWord)) - findColorTags(curWord);

			for(auto space = spaces.cbegin(); space < spaces.cend(); space = std::next(space))
			{
				lineWidth += nextWordWidth;
				const auto curColor = lastFoundColor;
				const auto wordEnd = *space;
				const auto nextSpace = std::next(space);
				const auto nextWord = nextSpace == spaces.end() ? std::string_view(&(*wordEnd), std::distance(wordEnd, trimmedLine.end()))
					// : std::string_view(wordEnd, *nextSpace); // Use this if C++20 is available
					: std::string_view(&(*wordEnd), std::distance(wordEnd, *nextSpace));
				nextWordWidth = fontPtr->getStringWidth(std::string(nextWord)) - findColorTags(nextWord);

				if(lineWidth + nextWordWidth > maxLineWidth)
				{
					const auto resultLine = trimmedLine.substr(std::distance(trimmedLine.cbegin(), lineBegin), std::distance(lineBegin, wordEnd));
					// Trim spaces from ending and place result line into result
					result.back().line = resultLine.substr(0, resultLine.find_last_not_of(' ') + 1);
					if(!curColor.empty())
						result.back().closingTagNeeded = true;
					result.emplace_back(std::string_view(), curColor);
					splitBySpaces(trimmedLine.substr(std::distance(trimmedLine.cbegin(), wordEnd), std::distance(wordEnd, trimmedLine.end())));
					return;
				}
				else
				{
					if(wordBegin == wordEnd)
						lineWidth += fontPtr->getStringWidth(" ");
					wordBegin = std::next(wordEnd);
				}
			}
			//result.back().line = std::string_view(lineBegin, trimmedLine.cend()); // Use this if C++20 is available
			result.back().line = std::string_view(&(*lineBegin), std::distance(lineBegin, trimmedLine.cend()));
		};

	result.emplace_back(std::string_view(), std::string_view());
	splitBySpaces(line);

	return result;
}

std::vector<std::string> CMessage::breakText(const std::string_view & text, size_t maxLineWidth, const EFonts font)
{
	if(maxLineWidth == 0)
		return {std::string()};

	std::vector<std::string> result;

	const auto addLinesToResult = [&result](const std::vector<coloredline> & lines)
		{
			for(const auto & line : lines)
			{
				result.emplace_back(std::string(line.startColorTag) + std::string(line.line));
				if(line.closingTagNeeded)
					result.back().append("}");
			}
		};

	// Firstly split text by new lines. Then split each line by spaces.
	auto endPos = std::string::npos;
	for(size_t beginPos = 0; beginPos < text.length();)
	{
		endPos = text.find('\n', beginPos);
		if(endPos != std::string::npos)
		{
			if(endPos > beginPos)
				addLinesToResult(splitLineBySpaces(text.substr(beginPos, endPos - beginPos), maxLineWidth, font));
			beginPos = text.find_first_not_of('\n', endPos);

			for(size_t emptyLine = 1; emptyLine < beginPos - endPos; emptyLine++)
				result.emplace_back("");
		}
		else
		{
			addLinesToResult(splitLineBySpaces(text.substr(beginPos, text.length() - beginPos), maxLineWidth, font));
			break;
		}
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
