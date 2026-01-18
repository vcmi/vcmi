/*
 * HttpApiServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <thread>

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class LobbyServer;

class HttpApiServer
{
public:
	HttpApiServer(LobbyServer & lobbyServer, unsigned short port);
	~HttpApiServer();

	void start();
	void stop();

private:
	void run();
	void handleSession(boost::beast::tcp_stream stream);
	void handleRequest(boost::beast::http::request<boost::beast::http::string_body> && req, boost::beast::tcp_stream & stream);

	JsonNode getStats();
	JsonNode getChats(const std::string & channelName);
	JsonNode getRooms(int hours, int limit);

	LobbyServer & lobbyServer;
	unsigned short port;
	boost::asio::io_context ioc;
	std::unique_ptr<std::thread> thread;
	bool running;
	std::chrono::system_clock::time_point startTime;
};
