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

#include "../LuaStack.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/networkPacks/Component.h"

namespace scripting::api
{

/// Lookup throws on an index that does not name a loaded entity
template<typename IdentifierType>
static IdentifierType validated(int raw)
{
	IdentifierType identifier(raw);
	identifier.toEntity(LIBRARY);
	return identifier;
}

Component LuaComponent::toComponent() const
{
	// components arrive from map scripts and are displayed by every client, so reject anything a client cannot show
	auto componentType = static_cast<ComponentType>(type);

	Component result;
	result.type = componentType;
	result.value = value;

	switch(componentType)
	{
		case ComponentType::PRIM_SKILL:
			if(subType < 0 || subType >= static_cast<int>(PrimarySkill::ALL_SKILLS().size()))
				throw LuaApiException("Component: invalid primary skill " + std::to_string(subType));
			result.subType = PrimarySkill(subType);
			break;
		case ComponentType::SEC_SKILL:
			if(value && (*value < MasteryLevel::BASIC || *value > MasteryLevel::EXPERT))
				throw LuaApiException("Component: invalid secondary skill level " + std::to_string(*value));
			result.subType = validated<SecondarySkill>(subType);
			break;
		case ComponentType::RESOURCE:
		case ComponentType::RESOURCE_PER_DAY:
			result.subType = validated<GameResID>(subType);
			break;
		case ComponentType::CREATURE:
			result.subType = validated<CreatureID>(subType);
			break;
		case ComponentType::ARTIFACT:
			result.subType = validated<ArtifactID>(subType);
			break;
		case ComponentType::SPELL_SCROLL:
		case ComponentType::SPELL:
			result.subType = validated<SpellID>(subType);
			break;
		case ComponentType::HERO_PORTRAIT:
			result.subType = validated<HeroTypeID>(subType);
			break;
		case ComponentType::FLAG:
			if(!PlayerColor(subType).isValidPlayer())
				throw LuaApiException("Component: invalid player " + std::to_string(subType));
			result.subType = PlayerColor(subType);
			break;
		case ComponentType::BUILDING:
		{
			if(!faction)
				throw LuaApiException("Component: building requires a faction");
			const auto * town = FactionID(*faction).toFaction()->town.get();
			if(!town || !town->buildings.count(BuildingID(subType)))
				throw LuaApiException("Component: invalid building " + std::to_string(subType) + " of faction " + std::to_string(*faction));
			result.subType = BuildingTypeUniqueID(FactionID(*faction), BuildingID(subType));
			break;
		}
		case ComponentType::MANA:
		case ComponentType::EXPERIENCE:
		case ComponentType::LEVEL:
		case ComponentType::MORALE:
		case ComponentType::LUCK:
			break;
		default:
			throw LuaApiException("Component: invalid type " + std::to_string(type));
	}

	return result;
}

}
