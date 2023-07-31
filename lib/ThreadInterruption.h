/*
 * ThreadInterruption.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class ThreadInterrupted : public std::exception
{
public:
	const char * what() const noexcept override
	{
		return "Thread interruption has been requested";
	}
};

class ThreadInterruption
{
	std::atomic<bool> interruptionRequested = false;

public:
	void interruptionPoint()
	{
		bool result = interruptionRequested.exchange(false);

		if (result)
			throw ThreadInterrupted();
	}

	void interruptThread()
	{
		interruptionRequested.store(true);
	}
};

VCMI_LIB_NAMESPACE_END
