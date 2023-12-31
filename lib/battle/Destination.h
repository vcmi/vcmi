/*
 * Destination.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{

class Unit;

class DLL_LINKAGE Destination
{
public:
	Destination();
	~Destination() = default;
	explicit Destination(const Unit * destination);
	explicit Destination(const BattleHex & destination);
	explicit Destination(const Unit * destination, const BattleHex & exactHex);

	Destination(const Destination & other) = default;

	Destination & operator=(const Destination & other) = default;

	const Unit * unitValue;
	BattleHex hexValue;
};

using Target = std::vector<Destination>;

}

VCMI_LIB_NAMESPACE_END
