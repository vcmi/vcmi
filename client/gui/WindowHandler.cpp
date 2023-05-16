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
#include "../render/Colors.h"
#include "../renderSDL/SDL_Extensions.h"

void WindowHandler::popInt(std::shared_ptr<IShowActivatable> top)
{
	assert(listInt.front() == top);
	top->deactivate();
	disposed.push_back(top);
	listInt.pop_front();
	objsToBlit -= top;
	if(!listInt.empty())
		listInt.front()->activate();
	totalRedraw();
}

void WindowHandler::pushInt(std::shared_ptr<IShowActivatable> newInt)
{
	assert(newInt);
	assert(!vstd::contains(listInt, newInt)); // do not add same object twice

	//a new interface will be present, we'll need to use buffer surface (unless it's advmapint that will alter screenBuf on activate anyway)
	screenBuf = screen2;

	if(!listInt.empty())
		listInt.front()->deactivate();
	listInt.push_front(newInt);
	CCS->curh->set(Cursor::Map::POINTER);
	newInt->activate();
	objsToBlit.push_back(newInt);
	totalRedraw();
}

void WindowHandler::popInts(int howMany)
{
	if(!howMany)
		return; //senseless but who knows...

	assert(listInt.size() >= howMany);
	listInt.front()->deactivate();
	for(int i = 0; i < howMany; i++)
	{
		objsToBlit -= listInt.front();
		disposed.push_back(listInt.front());
		listInt.pop_front();
	}

	if(!listInt.empty())
	{
		listInt.front()->activate();
		totalRedraw();
	}
	GH.fakeMouseMove();
}

std::shared_ptr<IShowActivatable> WindowHandler::topInt()
{
	if(listInt.empty())
		return std::shared_ptr<IShowActivatable>();
	else
		return listInt.front();
}

void WindowHandler::totalRedraw()
{
	CSDL_Ext::fillSurface(screen2, Colors::BLACK);

	for(auto & elem : objsToBlit)
		elem->showAll(screen2);
	CSDL_Ext::blitAt(screen2, 0, 0, screen);
}

void WindowHandler::simpleRedraw()
{
	//update only top interface and draw background
	if(objsToBlit.size() > 1)
		CSDL_Ext::blitAt(screen2, 0, 0, screen); //blit background
	if(!objsToBlit.empty())
		objsToBlit.back()->show(screen); //blit active interface/window
}

void WindowHandler::onScreenResize()
{
	for(const auto & entry : listInt)
	{
		auto intObject = std::dynamic_pointer_cast<CIntObject>(entry);

		if(intObject)
			intObject->onScreenResize();
	}

	totalRedraw();
}

void WindowHandler::onFrameRendered()
{
	disposed.clear();
}
