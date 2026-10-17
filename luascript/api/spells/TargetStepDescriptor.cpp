/*
 * TargetStepDescriptor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TargetStepDescriptor.h"

#include "../../../lib/spells/effects/Effect.h"

namespace scripting::api
{

::spells::effects::EffectTargetStep TargetStepDescriptor::toTargetStep() const
{
	return {{cursor, text.toMetaString(), blockedText.toMetaString()}, healthGainTextID};
}

}
