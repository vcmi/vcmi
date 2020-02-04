/*
 * SpellCreatedObstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SpellCreatedObstacle.h"
#include "CTownHandler.h"
#include "VCMI_Lib.h"
#include "spells/CSpellHandler.h"
#include "filesystem/ResourceID.h"
#include "battle/obstacle/ObstacleJson.h"
#include "UUID.h"
#include "NetPacks.h"
#include "../../serializer/JsonDeserializer.h"
#include "../../serializer/JsonSerializer.h"

SpellCreatedObstacle::SpellCreatedObstacle()
	: turnsRemaining(-1),
	casterSpellPower(0),
	spellLevel(0),
	casterSide(0),
	hidden(false),
	passable(false),
	trigger(false),
	trap(false),
	removeOnTrigger(false),
	revealed(false)
{
}

SpellCreatedObstacle::SpellCreatedObstacle(ObstacleJson info, int16_t position)
{
	if(!info.randomPosition())
		position = info.getPosition();
	setArea(ObstacleArea(info.getArea(), position));
	spellLevel = info.getSpellLevel();
	turnsRemaining = info.getTurnsRemaining();
	casterSpellPower = info.getSpellPower();
	visibleForAnotherSide = info.isVisibleForAnotherSide();
	casterSide = info.getBattleSide();
	setGraphicsInfo(info.getGraphicsInfo());
}

bool SpellCreatedObstacle::visibleForSide(ui8 side, bool hasNativeStack) const
{
	//we hide mines and not discovered quicksands
	//quicksands are visible to the caster or if owned unit stepped into that particular patch
	//additionally if side has a native unit, mines/quicksands will be visible

	//TODO contidion for revealed;
	return static_cast<ui8>(casterSide) == side || !hidden || hasNativeStack;
}

ObstacleType SpellCreatedObstacle::getType() const
{
	return ObstacleType::SPELL_CREATED;
}

void SpellCreatedObstacle::battleTurnPassed()
{
	if(turnsRemaining > 0)
		turnsRemaining--;
}

bool SpellCreatedObstacle::blocksTiles() const
{
	return !passable;
}

bool SpellCreatedObstacle::stopsMovement() const
{
	return trap;
}

bool SpellCreatedObstacle::triggersEffects() const
{
	return trigger;
}


void SpellCreatedObstacle::toInfo(ObstacleChanges & info)
{
	info.id = Obstacle::ID;
	info.spellID = spellID;
	info.operation = ObstacleChanges::EOperation::ADD;

	info.data.clear();
	JsonSerializer ser(nullptr, info.data);
	ser.serializeStruct("obstacle", *this);
}

void SpellCreatedObstacle::fromInfo(const ObstacleChanges & info)
{
	Obstacle::ID = info.id;
	spellID = info.spellID;
	if(info.operation != ObstacleChanges::EOperation::ADD)
		logGlobal->error("ADD operation expected");

	JsonDeserializer deser(nullptr, info.data);
	deser.serializeStruct("obstacle", *this);
}

void SpellCreatedObstacle::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("turnsRemaining", turnsRemaining);
	handler.serializeInt("casterSpellPower", casterSpellPower);
	handler.serializeInt("spellLevel", spellLevel);
	handler.serializeInt("casterSide", casterSide);

	handler.serializeBool("hidden", hidden);
	handler.serializeBool("passable", passable);
	handler.serializeBool("trigger", trigger);
	handler.serializeBool("trap", trap);
	
	handler.serializeBool("removeOnTrigger", removeOnTrigger);
	
	handler.serializeStruct("graphics", graphicsInfo);
	handler.serializeStruct("area", area);
}
