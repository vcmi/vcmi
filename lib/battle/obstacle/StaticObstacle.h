/*
 * StaticObstacle.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../BattleHex.h"
#include "ObstacleJson.h"
#include "Obstacle.h"

class DLL_LINKAGE StaticObstacle : public Obstacle
{
public:
	StaticObstacle();
	StaticObstacle(const ObstacleJson & info, int16_t position = 0);
	StaticObstacle(const ObstacleJson * info, int16_t position = 0);
	virtual ~StaticObstacle();

	ObstacleType getType() const override;
	bool visibleForSide(ui8 side, bool hasNativeStack) const override;
	void battleTurnPassed() override;

	bool blocksTiles() const override;
	bool stopsMovement() const override;
	bool triggersEffects() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<Obstacle&>(*this);
		h & canBeRemovedBySpell;
	}
protected:
	bool canBeRemovedBySpell = false;
};
