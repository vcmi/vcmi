#include "StdInc.h"
#include "LoadProgress.h"

using namespace Load;

Progress::Progress(): _progress(std::numeric_limits<Type>::min())
{
	steps(100);
}

Type Progress::get() const
{
	return _progress;
}

void Progress::set(Type p)
{
	_progress = p;
}

bool Progress::finished() const
{
	return _progress == std::numeric_limits<Type>::max();
}

void Progress::reset(int s)
{
	_progress = std::numeric_limits<Type>::min();
	steps(s);
}

void Progress::finish()
{
	_progress = std::numeric_limits<Type>::max();
}

void Progress::steps(int s)
{
	stepsTill(s, std::numeric_limits<Type>::max());
}

void Progress::stepsTill(int s, Type p)
{
	if(p < _progress)
		_step = 0;
	else
		_step = (p - _progress) / s;
}

void Progress::step(int count)
{
	for(int i = 0; i < count; ++i)
	{
		if(_progress + _step < _progress)
		{
			finish();
			return;
		}
		_progress += _step;
	}
}
