/*
 * CInGameConsole.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CInGameConsole.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"
#include "../ClientCommandManager.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/TextAlignment.h"
#include "../render/Colors.h"
#include "../render/Canvas.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../windows/CMessage.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/ThreadUtilities.h"
#include "../../lib/TextOperations.h"
#include "../../lib/mapObjects/CArmedInstance.h"

CInGameConsole::CInGameConsole()
	: CIntObject(KEYBOARD | TIME | TEXTINPUT)
	, prevEntDisp(-1)
{
	setRedrawParent(true);
}

void CInGameConsole::showAll(Canvas & to)
{
	show(to);
}

void CInGameConsole::show(Canvas & to)
{
	if (LOCPLINT->cingconsole != this)
		return;

	int number = 0;

	boost::unique_lock<boost::mutex> lock(texts_mx);
	for(auto & text : texts)
	{
		Point leftBottomCorner(0, pos.h);
		Point textPosition(leftBottomCorner.x + 50, leftBottomCorner.y - texts.size() * 20 - 80 + number * 20);

		to.drawText(pos.topLeft() + textPosition, FONT_MEDIUM, Colors::GREEN, ETextAlignment::TOPLEFT, text.text);
		number++;
	}
}

void CInGameConsole::tick(uint32_t msPassed)
{
	size_t sizeBefore = texts.size();
	{
		boost::unique_lock<boost::mutex> lock(texts_mx);

		for(auto & text : texts)
			text.timeOnScreen += msPassed;

		vstd::erase_if(
			texts,
			[&](const auto & value)
			{
				return value.timeOnScreen > defaultTimeout;
			}
		);
	}

	if(sizeBefore != texts.size())
		GH.windows().totalRedraw(); // FIXME: ingame console has no parent widget set
}

void CInGameConsole::print(const std::string & txt)
{
	// boost::unique_lock scope
	{
		boost::unique_lock<boost::mutex> lock(texts_mx);

		// Maximum width for a text line is limited by:
		// 1) width of adventure map terrain area, for when in-game console is on top of advmap
		// 2) width of castle/battle window (fixed to 800) when this window is open
		// 3) arbitrary selected left and right margins
		int maxWidth = std::min( 800, adventureInt->terrainAreaPixels().w) - 100;

		auto splitText = CMessage::breakText(txt, maxWidth, FONT_MEDIUM);

		for(const auto & entry : splitText)
			texts.push_back({entry, 0});

		while(texts.size() > maxDisplayedTexts)
			texts.erase(texts.begin());
	}

	GH.windows().totalRedraw(); // FIXME: ingame console has no parent widget set
	CCS->soundh->playSound("CHAT");
}

bool CInGameConsole::captureThisKey(EShortcut key)
{
	if (enteredText.empty())
		return false;

	switch (key)
	{
		case EShortcut::GLOBAL_ACCEPT:
		case EShortcut::GLOBAL_CANCEL:
		case EShortcut::GAME_ACTIVATE_CONSOLE:
		case EShortcut::GLOBAL_BACKSPACE:
		case EShortcut::MOVE_UP:
		case EShortcut::MOVE_DOWN:
			return true;

		default:
			return false;
	}
}

void CInGameConsole::keyPressed (EShortcut key)
{
	if (LOCPLINT->cingconsole != this)
		return;

	if(enteredText.empty() && key != EShortcut::GAME_ACTIVATE_CONSOLE)
		return; //because user is not entering any text

	switch(key)
	{
	case EShortcut::GLOBAL_CANCEL:
		if(!enteredText.empty())
			endEnteringText(false);
		break;

	case EShortcut::GAME_ACTIVATE_CONSOLE:
		if(!enteredText.empty())
			endEnteringText(false);
		else
			startEnteringText();
		break;

	case EShortcut::GLOBAL_ACCEPT:
		{
			if(!enteredText.empty())
			{
				bool anyTextExceptCaret = enteredText.size() > 1;
				endEnteringText(anyTextExceptCaret);
			}
			break;
		}
	case EShortcut::GLOBAL_BACKSPACE:
		{
			if(enteredText.size() > 1)
			{
				TextOperations::trimRightUnicode(enteredText,2);
				enteredText += '_';
				refreshEnteredText();
			}
			break;
		}
	case EShortcut::MOVE_UP:
		{
			if(previouslyEntered.empty())
				break;

			if(prevEntDisp == -1)
			{
				prevEntDisp = static_cast<int>(previouslyEntered.size() - 1);
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			else if( prevEntDisp > 0)
			{
				--prevEntDisp;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			break;
		}
	case EShortcut::MOVE_DOWN:
		{
			if(prevEntDisp != -1 && prevEntDisp+1 < previouslyEntered.size())
			{
				++prevEntDisp;
				enteredText = previouslyEntered[prevEntDisp] + "_";
				refreshEnteredText();
			}
			else if(prevEntDisp+1 == previouslyEntered.size()) //useful feature
			{
				prevEntDisp = -1;
				enteredText = "_";
				refreshEnteredText();
			}
			break;
		}
	}
}

void CInGameConsole::textInputed(const std::string & inputtedText)
{
	if (LOCPLINT->cingconsole != this)
		return;

	if(enteredText.empty())
		return;

	enteredText.resize(enteredText.size()-1);

	enteredText += inputtedText;
	enteredText += "_";

	refreshEnteredText();
}

void CInGameConsole::textEdited(const std::string & inputtedText)
{
 //do nothing here
}

void CInGameConsole::startEnteringText()
{
	if (!isActive())
		return;

	assert(currentStatusBar.expired());//effectively, nullptr check

	currentStatusBar = GH.statusbar();

	enteredText = "_";

	GH.statusbar()->setEnteringMode(true);
	GH.statusbar()->setEnteredText(enteredText);
}

void CInGameConsole::endEnteringText(bool processEnteredText)
{
	prevEntDisp = -1;
	if(processEnteredText)
	{
		std::string txt = enteredText.substr(0, enteredText.size()-1);
		previouslyEntered.push_back(txt);

		if(txt.at(0) == '/')
		{
			//some commands like gosolo don't work when executed from GUI thread
			auto threadFunction = [=]()
			{
				setThreadName("processCommand");
				ClientCommandManager commandController;
				commandController.processCommand(txt.substr(1), true);
			};

			boost::thread clientCommandThread(threadFunction);
			clientCommandThread.detach();
		}
		else
			LOCPLINT->cb->sendMessage(txt, LOCPLINT->localState->getCurrentArmy());
	}
	enteredText.clear();

	auto statusbar = currentStatusBar.lock();
	assert(statusbar);

	if (statusbar)
		statusbar->setEnteringMode(false);

	currentStatusBar.reset();
}

void CInGameConsole::refreshEnteredText()
{
	auto statusbar = currentStatusBar.lock();
	assert(statusbar);

	if (statusbar)
		statusbar->setEnteredText(enteredText);
}

