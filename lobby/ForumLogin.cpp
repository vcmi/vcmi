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

boost::asio::awaitable<std::optional<ForumLogin::Result>> ForumLogin::verifyCredentialsAsync(std::string username, std::string password, std::string forumHost)
{
	try
	{
		auto executor = co_await boost::asio::this_coro::executor;

		ssl::context sslCtx(ssl::context::tlsv12_client);
		sslCtx.set_default_verify_paths();
		sslCtx.set_verify_mode(ssl::verify_peer);

		tcp::resolver resolver(executor);
		beast::ssl_stream<beast::tcp_stream> stream(executor, sslCtx);

		if (!SSL_set_tlsext_host_name(stream.native_handle(), forumHost.c_str()))
		{
			logGlobal->warn("ForumLogin: failed to set SNI hostname");
			co_return std::nullopt;
		}

		auto endpoints = co_await resolver.async_resolve(forumHost, FORUM_PORT, boost::asio::use_awaitable);
		co_await beast::get_lowest_layer(stream).async_connect(endpoints, boost::asio::use_awaitable);
		co_await stream.async_handshake(ssl::stream_base::client, boost::asio::use_awaitable);

		// Step 1 – obtain CSRF token
		{
			http::request<http::empty_body> req(http::verb::get, "/session/csrf.json", 11);
			req.set(http::field::host, forumHost);
			req.set(http::field::user_agent, "VCMI-LobbyServer/1.0");
			req.set(http::field::accept, "application/json");
			co_await http::async_write(stream, req, boost::asio::use_awaitable);
		}

		beast::flat_buffer buf;
		http::response<http::string_body> csrfResp;
		co_await http::async_read(stream, buf, csrfResp, boost::asio::use_awaitable);

		if (csrfResp.result() != http::status::ok)
		{
			logGlobal->warn("ForumLogin: CSRF request failed with status %d", static_cast<int>(csrfResp.result()));
			co_return std::nullopt;
		}

		std::string csrfToken;
		std::string sessionCookie;

		{
			const std::string & body = csrfResp.body();
			JsonNode json(reinterpret_cast<const std::byte *>(body.data()), body.size(), "csrf_response");
			csrfToken = json["csrf"].String();
		}

		if (csrfToken.empty())
		{
			logGlobal->warn("ForumLogin: empty CSRF token in response");
			co_return std::nullopt;
		}

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
			req.set(http::field::host, forumHost);
			req.set(http::field::user_agent, "VCMI-LobbyServer/1.0");
			req.set(http::field::accept, "application/json");
			req.set(http::field::content_type, "application/x-www-form-urlencoded");
			req.set("X-CSRF-Token", csrfToken);
			if (!sessionCookie.empty())
				req.set(http::field::cookie, sessionCookie);
			req.content_length(formBody.size());
			req.body() = std::move(formBody);
			co_await http::async_write(stream, req, boost::asio::use_awaitable);
		}

		buf.consume(buf.size());
		http::response<http::string_body> loginResp;
		co_await http::async_read(stream, buf, loginResp, boost::asio::use_awaitable);

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
			authenticatedCookie = sessionCookie;

		beast::error_code shutdownEc;
		co_await stream.async_shutdown(boost::asio::redirect_error(boost::asio::use_awaitable, shutdownEc));

		if (loginResp.result() != http::status::ok)
		{
			logGlobal->info("ForumLogin: authentication failed for user '%s' (HTTP %d)", username, static_cast<int>(loginResp.result()));
			co_return std::nullopt;
		}

		const std::string & loginBody = loginResp.body();
		JsonNode loginJson(reinterpret_cast<const std::byte *>(loginBody.data()), loginBody.size(), "login_response");

		if (!loginJson["error"].String().empty())
		{
			logGlobal->info("ForumLogin: authentication failed for user '%s': %s", username, loginJson["error"].String());
			co_return std::nullopt;
		}

		std::string forumUsername = loginJson["user"]["username"].String();
		if (forumUsername.empty())
		{
			logGlobal->warn("ForumLogin: successful response but username is empty");
			co_return std::nullopt;
		}

		logGlobal->info("ForumLogin: authenticated forum user '%s'", forumUsername);
		co_return Result{forumUsername, authenticatedCookie};
	}
	catch (const std::exception & e)
	{
		logGlobal->warn("ForumLogin: exception during authentication: %s", e.what());
		co_return std::nullopt;
	}
}

boost::asio::awaitable<bool> ForumLogin::isSessionValidAsync(std::string sessionCookie, std::string forumHost)
{
	try
	{
		auto executor = co_await boost::asio::this_coro::executor;

		ssl::context sslCtx(ssl::context::tlsv12_client);
		sslCtx.set_default_verify_paths();
		sslCtx.set_verify_mode(ssl::verify_peer);

		tcp::resolver resolver(executor);
		beast::ssl_stream<beast::tcp_stream> stream(executor, sslCtx);

		if (!SSL_set_tlsext_host_name(stream.native_handle(), forumHost.c_str()))
			co_return false;

		auto endpoints = co_await resolver.async_resolve(forumHost, FORUM_PORT, boost::asio::use_awaitable);
		co_await beast::get_lowest_layer(stream).async_connect(endpoints, boost::asio::use_awaitable);
		co_await stream.async_handshake(ssl::stream_base::client, boost::asio::use_awaitable);

		http::request<http::empty_body> req(http::verb::get, "/session/current.json", 11);
		req.set(http::field::host, forumHost);
		req.set(http::field::user_agent, "VCMI-LobbyServer/1.0");
		req.set(http::field::accept, "application/json");
		req.set(http::field::cookie, sessionCookie);
		co_await http::async_write(stream, req, boost::asio::use_awaitable);

		beast::flat_buffer buf;
		http::response<http::string_body> resp;
		co_await http::async_read(stream, buf, resp, boost::asio::use_awaitable);

		beast::error_code ec;
		co_await stream.async_shutdown(boost::asio::redirect_error(boost::asio::use_awaitable, ec));

		co_return resp.result() == http::status::ok;
	}
	catch (const std::exception & e)
	{
		logGlobal->warn("ForumLogin: exception in isSessionValid: %s", e.what());
		co_return false;
	}
}
