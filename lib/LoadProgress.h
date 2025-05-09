/*
 * LoadProgress.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StdInc.h"
#include <atomic>

namespace Load
{

using Type = unsigned char;

class ProgressAccumulator;

/*
 * Purpose of that class is to track progress of computations
 * Derive from this class if you want to translate user or system
 * remaining amount of work needed.
 * Tracking of the progress should be from another thread.
 */
class DLL_LINKAGE Progress
{
public:
	
	//Sets current state to 0.
	//Amount of steps to finish progress will be equal to 100 for default constructor
	Progress();
	Progress(int steps);
	virtual ~Progress() = default;

	//Returns current state of the progress
	//To translate it into percentage (to float, for example):
	//float progress = <>.get() / std::numeric_limits<Load::Type>::max();
	Type get() const;
	
	//Returns true if current state equal to final state, false otherwise
	bool finished() const;
	
	//Sets current state equal to the argument
	void set(Type);
	
	//Sets current state to 0
	//steps - amount of steps needed to reach final state
	void reset(int steps = 100);
	
	//Immediately sets state to final
	//finished() will return true after calling this method
	void finish();
	
	//Sets amount of steps needed to reach final state
	//doesn't modify current state
	void setupSteps(int steps);
	
	//Sets amount of steps needed to reach state specified
	//doesn't modify current state
	void setupStepsTill(int steps, Type state);
	
	//Increases current state by steps count
	//if current state reaches final state, returns immediately
	void step(int count = 1);

private:
	std::atomic<Type> _progress;
	std::atomic<Type> _target;
	std::atomic<int> _step;
	std::atomic<int> _maxSteps;
	
	friend class ProgressAccumulator;
};

class DLL_LINKAGE ProgressAccumulator
{
public:
	ProgressAccumulator() = default;
	
	void include(const Progress &);
	void exclude(const Progress &);
	
	bool finished() const;
	Type get() const;
	
private:
	mutable std::mutex _mx;
	long long _accumulated = 0;
	long long _steps = 0;
	std::vector<std::reference_wrapper<const Progress>> _progress;
};

}
