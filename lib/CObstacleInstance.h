#pragma once

struct DLL_EXPORT CObstacleInstance
{
	int uniqueID;
	int ID; //ID of obstacle (defines type of it)
	int pos; //position on battlefield
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & pos & uniqueID;
	}
};