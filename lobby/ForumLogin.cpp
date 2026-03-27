/*
 * ForumLogin.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ForumLogin.h"

#include "../lib/json/JsonNode.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
namespace ssl   = boost::asio::ssl;
using tcp = net::ip::tcp;

static const std::string FORUM_HOST = "forum.vcmi.eu";
static const std::string FORUM_PORT = "443";

static std::string urlEncode(const std::string & input)
{
	std::string encoded;
	encoded.reserve(input.size() * 3);
	for (const unsigned char c : input)
	{
		if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
		{
			encoded += static_cast<char>(c);
		}
		else
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned>(c));
			encoded += buf;
		}
	}
	return encoded;
}

std::optional<ForumLogin::Result> ForumLogin::verifyCredentials(const std::string & username, const std::string & password)
{
	try
	{
		net::io_context ioc;
		ssl::context sslCtx(ssl::context::tlsv12_client);
		sslCtx.set_default_verify_paths();
		sslCtx.set_verify_mode(ssl::verify_peer);

		tcp::resolver resolver(ioc);
		beast::ssl_stream<beast::tcp_stream> stream(ioc, sslCtx);

		// Set SNI hostname for proper TLS handshake
		if (!SSL_set_tlsext_host_name(stream.native_handle(), FORUM_HOST.c_str()))
		{
			logGlobal->warn("ForumLogin: failed to set SNI hostname");
			return std::nullopt;
		}

		auto const endpoints = resolver.resolve(FORUM_HOST, FORUM_PORT);
		beast::get_lowest_layer(stream).connect(endpoints);
		stream.handshake(ssl::stream_base::client);

		// Step 1 – obtain CSRF token
		{
			http::request<http::empty_body> req(http::verb::get, "/session/csrf.json", 11);
			req.set(http::field::host, FORUM_HOST);
			req.set(http::field::user_agent, "VCMI-LobbyServer/1.0");
			req.set(http::field::accept, "application/json");
			http::write(stream, req);
		}

		beast::flat_buffer buf;
		http::response<http::string_body> csrfResp;
		http::read(stream, buf, csrfResp);

		if (csrfResp.result() != http::status::ok)
		{
			logGlobal->warn("ForumLogin: CSRF request failed with status %d", static_cast<int>(csrfResp.result()));
			return std::nullopt;
		}

		std::string csrfToken;
		std::string sessionCookie;

		// Parse CSRF token from JSON body
		{
			const std::string & body = csrfResp.body();
			JsonNode json(reinterpret_cast<const std::byte *>(body.data()), body.size(), "csrf_response");
			csrfToken = json["csrf"].String();
		}

		if (csrfToken.empty())
		{
			logGlobal->warn("ForumLogin: empty CSRF token in response");
			return std::nullopt;
		}

		// Collect session cookies from Set-Cookie headers of CSRF response
		// (needed to send back with the login POST)
		for (auto const & hdr : csrfResp)
		{
			if (boost::iequals(hdr.name_string(), "set-cookie"))
			{
				std::string_view cookieStr = hdr.value();
				auto pos = cookieStr.find(';');
				sessionCookie += cookieStr.substr(0, pos);
				sessionCookie += "; ";
			}
		}

		// Step 2 – POST credentials
		std::string formBody = "login=" + urlEncode(username) + "&password=" + urlEncode(password);

		{
			http::request<http::string_body> req(http::verb::post, "/session", 11);
			req.set(http::field::host, FORUM_HOST);
			req.set(http::field::user_agent, "VCMI-LobbyServer/1.0");
			req.set(http::field::accept, "application/json");
			req.set(http::field::content_type, "application/x-www-form-urlencoded");
			req.set("X-CSRF-Token", csrfToken);
			if (!sessionCookie.empty())
				req.set(http::field::cookie, sessionCookie);
			req.content_length(formBody.size());
			req.body() = std::move(formBody);
			http::write(stream, req);
		}

		buf.consume(buf.size());
		http::response<http::string_body> loginResp;
		http::read(stream, buf, loginResp);

		// Collect the authenticated session cookie from the POST /session response
		std::string authenticatedCookie;
		for (auto const & hdr : loginResp)
		{
			if (boost::iequals(hdr.name_string(), "set-cookie"))
			{
				std::string_view cookieStr = hdr.value();
				auto pos = cookieStr.find(';');
				authenticatedCookie += cookieStr.substr(0, pos);
				authenticatedCookie += "; ";
			}
		}
		if (authenticatedCookie.empty())
			authenticatedCookie = sessionCookie; // fall back to CSRF-phase cookies

		// Graceful SSL shutdown (ignore errors – connection will close anyway)
		beast::error_code shutdownEc;
		stream.shutdown(shutdownEc);

		if (loginResp.result() != http::status::ok)
		{
			logGlobal->info("ForumLogin: authentication failed for user '%s' (HTTP %d)", username, static_cast<int>(loginResp.result()));
			return std::nullopt;
		}

		const std::string & loginBody = loginResp.body();
		JsonNode loginJson(reinterpret_cast<const std::byte *>(loginBody.data()), loginBody.size(), "login_response");

		if (!loginJson["error"].String().empty())
		{
			logGlobal->info("ForumLogin: authentication failed for user '%s': %s", username, loginJson["error"].String());
			return std::nullopt;
		}

		std::string forumUsername = loginJson["user"]["username"].String();
		if (forumUsername.empty())
		{
			logGlobal->warn("ForumLogin: successful response but username is empty");
			return std::nullopt;
		}

		logGlobal->info("ForumLogin: authenticated forum user '%s'", forumUsername);
		return Result{forumUsername, authenticatedCookie};
	}
	catch (const std::exception & e)
	{
		logGlobal->warn("ForumLogin: exception during authentication: %s", e.what());
		return std::nullopt;
	}
}

bool ForumLogin::isSessionValid(const std::string & sessionCookie)
{
	try
	{
		net::io_context ioc;
		ssl::context sslCtx(ssl::context::tlsv12_client);
		sslCtx.set_default_verify_paths();
		sslCtx.set_verify_mode(ssl::verify_peer);

		tcp::resolver resolver(ioc);
		beast::ssl_stream<beast::tcp_stream> stream(ioc, sslCtx);

		if (!SSL_set_tlsext_host_name(stream.native_handle(), FORUM_HOST.c_str()))
			return false;

		beast::get_lowest_layer(stream).connect(resolver.resolve(FORUM_HOST, FORUM_PORT));
		stream.handshake(ssl::stream_base::client);

		http::request<http::empty_body> req(http::verb::get, "/session/current.json", 11);
		req.set(http::field::host, FORUM_HOST);
		req.set(http::field::user_agent, "VCMI-LobbyServer/1.0");
		req.set(http::field::accept, "application/json");
		req.set(http::field::cookie, sessionCookie);
		http::write(stream, req);

		beast::flat_buffer buf;
		http::response<http::string_body> resp;
		http::read(stream, buf, resp);

		beast::error_code ec;
		stream.shutdown(ec);

		return resp.result() == http::status::ok;
	}
	catch (const std::exception & e)
	{
		logGlobal->warn("ForumLogin: exception in isSessionValid: %s", e.what());
		return false;
	}
}
