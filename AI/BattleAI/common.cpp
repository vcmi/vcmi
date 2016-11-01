/*
 * common.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "common.h"

std::shared_ptr<CBattleCallback> cbc;

void setCbc(std::shared_ptr<CBattleCallback> cb)
{
	cbc = cb;
}

std::shared_ptr<CBattleCallback> getCbc()
{
	return cbc;
}
