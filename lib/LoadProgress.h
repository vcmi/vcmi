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
