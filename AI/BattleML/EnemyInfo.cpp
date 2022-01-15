/*
 * EnemyInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EnemyInfo.h"

#include "../../lib/battle/Unit.h"

bool EnemyInfo::operator==(const EnemyInfo & ei) const
{
	return s->unitId() == ei.s->unitId();
}

