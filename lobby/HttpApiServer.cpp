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

HttpApiServer::HttpApiServer(const boost::filesystem::path & databasePath, unsigned short port)
	: database(std::make_unique<LobbyDatabase>(databasePath, false))
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
		tcp::acceptor acceptor{ioc};
		tcp::endpoint ep{tcp::v6(), port};
		acceptor.open(ep.protocol());
		acceptor.set_option(tcp::acceptor::reuse_address(true));
		acceptor.set_option(boost::asio::ip::v6_only(false));
		acceptor.bind(ep);
		acceptor.listen();

		while (running)
		{
			tcp::socket socket{ioc};
			acceptor.accept(socket);

			// Handle each connection asynchronously
			std::thread([this, sock = std::move(socket)]() mutable {
				try {
					beast::tcp_stream stream(std::move(sock));
					handleSession(std::move(stream));
				} catch (const std::exception & e) {
					logGlobal->error("HTTP session error: %s", e.what());
				}
			}).detach();
		}
	}
	catch (const std::exception & e)
	{
		logGlobal->error("HTTP API Server error: %s", e.what());
	}
}

void HttpApiServer::handleSession(beast::tcp_stream stream)
{
	beast::flat_buffer buffer;
	http::request<http::string_body> req;
	http::read(stream, buffer, req);
	handleRequest(std::move(req), stream);
}

void HttpApiServer::handleRequest(http::request<http::string_body> && req, beast::tcp_stream & stream)
{
	// Log the request
	std::string clientIP = "unknown";
	try {
		auto endpoint = stream.socket().remote_endpoint();
		clientIP = endpoint.address().to_string();
	} catch(...) {}
	
	std::string userAgent = std::string(req[http::field::user_agent]);
	if (userAgent.empty()) userAgent = "unknown";
	
	logGlobal->info("HTTP API Request: %s %s from %s (User-Agent: %s)", 
		req.method_string().data(), 
		req.target().data(), 
		clientIP.c_str(), 
		userAgent.c_str());

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

std::string HttpApiServer::formatTimestamp(std::chrono::system_clock::time_point timePoint)
{
	auto tt = std::chrono::system_clock::to_time_t(timePoint);
	std::tm tm{};
	localtime_r(&tt, &tm);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S%z");
	return oss.str();
}

JsonNode HttpApiServer::getStats()
{
	JsonNode stats;
	stats["onlinePlayers"].Vector() = JsonVector();
	for (const auto & player : database->getActiveAccounts())
		stats["onlinePlayers"].Vector().push_back(JsonNode(player.displayName));
	stats["onlinePlayersCount"].Struct() = JsonMap{
		{"current", JsonNode(static_cast<int64_t>(stats["onlinePlayers"].Vector().size()))},
		{"lastHour", JsonNode(database->getActiveAccountsCount(1))},
		{"lastDay", JsonNode(database->getActiveAccountsCount(24))},
		{"lastWeek", JsonNode(database->getActiveAccountsCount(168))},
		{"lastMonth", JsonNode(database->getActiveAccountsCount(720))},
		{"lastYear", JsonNode(database->getActiveAccountsCount(8760))}
	};
	stats["registeredPlayersCount"].Struct() = JsonMap{
		{"total", JsonNode(database->getAccountCount())},
		{"lastDay", JsonNode(database->getRegisteredAccountsCount(24))},
		{"lastWeek", JsonNode(database->getRegisteredAccountsCount(168))},
		{"lastMonth", JsonNode(database->getRegisteredAccountsCount(720))},
		{"lastYear", JsonNode(database->getRegisteredAccountsCount(8760))}
	};
	std::map<LobbyRoomState, int> lobbysCount;
	for (const auto & room : database->getActiveGameRooms())
		lobbysCount[room.roomState]++;
	stats["gameCount"].Struct() = JsonMap{
		{"current", JsonNode(lobbysCount[LobbyRoomState::BUSY])},
		{"total", JsonNode(database->getClosedGameRoomsCount())},
		{"lastDay", JsonNode(database->getClosedGameRoomsCount(24))},
		{"lastWeek", JsonNode(database->getClosedGameRoomsCount(168))},
		{"lastMonth", JsonNode(database->getClosedGameRoomsCount(720))},
		{"lastYear", JsonNode(database->getClosedGameRoomsCount(8760))}
	};
	stats["lobbyCount"].Struct() = JsonMap{
		{"current", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PUBLIC] + lobbysCount[LobbyRoomState::PRIVATE]))},
		{"public", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PUBLIC]))},
		{"private", JsonNode(static_cast<int64_t>(lobbysCount[LobbyRoomState::PRIVATE]))}
	};
	stats["registeredPlayersCount"].Integer() = database->getAccountCount();

	stats["lobbyStartTime"].String() = formatTimestamp(startTime);
	
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
	
	auto messages = database->getRecentMessageHistory("global", channelName);
	
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
	
	return chats;
}

JsonNode HttpApiServer::getRooms(int hours, int limit)
{
	JsonNode result;
	result["rooms"].Vector() = JsonVector();
	result["hours"].Integer() = hours;
	result["limit"].Integer() = limit;
	
	auto rooms = database->getRooms(hours, limit);
	
	for (const auto & room : rooms)
	{
		JsonNode roomNode;
		roomNode["description"].String() = room.description;
		roomNode["status"].Integer() = static_cast<int>(room.roomState);
		roomNode["playerLimit"].Integer() = room.playerLimit;
		roomNode["version"].String() = room.version;
		roomNode["secondsElapsed"].Integer() = room.age.count();
		
		auto creationTime = std::chrono::system_clock::now() - room.age;
		roomNode["createdAt"].String() = formatTimestamp(creationTime);
		
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
