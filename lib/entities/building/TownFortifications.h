/*
 * TownFortifications.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

struct TownFortifications
{
	CreatureID citadelShooter;
	CreatureID upperTowerShooter;
	CreatureID lowerTowerShooter;
	SpellID moatSpell;

	int8_t wallsHealth = 0;
	int8_t citadelHealth = 0;
	int8_t upperTowerHealth = 0;
	int8_t lowerTowerHealth = 0;
	bool hasMoat = false;

	const TownFortifications & operator +=(const TownFortifications & other)
	{
		if (other.citadelShooter.hasValue())
			citadelShooter = other.citadelShooter;
		if (other.upperTowerShooter.hasValue())
			upperTowerShooter = other.upperTowerShooter;
		if (other.lowerTowerShooter.hasValue())
			lowerTowerShooter = other.lowerTowerShooter;
		if (other.moatSpell.hasValue())
			moatSpell = other.moatSpell;

		wallsHealth = std::max(wallsHealth, other.wallsHealth);
		citadelHealth = std::max(citadelHealth, other.citadelHealth);
		upperTowerHealth = std::max(upperTowerHealth, other.upperTowerHealth);
		lowerTowerHealth = std::max(lowerTowerHealth, other.lowerTowerHealth);
		hasMoat = hasMoat || other.hasMoat;
		return *this;
	}
};

VCMI_LIB_NAMESPACE_END
