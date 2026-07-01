/*
 * Effect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Effect.h"

#include "../../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

bool Effect::applicableGeneral(Problem & problem, const Mechanics * m) const
{
	return true;
}

bool Effect::applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const
{
	return true;
}

void Effect::init(JsonNode data)
{
	indirect = data["indirect"].Bool();
	optional = data["optional"].Bool();
	initImpl(std::move(data));
}

}
}

VCMI_LIB_NAMESPACE_END
