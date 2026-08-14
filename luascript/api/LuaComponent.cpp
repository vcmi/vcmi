/*
 * LuaComponent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaComponent.h"

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/networkPacks/Component.h"

namespace scripting::api
{

Component LuaComponent::toComponent() const
{
	auto componentType = static_cast<ComponentType>(type);

	Component result;
	result.type = componentType;
	result.value = value;

	switch(componentType)
	{
		case ComponentType::PRIM_SKILL:       result.subType = PrimarySkill(subType); break;
		case ComponentType::SEC_SKILL:        result.subType = SecondarySkill(subType); break;
		case ComponentType::RESOURCE:
		case ComponentType::RESOURCE_PER_DAY: result.subType = GameResID(subType); break;
		case ComponentType::CREATURE:         result.subType = CreatureID(subType); break;
		case ComponentType::ARTIFACT:
		case ComponentType::SPELL_SCROLL:     result.subType = ArtifactID(subType); break;
		case ComponentType::SPELL:            result.subType = SpellID(subType); break;
		case ComponentType::HERO_PORTRAIT:    result.subType = HeroTypeID(subType); break;
		case ComponentType::FLAG:             result.subType = PlayerColor(subType); break;
		default: break; // MANA / EXPERIENCE / LEVEL / MORALE / LUCK / BUILDING / NONE — no subtype
	}

	return result;
}

}
