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

class DLL_LINKAGE TerminationRequestedException : public std::exception
{
public:
	using exception::exception;
	~TerminationRequestedException() override;

	const char* what() const noexcept override
	{
		return "Thread termination requested";
	}
};

class DLL_LINKAGE InterruptionRequestedException : public std::exception
{
public:
	using exception::exception;
	~InterruptionRequestedException() override;

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
			throw InterruptionRequestedException();
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
	int activeWaiters = 0;
	std::condition_variable cond;
	std::condition_variable waitersExited;
	std::mutex mx;

	void set(bool value)
	{
		std::unique_lock lock(mx);
		isBusyValue = value;
	}

public:
	ConditionalWait() = default;

	~ConditionalWait()
	{
		// Defensive: if owner forgot to call requestTermination(), block destruction
		// until any waiter has actually exited waitWhileBusy(). Without this, the
		// waiter can race past notify_all and re-lock a destroyed mutex.
		requestTermination();
	}

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
		std::unique_lock un(mx);
		isTerminating = true;
		isBusyValue = false;
		cond.notify_all();
		waitersExited.wait(un, [this]{ return activeWaiters == 0; });
	}

	bool isBusy()
	{
		std::unique_lock lock(mx);
		return isBusyValue;
	}

	void waitWhileBusy()
	{
		std::unique_lock un(mx);
		++activeWaiters;
		cond.wait(un, [this](){ return !isBusyValue;});

		const bool terminate = isTerminating;
		if (--activeWaiters == 0)
			waitersExited.notify_all();

		if (terminate)
			throw TerminationRequestedException();
	}
};
