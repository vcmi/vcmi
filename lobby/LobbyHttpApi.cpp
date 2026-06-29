/*
 * LobbyHttpApi.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LobbyHttpApi.h"
#include "LobbyDatabase.h"

#include "../lib/json/JsonNode.h"
#include "../lib/logging/CLogger.h"

LobbyHttpApi::LobbyHttpApi(LobbyDatabase & database)
	: database(database)
	, startTime(std::chrono::system_clock::now())
{
}

bool LobbyHttpApi::isCacheValid(const CacheEntry & entry) const
{
	return std::chrono::system_clock::now() - entry.timestamp < std::chrono::seconds(CACHE_TTL_SECONDS);
}

std::string LobbyHttpApi::formatTimestamp(std::chrono::system_clock::time_point timePoint)
{
	auto tt = std::chrono::system_clock::to_time_t(timePoint);
	std::tm tm{};
	localtime_r(&tt, &tm);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S%z");
	return oss.str();
}

std::string LobbyHttpApi::getApiStats()
{
	if (statsCache && isCacheValid(*statsCache))
		return statsCache->json;
	JsonNode stats;
	stats["onlinePlayers"].Vector() = JsonVector();
	for (const auto & player : database.getActiveAccounts())
		stats["onlinePlayers"].Vector().push_back(JsonNode(player.displayName));
	auto activeCounts = database.getActiveAccountsCounts();
	stats["onlinePlayersCount"].Struct() = JsonMap{
		{"current", JsonNode(static_cast<int64_t>(stats["onlinePlayers"].Vector().size()))},
		{"lastHour", JsonNode(static_cast<int64_t>(activeCounts.h1))},
		{"lastDay", JsonNode(static_cast<int64_t>(activeCounts.h24))},
		{"lastWeek", JsonNode(static_cast<int64_t>(activeCounts.h168))},
		{"lastMonth", JsonNode(static_cast<int64_t>(activeCounts.h720))},
		{"lastYear", JsonNode(static_cast<int64_t>(activeCounts.h8760))}
	};
	auto registeredCounts = database.getRegisteredAccountsCounts();
	stats["registeredPlayersCount"].Struct() = JsonMap{
		{"total", JsonNode(static_cast<int64_t>(registeredCounts.total))},
		{"lastDay", JsonNode(static_cast<int64_t>(registeredCounts.h24))},
		{"lastWeek", JsonNode(static_cast<int64_t>(registeredCounts.h168))},
		{"lastMonth", JsonNode(static_cast<int64_t>(registeredCounts.h720))},
		{"lastYear", JsonNode(static_cast<int64_t>(registeredCounts.h8760))}
	};
	std::map<LobbyRoomState, int> lobbysCount;
	for (const auto & room : database.getActiveGameRooms())
		lobbysCount[room.roomState]++;
	auto closedCounts = database.getClosedGameRoomsCounts();
	stats["gameCount"].Struct() = JsonMap{
		{"current", JsonNode(lobbysCount[LobbyRoomState::BUSY])},
		{"total", JsonNode(static_cast<int64_t>(closedCounts.total))},
		{"lastDay", JsonNode(static_cast<int64_t>(closedCounts.h24))},
		{"lastWeek", JsonNode(static_cast<int64_t>(closedCounts.h168))},
		{"lastMonth", JsonNode(static_cast<int64_t>(closedCounts.h720))},
		{"lastYear", JsonNode(static_cast<int64_t>(closedCounts.h8760))}
	};
	stats["lobbyCount"].Struct() = JsonMap{
		{"current", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PUBLIC] + lobbysCount[LobbyRoomState::PRIVATE]))},
		{"public", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PUBLIC]))},
		{"private", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PRIVATE]))}
	};
	stats["lobbyStartTime"].String() = formatTimestamp(startTime);

	stats["server"].String() = "VCMI Lobby";
	stats["apiVersion"].String() = "1.0";

	std::string json = stats.toCompactString();
	statsCache = CacheEntry{json, std::chrono::system_clock::now()};
	return json;
}

std::string LobbyHttpApi::getApiChats(const std::string & channelName)
{
	auto it = chatsCache.find(channelName);
	if (it != chatsCache.end() && isCacheValid(it->second))
		return it->second.json;

	JsonNode chats;
	chats["messages"].Vector() = JsonVector();
	chats["channelName"].String() = channelName;

	auto messages = database.getRecentMessageHistory("global", channelName);

	for (const auto & msg : messages)
	{
		JsonNode message;
		message["displayName"].String() = msg.displayName;
		message["messageText"].String() = msg.messageText;
		message["ageSeconds"].Integer() = msg.age.count();

		auto messageTime = std::chrono::system_clock::now() - msg.age;
		message["timestamp"].String() = formatTimestamp(messageTime);

		chats["messages"].Vector().push_back(message);
	}

	chats["count"].Integer() = chats["messages"].Vector().size();

	std::string json = chats.toCompactString();
	chatsCache[channelName] = CacheEntry{json, std::chrono::system_clock::now()};
	return json;
}

std::string LobbyHttpApi::serializeRooms(const std::vector<LobbyGameRoom> & rooms, int hours, int limit)
{
	JsonNode result;
	result["rooms"].Vector() = JsonVector();
	result["hours"].Integer() = hours;
	result["limit"].Integer() = limit;

	int count = 0;
	for (const auto & room : rooms)
	{
		if (hours != -1 && room.age > std::chrono::hours(hours))
			continue;
		if (count >= limit)
			break;

		JsonNode roomNode = room.toJsonShort();
		roomNode["createdAt"].String() = formatTimestamp(std::chrono::system_clock::now() - room.age);
		result["rooms"].Vector().push_back(roomNode);
		++count;
	}

	result["count"].Integer() = result["rooms"].Vector().size();
	return result.toCompactString();
}

std::string LobbyHttpApi::getApiRooms(int hours, int limit)
{
	if (roomsCache)
	{
		const auto & c = *roomsCache;
		const bool ttlOk    = std::chrono::system_clock::now() - c.timestamp < std::chrono::seconds(CACHE_TTL_SECONDS);
		const bool hoursOk  = c.fetchedHours == -1 || (hours != -1 && c.fetchedHours >= hours);
		const bool limitOk  = c.fetchedLimit >= limit;
		if (ttlOk && hoursOk && limitOk)
			return serializeRooms(c.rooms, hours, limit);
	}

	auto fetchedRooms = database.getRooms(hours, limit);

	roomsCache = RoomsCacheEntry{fetchedRooms, hours, limit, std::chrono::system_clock::now()};

	return serializeRooms(fetchedRooms, hours, limit);
}
