/*
 * HttpServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HttpServer.h"
#include "EmbeddedWebAssets.h"

#include "../lib/logging/CLogger.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/// Extracts the value of a URL query parameter by key (e.g. "hours" from "/api?hours=24")
static std::string extractQueryParameter(boost::beast::string_view target, const std::string & key)
{
	std::string t(target);
	auto qpos = t.find('?');
	if (qpos == std::string::npos)
		return {};
	std::string query = t.substr(qpos + 1);
	const std::string prefix = key + '=';
	for (std::string::size_type pos = 0; pos < query.size(); )
	{
		auto amp = query.find('&', pos);
		std::string part = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
		if (part.substr(0, prefix.size()) == prefix)
			return part.substr(prefix.size());
		if (amp == std::string::npos) break;
		pos = amp + 1;
	}
	return {};
}

HttpServer::HttpServer(boost::asio::io_context & ioc, ILobbyHttpHandler & handler, unsigned short port, bool localhostOnly)
	: handler(handler)
	, port(port)
	, localhostOnly(localhostOnly)
	, ioc(ioc)
{
}

HttpServer::~HttpServer()
{
	stop();
}

void HttpServer::start()
{
	acceptor = std::make_unique<tcp::acceptor>(ioc);
	if (localhostOnly)
	{
		tcp::endpoint ep{boost::asio::ip::address_v4::loopback(), port};
		acceptor->open(ep.protocol());
		acceptor->set_option(tcp::acceptor::reuse_address(true));
		acceptor->bind(ep);
	}
	else
	{
		tcp::endpoint ep{tcp::v6(), port};
		acceptor->open(ep.protocol());
		acceptor->set_option(tcp::acceptor::reuse_address(true));
		acceptor->set_option(boost::asio::ip::v6_only(false));
		acceptor->bind(ep);
	}
	acceptor->listen();

	doAccept();
	logGlobal->info("HTTP API Server started on port %d", port);
}

void HttpServer::stop()
{
	if (acceptor && acceptor->is_open())
	{
		acceptor->close();
		logGlobal->info("HTTP API Server stopped");
	}
}

void HttpServer::doAccept()
{
	acceptor->async_accept([this](boost::system::error_code ec, tcp::socket socket)
	{
		if (!ec)
		{
			auto stream = std::make_shared<beast::tcp_stream>(std::move(socket));
			auto buffer = std::make_shared<beast::flat_buffer>();
			auto req = std::make_shared<http::request<http::string_body>>();

			http::async_read(*stream, *buffer, *req,
				[this, stream, buffer, req](boost::system::error_code readEc, std::size_t) mutable
				{
					if (readEc)
					{
						logGlobal->error("HTTP read error: %s", readEc.message());
						return;
					}
					try
					{
						auto res = std::make_shared<http::response<http::string_body>>(handleRequest(std::move(*req), *stream));
						http::async_write(*stream, *res,
							[stream, res](boost::system::error_code, std::size_t)
							{
								beast::error_code shutdownEc;
								stream->socket().shutdown(tcp::socket::shutdown_send, shutdownEc);
							});
					}
					catch (const std::exception & e)
					{
						logGlobal->error("HTTP session error: %s", e.what());
					}
				});
		}
		if (acceptor && acceptor->is_open())
			doAccept();
	});
}

HttpServer::Response HttpServer::makeResponse(const Request & req, http::status status, std::string body, const std::string & contentType)
{
	Response res{status, req.version()};
	res.set(http::field::server, "VCMI-Lobby-API");
	res.set(http::field::content_type, contentType);
	res.keep_alive(req.keep_alive());
	res.body() = std::move(body);
	res.prepare_payload();
	return res;
}

HttpServer::Response HttpServer::handleStatsV1(const Request & req)
{
	return makeResponse(req, http::status::ok, handler.getApiStats());
}

HttpServer::Response HttpServer::handleChatsV1(const Request & req)
{
	std::string channelName = "english";
	if (auto val = extractQueryParameter(req.target(), "channelName"); !val.empty())
		channelName = val;

	return makeResponse(req, http::status::ok, handler.getApiChats(channelName));
}

HttpServer::Response HttpServer::handleRoomsV1(const Request & req)
{
	int hours = -1;
	int limit = 50;
	if (auto val = extractQueryParameter(req.target(), "hours"); !val.empty())
	{
		try { hours = std::stoi(val); }
		catch (const std::invalid_argument &) { return makeResponse(req, http::status::bad_request, R"({"error":"Parameter 'hours' must be an integer"})"); }
		catch (const std::out_of_range &)     { return makeResponse(req, http::status::bad_request, R"({"error":"Parameter 'hours' is out of range"})"); }
	}
	if (auto val = extractQueryParameter(req.target(), "limit"); !val.empty())
	{
		try
		{
			limit = std::stoi(val);
			if (limit < 1 || limit > 250)
				return makeResponse(req, http::status::bad_request, R"({"error":"Parameter 'limit' must be between 1 and 250"})");
		}
		catch (const std::invalid_argument &) { return makeResponse(req, http::status::bad_request, R"({"error":"Parameter 'limit' must be an integer"})"); }
		catch (const std::out_of_range &)     { return makeResponse(req, http::status::bad_request, R"({"error":"Parameter 'limit' must be between 1 and 250"})"); }
	}

	return makeResponse(req, http::status::ok, handler.getApiRooms(hours, limit));
}

HttpServer::Response HttpServer::handleDocs(const Request & req)
{
	return makeResponse(req, http::status::ok, EmbeddedFiles::SWAGGER_CONTENT, "text/html");
}

HttpServer::Response HttpServer::handleOpenApiSpec(const Request & req)
{
	return makeResponse(req, http::status::ok, EmbeddedFiles::OPENAPI_CONTENT, "text/yaml");
}

HttpServer::Response HttpServer::handleRequest(Request && req, beast::tcp_stream & stream)
{
	std::string clientIP = "unknown";
	try
    {
		clientIP = stream.socket().remote_endpoint().address().to_string();
	}
	catch(const boost::system::system_error & e)
	{
		logGlobal->warn("HTTP API: could not get client IP: %s", e.what());
	}

	std::string userAgent = std::string(req[http::field::user_agent]);
	if (userAgent.empty())
        userAgent = "unknown";

	logGlobal->info("HTTP API Request: %s %s from %s (User-Agent: %s)",
		req.method_string().data(),
		req.target().data(),
		clientIP.c_str(),
		userAgent.c_str());

	try
	{
		if (req.target() == "/api/v1/stats")             return handleStatsV1(req);
		if (req.target().starts_with("/api/v1/chats"))   return handleChatsV1(req);
		if (req.target().starts_with("/api/v1/rooms"))   return handleRoomsV1(req);
		if (req.target() == "/api/docs" ||
		    req.target() == "/")                         return handleDocs(req);
		if (req.target() == "/api/openapi.yaml")         return handleOpenApiSpec(req);

		return makeResponse(req, http::status::not_found,
			R"({ "error": "Not Found", "message": "The requested endpoint does not exist" })");
	}
	catch (const std::exception & e)
	{
		logGlobal->error("Error handling HTTP request: %s", e.what());
		return makeResponse(req, http::status::internal_server_error, R"({"error":"Internal Server Error"})");
	}
}
