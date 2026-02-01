
/*
 * ServerDiscoveryListener.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CVCMIServer.h"
#include "ServerDiscoveryListener.h"
#include "../lib/network/NetworkInterface.h"

ServerDiscoveryListener::ServerDiscoveryListener(CVCMIServer & server, unsigned short port)
	: server(server), port(port)
{
}

ServerDiscoveryListener::~ServerDiscoveryListener()
{
	stop();
}

void ServerDiscoveryListener::start()
{
	if(socket)
		return;

	auto & context = server.getNetworkHandler().getContext();
	boost::asio::ip::udp::endpoint listenEndpoint(boost::asio::ip::udp::v4(), port);

	socket = std::make_shared<boost::asio::ip::udp::socket>(context, listenEndpoint);
	asyncReceive();
}

void ServerDiscoveryListener::stop()
{
	if(socket)
	{
		boost::system::error_code ec;
		socket->close(ec);
		socket.reset();
	}
}

void ServerDiscoveryListener::asyncReceive()
{
	auto socketPtr = socket;
	if(!socketPtr)
		return;

	socketPtr->async_receive_from(
		boost::asio::buffer(recvBuffer),
		remoteEndpoint,
		[this, socketPtr](const boost::system::error_code & ec, std::size_t len)
		{
			if(ec == boost::asio::error::operation_aborted)
				return;

			if(!ec && len > 0)
			{
				std::string msg(recvBuffer.data(), recvBuffer.data() + len);

				if(msg == "VCMI_DISCOVERY" && server.getState() == EServerState::LOBBY)
				{
					std::string response = "VCMI_SERVER:" + std::to_string(server.getPort());
					socketPtr->send_to(boost::asio::buffer(response), remoteEndpoint);
				}
			}

			asyncReceive();
		}
	);
}
