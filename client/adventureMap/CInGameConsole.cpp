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

#include "../render/Colors.h"
#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../gui/CGuiHandler.h"
#include "../ClientCommandManager.h"

#include "../../CCallback.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapObjects/CArmedInstance.h"

#include <SDL_timer.h>

CInGameConsole::CInGameConsole()
	: CIntObject(KEYBOARD | TEXTINPUT),
	prevEntDisp(-1),
	defaultTimeout(10000),
	maxDisplayedTexts(10)
{
}

void CInGameConsole::show(SDL_Surface * to)
{
	int number = 0;

	std::vector<std::list< std::pair< std::string, uint32_t > >::iterator> toDel;

	boost::unique_lock<boost::mutex> lock(texts_mx);
	for(auto it = texts.begin(); it != texts.end(); ++it, ++number)
	{
		Point leftBottomCorner(0, pos.h);

		graphics->fonts[FONT_MEDIUM]->renderTextLeft(to, it->first, Colors::GREEN,
			Point(leftBottomCorner.x + 50, leftBottomCorner.y - (int)texts.size() * 20 - 80 + number*20));

		if((int)(SDL_GetTicks() - it->second) > defaultTimeout)
		{
			toDel.push_back(it);
		}
	}

	for(auto & elem : toDel)
	{
		texts.erase(elem);
	}
}

void CInGameConsole::print(const std::string &txt)
{
	boost::unique_lock<boost::mutex> lock(texts_mx);
	int lineLen = conf.go()->ac.outputLineLength;

	if(txt.size() < lineLen)
	{
		texts.push_back(std::make_pair(txt, SDL_GetTicks()));
		if(texts.size() > maxDisplayedTexts)
		{
			texts.pop_front();
		}
	}
	else
	{
		assert(lineLen);
		for(int g=0; g<txt.size() / lineLen + 1; ++g)
		{
			std::string part = txt.substr(g * lineLen, lineLen);
			if(part.size() == 0)
				break;

			texts.push_back(std::make_pair(part, SDL_GetTicks()));
			if(texts.size() > maxDisplayedTexts)
			{
				texts.pop_front();
			}
		}
	}
}

void CInGameConsole::keyPressed (const SDL_Keycode & key)
{
	if(!captureAllKeys && key != SDLK_TAB) return; //because user is not entering any text

	switch(key)
	{
	case SDLK_TAB:
	case SDLK_ESCAPE:
		{
			if(captureAllKeys)
			{
				endEnteringText(false);
			}
			else if(SDLK_TAB == key)
			{
				startEnteringText();
			}
			break;
		}
	case SDLK_RETURN: //enter key
		{
			if(!enteredText.empty() && captureAllKeys)
			{
				bool anyTextExceptCaret = enteredText.size() > 1;
				endEnteringText(anyTextExceptCaret);

				if(anyTextExceptCaret)
				{
					CCS->soundh->playSound("CHAT");
				}
			}
			break;
		}
	case SDLK_BACKSPACE:
		{
			if(enteredText.size() > 1)
			{
				Unicode::trimRight(enteredText,2);
				enteredText += '_';
				refreshEnteredText();
			}
			break;
		}
	case SDLK_UP: //up arrow
		{
			if(previouslyEntered.size() == 0)
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
	case SDLK_DOWN: //down arrow
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
	default:
		{
			break;
		}
	}
}

void CInGameConsole::textInputed(const std::string & inputtedText)
{
	if(!captureAllKeys || enteredText.size() == 0)
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
	if (!active)
		return;

	if (captureAllKeys)
		return;

	assert(GH.statusbar);
	assert(currentStatusBar.expired());//effectively, nullptr check

	currentStatusBar = GH.statusbar;

	captureAllKeys = true;
	enteredText = "_";

	GH.statusbar->setEnteringMode(true);
	GH.statusbar->setEnteredText(enteredText);
}

void CInGameConsole::endEnteringText(bool processEnteredText)
{
	captureAllKeys = false;
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
				ClientCommandManager commandController;
				commandController.processCommand(txt.substr(1), true);
			};

			boost::thread clientCommandThread(threadFunction);
			clientCommandThread.detach();
		}
		else
			LOCPLINT->cb->sendMessage(txt, LOCPLINT->getSelection());
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

