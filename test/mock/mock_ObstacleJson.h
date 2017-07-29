/*
 * mock_ObstacleJson.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "StdInc.h"
#include "lib/battle/obstacle/ObstacleJson.h"

class ObstacleJsonMock : public ObstacleJson
{
public:
	MOCK_CONST_METHOD0(getArea, ObstacleArea());
	MOCK_CONST_METHOD0(getSurface, ObstacleSurface());
	MOCK_CONST_METHOD0(getOffsetGraphicsInY, int32_t());
	MOCK_CONST_METHOD0(getOffsetGraphicsInX, int32_t());
	MOCK_CONST_METHOD0(canBeRemovedBySpell, bool());
	MOCK_CONST_METHOD0(getPosition, int16_t());
	MOCK_CONST_METHOD0(getPlace, std::vector<std::string>());

	MOCK_CONST_METHOD0(getDamage, int32_t());
	MOCK_CONST_METHOD0(getSpellLevel, int8_t());
	MOCK_CONST_METHOD0(getSpellPower, int32_t());
	MOCK_CONST_METHOD0(getBattleSide, int8_t());
	MOCK_CONST_METHOD0(isVisibleForAnotherSide, int8_t());
	MOCK_CONST_METHOD0(getTurnsRemaining, int32_t());

	MOCK_CONST_METHOD0(getType, ObstacleType());
	MOCK_CONST_METHOD0(getGraphicsName, std::string ());
	MOCK_CONST_METHOD0(randomPosition, bool());
	
};
