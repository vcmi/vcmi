/*
 * Destination.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Destination.h"

#include "Unit.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{

Destination::Destination()
	: unitValue(nullptr),
	hexValue(BattleHex::INVALID)
{

}

Destination::~Destination() = default;

Destination::Destination(const battle::Unit * destination)
	: unitValue(destination),
	hexValue(destination->getPosition())
{

}

Destination::Destination(const BattleHex & destination)
	: unitValue(nullptr),
	hexValue(destination)
{

}

Destination::Destination(const Unit * destination, const BattleHex & exactHex)
	: unitValue(destination),
	hexValue(exactHex)
{

}

Destination::Destination(const Destination & other)
	: unitValue(other.unitValue),
	hexValue(other.hexValue)
{

}

Destination & Destination::operator=(const Destination & other)
{
	unitValue = other.unitValue;
	hexValue = other.hexValue;
	return *this;
}


}

VCMI_LIB_NAMESPACE_END
