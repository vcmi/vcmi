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

#include "GameEngine.h"
#include "CIntObject.h"
#include "CursorHandler.h"

#include "../render/Canvas.h"
#include "../render/IScreenHandler.h"
#include "../render/Colors.h"

void WindowHandler::popWindow(std::shared_ptr<IShowActivatable> top)
{
	if (windowsStack.back() != top)
		throw std::runtime_error("Attempt to pop non-top window from stack!");

	top->deactivate();
	disposed.push_back(top);
	windowsStack.pop_back();
	if(!windowsStack.empty())
		windowsStack.back()->activate();

	totalRedraw();
}

void WindowHandler::pushWindow(std::shared_ptr<IShowActivatable> newInt)
{
	if (newInt == nullptr)
		throw std::runtime_error("Attempt to push null window onto windows stack!");

	if (vstd::contains(windowsStack, newInt))
		throw std::runtime_error("Attempt to add already existing window to stack!");

	if(!windowsStack.empty())
		windowsStack.back()->deactivate();
	windowsStack.push_back(newInt);
	ENGINE->cursor().set(Cursor::Map::POINTER);
	newInt->activate();
	totalRedraw();
}

bool WindowHandler::tryReplaceWindow(std::shared_ptr<IShowActivatable> oldInt, std::shared_ptr<IShowActivatable> newInt)
{
	int pos = vstd::find_pos(windowsStack, oldInt);
	if(pos == -1)
		return false;

	if(newInt == nullptr)
		throw std::runtime_error("Attempt to replace null window onto windows stack!");

	if(vstd::contains(windowsStack, newInt))
		throw std::runtime_error("Attempt to add already existing window to stack!");

	windowsStack[pos] = newInt;
	if(!windowsStack.empty())
		windowsStack.back()->activate();
	totalRedraw();
	return true;
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
	ENGINE->fakeMouseMove();
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
	totalRedrawRequested = true;
}

void WindowHandler::totalRedrawImpl()
{
	logGlobal->debug("totalRedraw requested!");

	Canvas target = ENGINE->screenHandler().getScreenCanvas();

	for(auto & elem : windowsStack)
		elem->showAll(target);
}

void WindowHandler::simpleRedraw()
{
	if (totalRedrawRequested)
		totalRedrawImpl();
	else
		simpleRedrawImpl();

	totalRedrawRequested = false;
}

void WindowHandler::simpleRedrawImpl()
{
	Canvas target = ENGINE->screenHandler().getScreenCanvas();

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
