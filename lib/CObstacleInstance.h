#pragma once
#include "BattleHex.h"

struct CObstacleInfo;

struct DLL_LINKAGE CObstacleInstance
{
	si32 uniqueID;
	si32 ID; //ID of obstacle (defines type of it)
	BattleHex pos; //position on battlefield

	ui8 isAbsoluteObstacle; //if true, then position is meaningless

	CObstacleInstance();

	const CObstacleInfo &getInfo() const;
	std::vector<BattleHex> getBlocked() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & pos & isAbsoluteObstacle & uniqueID;
	}
};