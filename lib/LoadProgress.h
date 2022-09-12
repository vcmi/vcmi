#ifndef LOADPROGRESS_H
#define LOADPROGRESS_H
#include "StdInc.h"
#include <atomic>

namespace Load
{

using Type = unsigned char;

class DLL_LINKAGE Progress
{
public:
	Progress();
	virtual ~Progress() = default;

	Type get() const;
	bool finished() const;
	void set(Type);
	void reset(int s = 100);
	void finish();
	void steps(int);
	void stepsTill(int, Type);
	void step(int count = 1);

private:
	std::atomic<Type> _progress, _step;
};
}

#endif // LOADPROGRESS_H
