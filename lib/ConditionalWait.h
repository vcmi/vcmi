/*
 * ConditionalWait.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <condition_variable>

VCMI_LIB_NAMESPACE_BEGIN

class TerminationRequestedException : public std::exception
{
public:
	using exception::exception;

	const char* what() const noexcept override
	{
		return "Thread termination requested";
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
			throw TerminationRequestedException();
	}

	void interruptThread()
	{
		interruptionRequested.store(true);
	}

	void reset()
	{
		interruptionRequested.store(false);
	}
};

class ConditionalWait
{
	bool isBusyValue = false;
	bool isTerminating = false;
	std::condition_variable cond;
	std::mutex mx;

	void set(bool value)
	{
		std::unique_lock lock(mx);
		isBusyValue = value;
	}

public:
	ConditionalWait() = default;

	void setBusy()
	{
		set(true);
	}

	void setFree()
	{
		set(false);
		cond.notify_all();
	}

	void requestTermination()
	{
		isTerminating = true;
		setFree();
	}

	bool isBusy()
	{
		std::unique_lock lock(mx);
		return isBusyValue;
	}

	void waitWhileBusy()
	{
		std::unique_lock un(mx);
		cond.wait(un, [this](){ return !isBusyValue;});

		if (isTerminating)
			throw TerminationRequestedException();
	}
};

VCMI_LIB_NAMESPACE_END
