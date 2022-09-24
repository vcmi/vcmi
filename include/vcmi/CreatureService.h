/*
 * CreatureService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "EntityService.h"

VCMI_LIB_NAMESPACE_BEGIN

class CreatureID;
class Creature;

class DLL_LINKAGE CreatureService : public EntityServiceT<CreatureID, Creature>
{
public:
};

VCMI_LIB_NAMESPACE_END
