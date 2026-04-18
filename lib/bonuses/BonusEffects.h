/*
 * BonusEffects.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Bonus.h"
#include "Trigger.h"

#include <constants/EntityIdentifiers.h>

VCMI_LIB_NAMESPACE_BEGIN

struct BBGrantBonus
{
	Trigger trigger;
	std::shared_ptr<Bonus> bonus;
	bool targetEnemy = false;

	template<class H>
	void serialize(H & h)
	{
		h & trigger;
		h & bonus;
		h & targetEnemy;
	}
};

struct BBCastSpell
{
	Trigger trigger;
	SpellID spell;
	int masteryLevel = 0;
	bool targetEnemy = false;

	template<class H>
	void serialize(H & h)
	{
		h & trigger;
		h & spell;
		h & masteryLevel;
		h & targetEnemy;
	}
};

struct BBTerminate
{
	Trigger trigger;
	template<class H>
	void serialize(H & h)
	{
		h & trigger;
	}
};

struct BBChangeDuration
{
	Trigger trigger;
	int value;

	template<class H>
	void serialize(H & h)
	{
		h & trigger;
		h & value;
	}
};

VCMI_LIB_NAMESPACE_END
