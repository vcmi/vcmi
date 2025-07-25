/*
 * BattleConsole.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleConsole.h"

#include "BattleInterface.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../adventureMap/CInGameConsole.h"
#include "../eventsSDL/InputHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/CMessage.h"

BattleConsoleWindow::BattleConsoleWindow(const std::string & text)
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 429;
	pos.h = 434;

	updateShadow();
	center();

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	buttonOk = std::make_shared<CButton>(Point(183, 388), AnimationPath::builtin("IOKAY"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT);
	Rect textArea(18, 17, 393, 354);
	textBoxBackgroundBorder = std::make_shared<TransparentFilledRectangle>(textArea, ColorRGBA(0, 0, 0, 75), ColorRGBA(128, 100, 75));
	textBox = std::make_shared<CTextBox>(text, textArea.resize(-5), CSlider::BROWN);
	if(textBox->slider)
		textBox->slider->scrollToMax();
}

void BattleConsole::showAll(Canvas & to)
{
	CIntObject::showAll(to);

	Point line1(pos.x + pos.w / 2, pos.y + 8);
	Point line2(pos.x + pos.w / 2, pos.y + 24);

	auto visibleText = getVisibleText();

	if(visibleText.size() > 0)
		to.drawText(line1, FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, visibleText[0]);

	if(visibleText.size() > 1)
		to.drawText(line2, FONT_SMALL, Colors::WHITE, ETextAlignment::CENTER, visibleText[1]);
}

std::vector<std::string> BattleConsole::getVisibleText() const
{
	// high priority texts that hide battle log entries
	for(const auto & text : {consoleText, hoverText})
	{
		if(text.empty())
			continue;

		auto result = CMessage::breakText(text, pos.w, FONT_SMALL);

		if(result.size() > 2 && text.find('\n') != std::string::npos)
		{
			// Text has too many lines to fit into console, but has line breaks. Try ignore them and fit text that way
			std::string cleanText = boost::algorithm::replace_all_copy(text, "\n", " ");
			result = CMessage::breakText(cleanText, pos.w, FONT_SMALL);
		}

		if(result.size() > 2)
			result.resize(2);
		return result;
	}

	// log is small enough to fit entirely - display it as such
	if(logEntries.size() < 3)
		return logEntries;

	return {logEntries[scrollPosition - 1], logEntries[scrollPosition]};
}

std::vector<std::string> BattleConsole::splitText(const std::string & text) const
{
	std::vector<std::string> lines;
	std::vector<std::string> output;

	boost::split(lines, text, boost::is_any_of("\n"));

	const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
	for(const auto & line : lines)
	{
		if(font->getStringWidth(text) < pos.w)
		{
			output.push_back(line);
		}
		else
		{
			std::vector<std::string> substrings = CMessage::breakText(line, pos.w, FONT_SMALL);
			output.insert(output.end(), substrings.begin(), substrings.end());
		}
	}
	return output;
}

bool BattleConsole::addText(const std::string & text)
{
	logGlobal->trace("CBattleConsole message: %s", text);

	auto newLines = splitText(text);

	logEntries.insert(logEntries.end(), newLines.begin(), newLines.end());
	scrollPosition = static_cast<int>(logEntries.size()) - 1;
	redraw();
	return true;
}
void BattleConsole::scrollUp(ui32 by)
{
	if(scrollPosition > static_cast<int>(by))
		scrollPosition -= by;
	redraw();
}

void BattleConsole::scrollDown(ui32 by)
{
	if(scrollPosition + by < logEntries.size())
		scrollPosition += by;
	redraw();
}

BattleConsole::BattleConsole(const BattleInterface & owner, const std::shared_ptr<CPicture> & backgroundSource, const Point & objectPos, const Point & imagePos, const Point &size)
	: CIntObject(LCLICK)
	, owner(owner)
	, scrollPosition(-1)
	, enteringText(false)
{
	OBJECT_CONSTRUCTION;
	pos += objectPos;
	pos.w = size.x;
	pos.h = size.y;

	background = std::make_shared<CPicture>(backgroundSource->getSurface(), Rect(imagePos, size), 0, 0);
}

void BattleConsole::deactivate()
{
	if(enteringText)
		GAME->interface()->cingconsole->endEnteringText(false);

	CIntObject::deactivate();
}

void BattleConsole::clickPressed(const Point & cursorPosition)
{
	if(owner.makingTurn() && !owner.openingPlaying())
	{
		ENGINE->windows().createAndPushWindow<BattleConsoleWindow>(boost::algorithm::join(logEntries, "\n"));
	}
}

void BattleConsole::setEnteringMode(bool on)
{
	consoleText.clear();

	if(on)
	{
		assert(enteringText == false);
		ENGINE->input().startTextInput(pos);
	}
	else
	{
		assert(enteringText == true);
		ENGINE->input().stopTextInput();
	}
	enteringText = on;
	redraw();
}

void BattleConsole::setEnteredText(const std::string & text)
{
	assert(enteringText == true);
	consoleText = text;
	redraw();
}

void BattleConsole::write(const std::string & Text)
{
	hoverText = Text;
	redraw();
}

void BattleConsole::clearIfMatching(const std::string & Text)
{
	if(hoverText == Text)
		clear();
}

void BattleConsole::clear()
{
	write({});
}
