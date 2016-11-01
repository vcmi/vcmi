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
#include "../../lib/CRandomGenerator.h"
#include "../../CCallback.h"
#include "common.h"

void EnemyInfo::calcDmg(const CStack * ourStack)
{
	TDmgRange retal, dmg = getCbc()->battleEstimateDamage(CRandomGenerator::getDefault(), ourStack, s, &retal);
	adi = (dmg.first + dmg.second) / 2;
	adr = (retal.first + retal.second) / 2;
}
