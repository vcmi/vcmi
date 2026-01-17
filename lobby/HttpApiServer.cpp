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
	auto const createResponse = [&req](http::status status, const std::string & body)
	{
		http::response<http::string_body> res{status, req.version()};
		res.set(http::field::server, "VCMI-Lobby-API");
		res.set(http::field::content_type, "application/json");
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
	stats["totalGames"].Integer() = 0;
	stats["activePlayers"].Integer() = lobbyServer.getDatabase()->getActiveAccounts().size();
	stats["startTime"].String() = std::format("{:%Y-%m-%dT%H:%M:%S}", startTime);
	stats["server"].String() = "VCMI Lobby";
	stats["lobbyVersion"].String() = GameConstants::VCMI_VERSION;
	stats["apiVersion"].String() = "1.0";
	return stats;
}
