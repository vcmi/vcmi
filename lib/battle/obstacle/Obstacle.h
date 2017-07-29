/*
 * Obstacle.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "ObstacleType.h"
#include "ObstacleArea.h"
#include "ObstacleGraphicsInfo.h"
#include "../../UUID.h"

class DLL_LINKAGE Obstacle
{
public:
	virtual ~Obstacle() {}
	virtual ObstacleType getType() const = 0;
	virtual bool visibleForSide(ui8 side, bool hasNativeStack) const = 0;
	virtual void battleTurnPassed() = 0;

	virtual bool blocksTiles() const = 0;
	virtual bool stopsMovement() const = 0;
	virtual bool triggersEffects() const = 0;

	UUID ID;
	ObstacleArea getArea() const;
	void setArea(ObstacleArea zone);

	ObstacleGraphicsInfo getGraphicsInfo() const;
	void setGraphicsInfo(ObstacleGraphicsInfo info);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		if(version < 777 & !h.saving)
		{
			si32 id;
			BattleHex pos;
			ui8 obstacleType;
			si32 uniqueID;
			h & id;
			h & uniqueID;
			h & obstacleType;
			h & pos;
		}
		h & ID;
		h & area;
		h & graphicsInfo;
	}

protected:
	ObstacleArea area;
	ObstacleGraphicsInfo graphicsInfo;
};
