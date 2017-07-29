/*
 * ObstacleJson.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ObstacleJson.h"

ObstacleType ObstacleJson::typeConvertFromString(const std::string type) const
{
	if(type == "static")
		return ObstacleType::STATIC;
	else if(type == "moat")
		return ObstacleType::MOAT;
	else if(type == "spell")
		return ObstacleType::SPELL_CREATED;
}

ObstacleArea ObstacleJson::getArea() const
{
	ObstacleArea area;
	area.setArea(config["blockedTiles"].convertTo<std::vector<BattleHex>>());
	return area;
}

ObstacleGraphicsInfo ObstacleJson::getGraphicsInfo() const
{
	ObstacleGraphicsInfo info;
	info.setGraphicsName(config["graphics"].String());
	info.setAppearAnimation(config["appearAnimation"].String());
	info.setOffsetGraphicsInX(config["offsetGraphicsInX"].Integer());
	info.setOffsetGraphicsInY(config["offsetGraphicsInY"].Integer());
	return info;
}

ObstacleSurface ObstacleJson::getSurface() const
{
	ObstacleSurface surface;
	auto battlefieldTypes = config["battlefieldSurface"].convertTo<std::vector<std::string>>();
	for(auto type : battlefieldTypes)
		surface.battlefieldSurface.push_back(BattlefieldType::fromString(type));
	return surface;
}

bool ObstacleJson::canBeRemovedBySpell() const
{
	return config["canBeRemoved"].Bool();
}

int16_t ObstacleJson::getPosition() const
{
	return config["position"].Integer();
}

std::vector<std::string> ObstacleJson::getPlace() const
{
	return config["place"].convertTo<std::vector<std::string>>();
}

int32_t ObstacleJson::getDamage() const
{
	return config["damage"].Integer();
}

int8_t ObstacleJson::getSpellLevel() const
{
	if(config["spellLevel"].isNull())
		return 1;
	return config["spellLevel"].Integer();
}

int32_t ObstacleJson::getSpellPower() const
{
	if(config["spellPower"].isNull())
		return 1;
	return config["spellPower"].Integer();
}

int8_t ObstacleJson::getBattleSide() const
{
	if(config["battleSide"].isNull())
		return 0;
	return config["battleSide"].Integer();
}

int8_t ObstacleJson::isVisibleForAnotherSide() const
{
	if(config["visibleForAnotherSide"].isNull())
		return 1;
	return config["visibleForAnotherSide"].Integer();
}

int32_t ObstacleJson::getTurnsRemaining() const
{
	if(config["turnsRemaining"].isNull())
		return -1;
	return config["turnsRemaining"].Integer();
}

ObstacleType ObstacleJson::getType() const
{
	return typeConvertFromString(config["type"].String());
}

bool ObstacleJson::randomPosition() const
{
	return config["position"].isNull();
}

bool ObstacleJson::isInherent()
{
	return config["inherent"].Bool();
}

ObstacleJson::ObstacleJson(JsonNode jsonNode) : config(jsonNode)
{

}

ObstacleJson::ObstacleJson()
{

}
