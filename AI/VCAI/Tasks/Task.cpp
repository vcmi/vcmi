/*
* Goals.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Task.h"

using namespace Tasks;

TaskPtr Tasks::sptr(const CTask & tmp)
{
	TaskPtr ptr;
	ptr.reset(tmp.clone());

	return ptr;
}