/*
 * SpellCreatedObstacle.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "Obstacle.h"
#include "ObstacleJson.h"
class ObstacleChanges;
class JsonSerializeFormat;

class DLL_LINKAGE SpellCreatedObstacle : public Obstacle
{
public:
	SpellCreatedObstacle();
	SpellCreatedObstacle(ObstacleJson info, int16_t position = 0);

	ObstacleType getType() const override;
	bool visibleForSide(ui8 side, bool hasNativeStack) const override;
	void battleTurnPassed() override;

	bool blocksTiles() const override;
	bool stopsMovement() const override;
	bool triggersEffects() const override;

	void toInfo(ObstacleChanges & info);
	void fromInfo(const ObstacleChanges & info);

	void serializeJson(JsonSerializeFormat & handler);

	int32_t turnsRemaining;
	int32_t casterSpellPower;
	int8_t spellLevel;
	int8_t casterSide;
	bool visibleForAnotherSide = true;

	bool hidden;
	bool passable;
	bool trigger;
	bool trap;
	bool removeOnTrigger;

	bool revealed;
	SpellID spellID;
	
	

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<Obstacle&>(*this);
		h & turnsRemaining;
		h & casterSpellPower;
		h & spellLevel;
		h & casterSide;
		h & visibleForAnotherSide;

		h & hidden;
		h & passable;
		h & trigger;
		h & trap;
		h & spellID;
	}
};

