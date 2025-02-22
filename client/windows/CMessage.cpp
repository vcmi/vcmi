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

#include "../GameEngine.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Images.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"

#include "../../lib/texts/TextOperations.h"

constexpr int RIGHT_CLICK_POPUP_MIN_SIZE = 100;
constexpr int RIGHT_CLICK_POPUP_MAX_HEIGHT_TEXTONLY = 450;
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
		dialogBorders[i] = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("DIALGBOX"), EImageBlitMode::COLORKEY);

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
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
	const auto lines = TextOperations::breakText(txt, width,
		[fontPtr](const std::string & str) -> size_t {return fontPtr->getStringWidth(str);});
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
		if(ret->text->slider)
		{
			ret->text->resize(Point(ret->text->pos.w, std::min(ret->text->label->textSize.y, RIGHT_CLICK_POPUP_MAX_HEIGHT_TEXTONLY)));
			ret->pos.w = std::max(RIGHT_CLICK_POPUP_MIN_SIZE, ret->text->pos.w + 2 * SIDE_MARGIN);
			ret->pos.h = std::max(RIGHT_CLICK_POPUP_MIN_SIZE, ret->text->pos.h + TOP_MARGIN + BOTTOM_MARGIN);
		}
		else
		{
			ret->pos.w = std::max(RIGHT_CLICK_POPUP_MIN_SIZE, ret->text->label->textSize.x + 2 * SIDE_MARGIN);
			ret->pos.h = std::max(RIGHT_CLICK_POPUP_MIN_SIZE, ret->text->label->textSize.y + TOP_MARGIN + BOTTOM_MARGIN);
		}
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
