/*
 * spells/Service.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../EntityService.h"

VCMI_LIB_NAMESPACE_BEGIN

class SpellID;

namespace spells
{
class Spell;

class DLL_LINKAGE Service : public EntityServiceT<SpellID, Spell>
{
public:
};

}

VCMI_LIB_NAMESPACE_END
