/*
 * HttpApiServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HttpApiServer.h"
#include "LobbyServer.h"
#include "LobbyDatabase.h"
#include "EmbeddedWebAssets.h"

#include "../lib/json/JsonNode.h"
#include "../lib/logging/CLogger.h"
#include "../lib/GameConstants.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

HttpApiServer::HttpApiServer(LobbyServer & lobbyServer, unsigned short port)
	: lobbyServer(lobbyServer)
	, port(port)
	, running(false)
{
}

HttpApiServer::~HttpApiServer()
{
	stop();
}

void HttpApiServer::start()
{
	running = true;
	thread = std::make_unique<std::thread>([this]() { run(); });
	logGlobal->info("HTTP API Server started on port %d", port);
    startTime = std::chrono::system_clock::now();
}

void HttpApiServer::stop()
{
	if (running)
	{
		running = false;
		ioc.stop();
		if (thread && thread->joinable())
			thread->join();
		logGlobal->info("HTTP API Server stopped");
	}
}

void HttpApiServer::run()
{
	try
	{
		tcp::acceptor acceptor{ioc, {tcp::v4(), port}};

		while (running)
		{
			tcp::socket socket{ioc};
			acceptor.accept(socket);

			beast::tcp_stream stream(std::move(socket));
			beast::flat_buffer buffer;

			http::request<http::string_body> req;
			http::read(stream, buffer, req);

			handleRequest(std::move(req), stream);
		}
	}
	catch (const std::exception & e)
	{
		logGlobal->error("HTTP API Server error: %s", e.what());
	}
}

void HttpApiServer::handleRequest(http::request<http::string_body> && req, beast::tcp_stream & stream)
{
	auto const createResponse = [&req](http::status status, const std::string & body, const std::string & contentType = "application/json")
	{
		http::response<http::string_body> res{status, req.version()};
		res.set(http::field::server, "VCMI-Lobby-API");
		res.set(http::field::content_type, contentType);
		res.keep_alive(req.keep_alive());
		res.body() = body;
		res.prepare_payload();
		return res;
	};

	try
	{
		if (req.target() == "/api/v1/stats")
		{
			JsonNode stats = getStats();
			std::string json = stats.toCompactString();
			auto res = createResponse(http::status::ok, json);
			http::write(stream, res);
		}
		else if (req.target().starts_with("/api/v1/chats"))
		{
			// Parse query parameters
			std::string channelName = "english";
			auto target = std::string(req.target());
			auto queryPos = target.find('?');
			if (queryPos != std::string::npos)
			{
				auto query = target.substr(queryPos + 1);
				auto channelPos = query.find("channelName=");
				if (channelPos != std::string::npos)
				{
					auto valueStart = channelPos + 12; // length of "channelName="
					auto valueEnd = query.find('&', valueStart);
					channelName = query.substr(valueStart, valueEnd == std::string::npos ? std::string::npos : valueEnd - valueStart);
				}
			}
			
			JsonNode chats = getChats(channelName);
			std::string json = chats.toCompactString();
			auto res = createResponse(http::status::ok, json);
			http::write(stream, res);
		}
		else if (req.target().starts_with("/api/v1/rooms"))
		{
			// Parse query parameters
			int hours = -1;
			int limit = 50;
			auto target = std::string(req.target());
			auto queryPos = target.find('?');
			if (queryPos != std::string::npos)
			{
				auto query = target.substr(queryPos + 1);
				auto hoursPos = query.find("hours=");
				if (hoursPos != std::string::npos)
				{
					auto valueStart = hoursPos + 6; // length of "hours="
					auto valueEnd = query.find('&', valueStart);
					std::string hoursStr = query.substr(valueStart, valueEnd == std::string::npos ? std::string::npos : valueEnd - valueStart);
					try {
						hours = std::stoi(hoursStr);
					} catch(...) {
						hours = -1;
					}
				}
				auto limitPos = query.find("limit=");
				if (limitPos != std::string::npos)
				{
					auto valueStart = limitPos + 6; // length of "limit="
					auto valueEnd = query.find('&', valueStart);
					std::string limitStr = query.substr(valueStart, valueEnd == std::string::npos ? std::string::npos : valueEnd - valueStart);
					try {
						limit = std::stoi(limitStr);
						if (limit > 250) limit = 250;
						if (limit < 1) limit = 50;
					} catch(...) {
						limit = 50;
					}
				}
			}
			
			JsonNode rooms = getRooms(hours, limit);
			std::string json = rooms.toCompactString();
			auto res = createResponse(http::status::ok, json);
			http::write(stream, res);
		}
		else if (req.target() == "/api/docs" || req.target() == "/")
		{
			std::string html = EmbeddedFiles::SWAGGER_CONTENT;
			auto res = createResponse(http::status::ok, html, "text/html");
			http::write(stream, res);
		}
		else if (req.target() == "/api/openapi.yaml")
		{
			std::string spec = EmbeddedFiles::OPENAPI_CONTENT;
			auto res = createResponse(http::status::ok, spec, "text/yaml");
			http::write(stream, res);
		}
		else
		{
			// 404 Not Found
			std::string json = R"({ "error": "Not Found", "message": "The requested endpoint does not exist" })";
			auto res = createResponse(http::status::not_found, json);
			http::write(stream, res);
		}

		// Graceful shutdown
		beast::error_code ec;
		stream.socket().shutdown(tcp::socket::shutdown_send, ec);
	}
	catch (const std::exception & e)
	{
		logGlobal->error("Error handling HTTP request: %s", e.what());
	}
}

JsonNode HttpApiServer::getStats()
{
	JsonNode stats;
	stats["onlinePlayers"].Vector() = JsonVector();
    for (const auto & player : lobbyServer.getDatabase()->getActiveAccounts())
        stats["onlinePlayers"].Vector().push_back(JsonNode(player.displayName));
    stats["onlinePlayersCount"].Struct() = JsonMap{
        {"current", JsonNode(static_cast<int64_t>(stats["onlinePlayers"].Vector().size()))},
        {"lastHour", JsonNode(lobbyServer.getDatabase()->getActiveAccountsCount(1))},
        {"lastDay", JsonNode(lobbyServer.getDatabase()->getActiveAccountsCount(24))},
        {"lastWeek", JsonNode(lobbyServer.getDatabase()->getActiveAccountsCount(168))},
        {"lastMonth", JsonNode(lobbyServer.getDatabase()->getActiveAccountsCount(720))},
        {"lastYear", JsonNode(lobbyServer.getDatabase()->getActiveAccountsCount(8760))}
    };
    stats["registeredPlayersCount"].Struct() = JsonMap{
        {"total", JsonNode(lobbyServer.getDatabase()->getAccountCount())},
        {"lastDay", JsonNode(lobbyServer.getDatabase()->getRegisteredAccountsCount(24))},
        {"lastWeek", JsonNode(lobbyServer.getDatabase()->getRegisteredAccountsCount(168))},
        {"lastMonth", JsonNode(lobbyServer.getDatabase()->getRegisteredAccountsCount(720))},
        {"lastYear", JsonNode(lobbyServer.getDatabase()->getRegisteredAccountsCount(8760))}
    };
    std::map<LobbyRoomState, int> lobbysCount;
    for (const auto & room : lobbyServer.getDatabase()->getActiveGameRooms())
        lobbysCount[room.roomState]++;
    stats["gameCount"].Struct() = JsonMap{
        {"current", JsonNode(lobbysCount[LobbyRoomState::BUSY])},
        {"total", JsonNode(lobbyServer.getDatabase()->getClosedGameRoomsCount())},
        {"lastDay", JsonNode(lobbyServer.getDatabase()->getClosedGameRoomsCount(24))},
        {"lastWeek", JsonNode(lobbyServer.getDatabase()->getClosedGameRoomsCount(168))},
        {"lastMonth", JsonNode(lobbyServer.getDatabase()->getClosedGameRoomsCount(720))},
        {"lastYear", JsonNode(lobbyServer.getDatabase()->getClosedGameRoomsCount(8760))}
    };
    stats["lobbyCount"].Struct() = JsonMap{
        {"current", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PUBLIC] + lobbysCount[LobbyRoomState::PRIVATE]))},
        {"public", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PUBLIC]))},
        {"private", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PRIVATE]))}
    };
    stats["registeredPlayersCount"].Integer() = lobbyServer.getDatabase()->getAccountCount();
	stats["lobbyStartTime"].String() = std::format("{:%Y-%m-%dT%H:%M:%S%z}", startTime);
	stats["server"].String() = "VCMI Lobby";
	stats["lobbyVersion"].String() = GameConstants::VCMI_VERSION;
	stats["apiVersion"].String() = "1.0";
	return stats;
}

JsonNode HttpApiServer::getChats(const std::string & channelName)
{
	JsonNode chats;
	chats["messages"].Vector() = JsonVector();
	chats["channelName"].String() = channelName;
	
	auto messages = lobbyServer.getDatabase()->getRecentMessageHistory("global", channelName);
	
	for (const auto & msg : messages)
	{
		JsonNode message;
		message["accountID"].String() = msg.accountID;
		message["displayName"].String() = msg.displayName;
		message["messageText"].String() = msg.messageText;
		message["ageSeconds"].Integer() = msg.age.count();
		chats["messages"].Vector().push_back(message);
	}
	
	chats["count"].Integer() = chats["messages"].Vector().size();
	
	return chats;
}

JsonNode HttpApiServer::getRooms(int hours, int limit)
{
	JsonNode result;
	result["rooms"].Vector() = JsonVector();
	result["hours"].Integer() = hours;
	result["limit"].Integer() = limit;
	
	auto rooms = lobbyServer.getDatabase()->getRooms(hours, limit);
	
	for (const auto & room : rooms)
	{
		JsonNode roomNode;
		roomNode["description"].String() = room.description;
		roomNode["status"].Integer() = static_cast<int>(room.roomState);
		roomNode["playerLimit"].Integer() = room.playerLimit;
		roomNode["version"].String() = room.version;
		roomNode["secondsElapsed"].Integer() = room.age.count();
		
		// Parse mods JSON string
		try {
			JsonNode modsNode(reinterpret_cast<const std::byte*>(room.modsJson.data()), room.modsJson.size(), "");
			roomNode["mods"] = modsNode;
		} catch(...) {
			roomNode["mods"].Struct() = JsonMap{};
		}
		
		result["rooms"].Vector().push_back(roomNode);
	}
	
	result["count"].Integer() = result["rooms"].Vector().size();
	
	return result;
}
