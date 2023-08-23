/*
 * CObstacleInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CObstacleInstance.h"
#include "../CHeroHandler.h"
#include "../CTownHandler.h"
#include "../ObstacleHandler.h"
#include "../VCMI_Lib.h"
#include "../NetPacksBase.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

const ObstacleInfo & CObstacleInstance::getInfo() const
{
	assert( obstacleType == USUAL || obstacleType == ABSOLUTE_OBSTACLE);

	return *Obstacle(ID).getInfo();
}

std::vector<BattleHex> CObstacleInstance::getBlockedTiles() const
{
	if(blocksTiles())
		return getAffectedTiles();
	return std::vector<BattleHex>();
}

std::vector<BattleHex> CObstacleInstance::getStoppingTile() const
{
	if(stopsMovement())
		return getAffectedTiles();
	return std::vector<BattleHex>();
}

std::vector<BattleHex> CObstacleInstance::getAffectedTiles() const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
	case USUAL:
		return getInfo().getBlocked(pos);
	default:
		assert(0);
		return std::vector<BattleHex>();
	}
}

bool CObstacleInstance::visibleForSide(ui8 side, bool hasNativeStack) const
{
	//by default obstacle is visible for everyone
	return true;
}

const AnimationPath & CObstacleInstance::getAnimation() const
{
	return getInfo().animation;
}

const AnimationPath & CObstacleInstance::getAppearAnimation() const
{
	return getInfo().appearAnimation;
}

const std::string & CObstacleInstance::getAppearSound() const
{
	return getInfo().appearSound;
}

int CObstacleInstance::getAnimationYOffset(int imageHeight) const
{
	int offset = imageHeight % 42;
	if(obstacleType == CObstacleInstance::USUAL)
	{
		if(getInfo().blockedTiles.front() < 0 || offset > 37) //second or part is for holy ground ID=62,65,63
			offset -= 42;
	}
	return offset;
}

bool CObstacleInstance::stopsMovement() const
{
	return obstacleType == MOAT;
}

bool CObstacleInstance::blocksTiles() const
{
	return obstacleType == USUAL || obstacleType == ABSOLUTE_OBSTACLE ;
}

bool CObstacleInstance::triggersEffects() const
{
	return getTrigger() != SpellID::NONE;
}

SpellID CObstacleInstance::getTrigger() const
{
	return SpellID::NONE;
}

void CObstacleInstance::serializeJson(JsonSerializeFormat & handler)
{
	auto obstacleInfo = getInfo();
	auto hidden = false;
	auto needAnimationOffsetFix = obstacleType == CObstacleInstance::USUAL;
	int animationYOffset = 0;
		
	if(getInfo().blockedTiles.front() < 0) //TODO: holy ground ID=62,65,63
		animationYOffset -= 42;

	//We need only a subset of obstacle info for correct render
	handler.serializeInt("position", pos);
	handler.serializeString("appearSound", obstacleInfo.appearSound);
	handler.serializeStruct("appearAnimation", obstacleInfo.appearAnimation);
	handler.serializeStruct("animation", obstacleInfo.animation);
	handler.serializeInt("animationYOffset", animationYOffset);

	handler.serializeBool("hidden", hidden);
	handler.serializeBool("needAnimationOffsetFix", needAnimationOffsetFix);
}

void CObstacleInstance::toInfo(ObstacleChanges & info, BattleChanges::EOperation operation)
{
	info.id = uniqueID;
	info.operation = operation;

	info.data.clear();
	JsonSerializer ser(nullptr, info.data);
	ser.serializeStruct("obstacle", *this);
}

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
	revealed(false),
	animationYOffset(0),
	nativeVisible(true),
	minimalDamage(0)
{
	obstacleType = SPELL_CREATED;
}

bool SpellCreatedObstacle::visibleForSide(ui8 side, bool hasNativeStack) const
{
	//we hide mines and not discovered quicksands
	//quicksands are visible to the caster or if owned unit stepped into that particular patch
	//additionally if side has a native unit, mines/quicksands will be visible
	//but it is not a case for a moat, so, hasNativeStack should not work for moats

	auto nativeVis = hasNativeStack && nativeVisible;

	return casterSide == side || !hidden || revealed || nativeVis;
}

bool SpellCreatedObstacle::blocksTiles() const
{
	return !passable;
}

bool SpellCreatedObstacle::stopsMovement() const
{
	return trap;
}

SpellID SpellCreatedObstacle::getTrigger() const
{
	return trigger;
}

void SpellCreatedObstacle::fromInfo(const ObstacleChanges & info)
{
	uniqueID = info.id;

	if(info.operation != ObstacleChanges::EOperation::ADD && info.operation != ObstacleChanges::EOperation::UPDATE)
		logGlobal->error("ADD or UPDATE operation expected");

    JsonDeserializer deser(nullptr, info.data);
    deser.serializeStruct("obstacle", *this);
}

void SpellCreatedObstacle::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("spell", ID);
	handler.serializeInt("position", pos);

	handler.serializeInt("turnsRemaining", turnsRemaining);
	handler.serializeInt("casterSpellPower", casterSpellPower);
	handler.serializeInt("spellLevel", spellLevel);
	handler.serializeInt("casterSide", casterSide);
	handler.serializeInt("minimalDamage", minimalDamage);
	handler.serializeInt("type", obstacleType);

	handler.serializeBool("hidden", hidden);
	handler.serializeBool("revealed", revealed);
	handler.serializeBool("passable", passable);
	handler.serializeId("trigger", trigger, SpellID::NONE);
	handler.serializeBool("trap", trap);
	handler.serializeBool("removeOnTrigger", removeOnTrigger);
	handler.serializeBool("nativeVisible", nativeVisible);

	handler.serializeString("appearSound", appearSound);
	handler.serializeStruct("appearAnimation", appearAnimation);
	handler.serializeStruct("animation", animation);

	handler.serializeInt("animationYOffset", animationYOffset);

	{
		JsonArraySerializer customSizeJson = handler.enterArray("customSize");
		customSizeJson.syncSize(customSize, JsonNode::JsonType::DATA_INTEGER);

		for(size_t index = 0; index < customSizeJson.size(); index++)
			customSizeJson.serializeInt(index, customSize.at(index));
	}
}

std::vector<BattleHex> SpellCreatedObstacle::getAffectedTiles() const
{
	return customSize;
}

void SpellCreatedObstacle::battleTurnPassed()
{
	if(turnsRemaining > 0)
		turnsRemaining--;
}

const AnimationPath & SpellCreatedObstacle::getAnimation() const
{
	return animation;
}

const AnimationPath & SpellCreatedObstacle::getAppearAnimation() const
{
	return appearAnimation;
}

const std::string & SpellCreatedObstacle::getAppearSound() const
{
	return appearSound;
}

int SpellCreatedObstacle::getAnimationYOffset(int imageHeight) const
{
	int offset = imageHeight % 42;

	if(obstacleType == CObstacleInstance::SPELL_CREATED || obstacleType == CObstacleInstance::MOAT)
	{
		offset += animationYOffset;
	}

	return offset;
}

VCMI_LIB_NAMESPACE_END
