/*
 * BonusParameters.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BonusParameters.h"

#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string BonusParameters::toString() const
{
    return toJsonNode().toCompactString();
}

JsonNode BonusParameters::toJsonNode() const
{
    return JsonNode(); // TODO
}

VCMI_LIB_NAMESPACE_END
