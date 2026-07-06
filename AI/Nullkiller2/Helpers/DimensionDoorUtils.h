/*
* DimensionDoorUtils.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../../../lib/GameLibrary.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/spells/adventure/DimensionDoorEffect.h"

namespace NK2AI
{

template<typename Function>
void forEachDimensionDoorSpell(const CGHeroInstance * hero, Function function)
{
	if(!hero)
		return;

	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(!spell || !spell->isAdventure())
			continue;

		const auto & mechanics = spell->getAdventureMechanics();
		const auto * effect = mechanics.getEffectAs<DimensionDoorEffect>(hero);

		if(!effect)
			continue;

		function(spell.get(), mechanics, effect);
	}
}

}
