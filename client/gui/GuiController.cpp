/*
 * GuiController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GuiController.h"
#include "../../CCallback.h"
#include "../../lib/CGameState.h"
#include "../CPlayerInterface.h"

void GsThread::run(std::function<void()> action)
{
	std::shared_ptr<GsThread> instance(new GsThread(action));


	boost::thread(std::bind(&GsThread::staticRun, instance));
}

GsThread::GsThread(std::function<void()> action)
	:action(action), cb(LOCPLINT->cb)
{
}

void GsThread::staticRun(std::shared_ptr<GsThread> instance)
{
	instance->run();
}

void GsThread::run()
{
	boost::shared_lock<boost::shared_mutex> gsLock(CGameState::mutex);

	auto originalWaitTillRealize = cb->waitTillRealize;
	auto originalUnlockGsWhenWating = cb->unlockGsWhenWaiting;

	cb->waitTillRealize = true;
	cb->unlockGsWhenWaiting = true;

	action();

	cb->waitTillRealize = originalWaitTillRealize;
	cb->unlockGsWhenWaiting = originalUnlockGsWhenWating;
}