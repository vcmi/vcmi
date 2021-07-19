/*
 * GuiController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

typedef std::vector<std::string> THandlerArgs;
class CCallback;

// Runs a task asynchronously with gamestate locking and waitTillRealize set to true
class GsThread
{
private:
	std::function<void()> action;
	std::shared_ptr<CCallback> cb;

public:
	static void run(std::function<void()> action);

private:
	GsThread(std::function<void()> action);

	static void staticRun(std::shared_ptr<GsThread> instance);

	void run();
};

class IUiController
{
public:
	virtual std::map<std::string, std::function<void(const THandlerArgs & args)>> getHandlers() = 0;
};