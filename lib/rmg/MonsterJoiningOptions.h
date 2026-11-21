/*
 * MonsterJoiningOptions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../mapObjects/CGCreature.h"

class JsonSerializeFormat;

namespace rmg
{

/// Joining settings for guards the generator places inside a zone
class DLL_LINKAGE MonsterJoiningOptions
{
public:
	std::optional<CGCreature::Character> character;
	std::optional<int8_t> aggression;
	std::optional<int8_t> joiningPercentage;
	std::optional<bool> joinOnlyForMoney;

	void serializeJson(JsonSerializeFormat & handler);
	/// Per field, override's value when set, otherwise base's
	static MonsterJoiningOptions merge(const MonsterJoiningOptions & base, const MonsterJoiningOptions & override);
	/// Writes initialCharacter (+ agression for CUSTOM), joiningPercentage, joinOnlyForMoney; unset fields get the defaults above
	void applyTo(CGCreature & guard) const;
};

}
