/*
 * LobbyHttpApi.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "HttpServer.h"
#include "LobbyDefines.h"

class LobbyDatabase;

/// Concrete implementation of the VCMI lobby REST API.
/// Fetches data from LobbyDatabase, applies caching, and serializes to JSON.
class LobbyHttpApi : public ILobbyHttpHandler
{
public:
	static constexpr int CACHE_TTL_SECONDS = 30;

	explicit LobbyHttpApi(LobbyDatabase & database);

	std::string getApiStats() override;
	std::string getApiChats(const std::string & channelName) override;
	std::string getApiRooms(int hours, int limit) override;

private:
	struct CacheEntry
	{
		std::string json;
		std::chrono::system_clock::time_point timestamp;
	};

	bool isCacheValid(const CacheEntry & entry) const;
	std::string formatTimestamp(std::chrono::system_clock::time_point timePoint);
	std::string serializeRooms(const std::vector<LobbyGameRoom> & rooms, int hours, int limit);

	LobbyDatabase & database;
	std::chrono::system_clock::time_point startTime;

	std::optional<CacheEntry> statsCache;
	std::map<std::string, CacheEntry> chatsCache;

	struct RoomsCacheEntry
	{
		std::vector<LobbyGameRoom> rooms;
		int fetchedHours;   // -1 means all time
		int fetchedLimit;
		std::chrono::system_clock::time_point timestamp;
	};
	std::optional<RoomsCacheEntry> roomsCache;
};
