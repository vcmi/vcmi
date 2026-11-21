/*
 * MonsterJoiningOptions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MonsterJoiningOptions.h"

#include "../json/JsonNode.h"
#include "../serializer/JsonSerializeFormat.h"

namespace rmg
{

void MonsterJoiningOptions::serializeJson(JsonSerializeFormat & handler)
{
	if(handler.saving)
	{
		if(character)
		{
			auto value = *character;
			handler.serializeEnum("character", value, CGCreature::CHARACTER_JSON);
		}
	}
	else if(handler.getCurrent()["character"].getType() == JsonNode::JsonType::DATA_STRING)
	{
		CGCreature::Character value = CGCreature::Character::HOSTILE;
		handler.serializeEnum("character", value, CGCreature::CHARACTER_JSON);
		character = value;
	}

	handler.serializeInt("aggression", aggression);
	handler.serializeInt("joiningPercentage", joiningPercentage);

	if(handler.saving)
	{
		if(joinOnlyForMoney)
		{
			JsonNode flag(*joinOnlyForMoney);
			handler.serializeRaw("joinOnlyForMoney", flag, std::nullopt);
		}
	}
	else
	{
		const JsonNode & flag = handler.getCurrent()["joinOnlyForMoney"];
		if(flag.getType() == JsonNode::JsonType::DATA_BOOL)
			joinOnlyForMoney = flag.Bool();
	}
}

MonsterJoiningOptions MonsterJoiningOptions::merge(const MonsterJoiningOptions & base, const MonsterJoiningOptions & override)
{
	MonsterJoiningOptions result = base;
	if(override.character)
		result.character = override.character;
	if(override.aggression)
		result.aggression = override.aggression;
	if(override.joiningPercentage)
		result.joiningPercentage = override.joiningPercentage;
	if(override.joinOnlyForMoney)
		result.joinOnlyForMoney = override.joinOnlyForMoney;
	return result;
}

void MonsterJoiningOptions::applyTo(CGCreature & guard) const
{
	guard.initialCharacter = character.value_or(CGCreature::Character::HOSTILE);
	if(guard.initialCharacter == CGCreature::Character::CUSTOM && aggression)
		guard.agression = *aggression;
	if(joiningPercentage)
		guard.joiningPercentage = *joiningPercentage;
	if(joinOnlyForMoney)
		guard.joinOnlyForMoney = *joinOnlyForMoney;
}

}
