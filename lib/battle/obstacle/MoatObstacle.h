/*
 * MoatObstacle.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "StaticObstacle.h"

class DLL_LINKAGE MoatObstacle : public StaticObstacle
{
public:
	MoatObstacle();
	MoatObstacle(ObstacleJson info, int16_t position = 0);

	int32_t getDamage() const;
	void setDamage(int32_t value);
	ObstacleType getType() const override;

	bool blocksTiles() const override;
	bool stopsMovement() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<StaticObstacle&>(*this);
		h & damage;
	}
private:
	int32_t damage = 0;
};
