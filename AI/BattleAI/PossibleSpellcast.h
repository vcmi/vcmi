/*
 * PossibleSpellcast.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Magic.h>

#include "../../lib/battle/Destination.h"

VCMI_LIB_NAMESPACE_BEGIN

class CSpell;

VCMI_LIB_NAMESPACE_END

class PossibleSpellcast
{
public:
	using ValueMap = std::map<uint32_t, int64_t>;

	const CSpell * spell;
	spells::Target dest;
	float value;

	PossibleSpellcast();
	virtual ~PossibleSpellcast();
};
