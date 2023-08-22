/*
 * LoadProgress.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "LoadProgress.h"

using namespace Load;

Progress::Progress(): _progress(std::numeric_limits<Type>::min())
{
	setupSteps(100);
}

Progress::Progress(int steps): _progress(std::numeric_limits<Type>::min())
{
	setupSteps(steps);
}

Type Progress::get() const
{
	if(_step >= _maxSteps)
		return _target;
	
	if(!_maxSteps)
		return _progress;
	
	return static_cast<int>(_progress) + _step * static_cast<int>(_target - _progress) / _maxSteps;
}

void Progress::set(Type p)
{
	_progress = p;
}

bool Progress::finished() const
{
	return get() == std::numeric_limits<Type>::max();
}

void Progress::reset(int s)
{
	_progress = std::numeric_limits<Type>::min();
	setupSteps(s);
}

void Progress::finish()
{
	_progress = _target = std::numeric_limits<Type>::max();
	_step = std::numeric_limits<Type>::min();
	_maxSteps = std::numeric_limits<Type>::min();
}

void Progress::setupSteps(int s)
{
	setupStepsTill(s, std::numeric_limits<Type>::max());
}

void Progress::setupStepsTill(int s, Type p)
{
	if(finished())
		return;
	
	if(_step > std::numeric_limits<Type>::min())
		_progress = get();
	
	_step = std::numeric_limits<Type>::min();
	_maxSteps = s;
	
	_target = p;
}

void Progress::step(int count)
{
	if(_step + count > _maxSteps)
	{
		_step = _maxSteps.load();
	}
	else
	{
		_step += count;
	}
}

void ProgressAccumulator::include(const Progress & p)
{
	boost::unique_lock<boost::mutex> guard(_mx);
	_progress.emplace_back(p);
}

void ProgressAccumulator::exclude(const Progress & p)
{
	boost::unique_lock<boost::mutex> guard(_mx);
	for(auto i = _progress.begin(); i != _progress.end(); ++i)
	{
		if(&i->get() == &p)
		{
			_accumulated += static_cast<long long>(p.get()) * p._maxSteps;
			_steps += p._maxSteps;
			_progress.erase(i);
			return;
		}
	}
}

bool ProgressAccumulator::finished() const
{
	boost::unique_lock<boost::mutex> guard(_mx);
	for(auto i : _progress)
		if(!i.get().finished())
			return false;
	return true;
}

Type ProgressAccumulator::get() const
{
	boost::unique_lock<boost::mutex> guard(_mx);
	auto sum = _accumulated;
	auto totalSteps = _steps;
	for(auto p : _progress)
	{
		sum += static_cast<long long>(p.get().get()) * p.get()._maxSteps;
		totalSteps += p.get()._maxSteps;
	}
	
	if(totalSteps)
		sum /= totalSteps;
	return static_cast<Type>(sum);
}
