/*
 * WindowHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WindowHandler.h"

#include "CGuiHandler.h"
#include "CIntObject.h"
#include "CursorHandler.h"

#include "../CMT.h"
#include "../CGameInfo.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../renderSDL/SDL_Extensions.h"

void WindowHandler::popWindow(std::shared_ptr<IShowActivatable> top)
{
	assert(windowsStack.back() == top);
	top->deactivate();
	disposed.push_back(top);
	windowsStack.pop_back();
	if(!windowsStack.empty())
		windowsStack.back()->activate();

	totalRedraw();
}

void WindowHandler::pushWindow(std::shared_ptr<IShowActivatable> newInt)
{
	assert(newInt);
	assert(!vstd::contains(windowsStack, newInt)); // do not add same object twice

	//a new interface will be present, we'll need to use buffer surface (unless it's advmapint that will alter screenBuf on activate anyway)
	screenBuf = screen2;

	if(!windowsStack.empty())
		windowsStack.back()->deactivate();
	windowsStack.push_back(newInt);
	CCS->curh->set(Cursor::Map::POINTER);
	newInt->activate();
	totalRedraw();
}

bool WindowHandler::isTopWindowPopup() const
{
	if (windowsStack.empty())
		return false;

	return windowsStack.back()->isPopupWindow();
}

void WindowHandler::popWindows(int howMany)
{
	if(!howMany)
		return; //senseless but who knows...

	assert(windowsStack.size() >= howMany);
	windowsStack.back()->deactivate();
	for(int i = 0; i < howMany; i++)
	{
		disposed.push_back(windowsStack.back());
		windowsStack.pop_back();
	}

	if(!windowsStack.empty())
	{
		windowsStack.back()->activate();
		totalRedraw();
	}
	GH.fakeMouseMove();
}

std::shared_ptr<IShowActivatable> WindowHandler::topWindowImpl() const
{
	if(windowsStack.empty())
		return nullptr;

	return windowsStack.back();
}

bool WindowHandler::isTopWindow(std::shared_ptr<IShowActivatable> window) const
{
	assert(window != nullptr);
	return !windowsStack.empty() && windowsStack.back() == window;
}

bool WindowHandler::isTopWindow(IShowActivatable * window) const
{
	assert(window != nullptr);
	return !windowsStack.empty() && windowsStack.back().get() == window;
}

void WindowHandler::totalRedraw()
{
	CSDL_Ext::fillSurface(screen2, Colors::BLACK);

	Canvas target = Canvas::createFromSurface(screen2);

	for(auto & elem : windowsStack)
		elem->showAll(target);
	CSDL_Ext::blitAt(screen2, 0, 0, screen);
}

void WindowHandler::simpleRedraw()
{
	//update only top interface and draw background
	if(windowsStack.size() > 1)
		CSDL_Ext::blitAt(screen2, 0, 0, screen); //blit background

	Canvas target = Canvas::createFromSurface(screen);

	if(!windowsStack.empty())
		windowsStack.back()->show(target); //blit active interface/window
}

void WindowHandler::onScreenResize()
{
	for(const auto & entry : windowsStack)
		entry->onScreenResize();

	totalRedraw();
}

void WindowHandler::onFrameRendered()
{
	disposed.clear();
}

size_t WindowHandler::count() const
{
	return windowsStack.size();
}

void WindowHandler::clear()
{
	if(!windowsStack.empty())
		windowsStack.back()->deactivate();

	windowsStack.clear();
	disposed.clear();
}
