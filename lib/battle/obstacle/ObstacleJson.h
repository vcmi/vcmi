/*
 * ObstacleJson.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "ObstacleArea.h"
#include "ObstacleSurface.h"
#include "ObstacleType.h"
#include "ObstacleGraphicsInfo.h"
#include "../../JsonNode.h"


class DLL_LINKAGE ObstacleJson
{
public:
	virtual ObstacleArea getArea() const;
	virtual ObstacleSurface getSurface() const;
	virtual ObstacleGraphicsInfo getGraphicsInfo() const;
	virtual bool canBeRemovedBySpell() const;
	virtual int16_t getPosition() const;
	virtual std::vector<std::string> getPlace() const;
	virtual int32_t getDamage() const;

	virtual int8_t getSpellLevel() const;
	virtual int32_t getSpellPower() const;
	virtual int8_t getBattleSide() const;
	virtual int8_t isVisibleForAnotherSide() const;
	virtual int32_t getTurnsRemaining() const;

	virtual ObstacleType getType() const;
	virtual bool randomPosition() const;
	virtual bool isInherent();
	ObstacleJson(JsonNode jsonNode);
	ObstacleJson();
protected:
	JsonNode config;
private:
	ObstacleType typeConvertFromString(const std::string type) const;
};
