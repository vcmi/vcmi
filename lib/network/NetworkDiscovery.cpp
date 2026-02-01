/*
 * NetworkDiscovery.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "NetworkDiscovery.h"
#include "NetworkInterface.h"

VCMI_LIB_NAMESPACE_BEGIN

ServerDiscovery::ServerDiscovery(NetworkContext & context, std::function<void(const DiscoveredServer &)> callback)
	: context(context), callback(std::move(callback))
{
}

ServerDiscovery::~ServerDiscovery()
{
	abort();
}

void ServerDiscovery::abort()
{
	if(sendTimer)
	{
		boost::system::error_code ec;
		sendTimer->cancel(ec);
	}
	if(socket)
	{
		boost::system::error_code ec;
		socket->close(ec);
		socket.reset();
	}
}

void ServerDiscovery::start()
{
	socket = std::make_shared<boost::asio::ip::udp::socket>(context);
	socket->open(boost::asio::ip::udp::v4());
	socket->set_option(boost::asio::socket_base::broadcast(true));

	auto recvBuf = std::make_shared<std::array<char, 1024>>();
	auto senderEndpoint = std::make_shared<boost::asio::ip::udp::endpoint>();
	sendTimer = std::make_shared<boost::asio::steady_timer>(context);
	
	// Receive handler for incoming server responses
	auto receiveHandler = std::make_shared<std::function<void(const boost::system::error_code&, std::size_t)>>();
	*receiveHandler = [this, socket = this->socket, recvBuf, senderEndpoint, receiveHandler](const boost::system::error_code& ec, std::size_t len)
	{
		if(ec == boost::asio::error::operation_aborted)
			return;
		
		if(!ec && len > 0)
		{
			std::string resp(recvBuf->data(), recvBuf->data() + len);
			if(resp.rfind("VCMI_SERVER:", 0) == 0)
			{
				std::string portStr = resp.substr(12);
				DiscoveredServer server;
				server.address = senderEndpoint->address().to_string();
				server.port = static_cast<uint16_t>(std::stoi(portStr));
				
				auto serverKey = std::make_pair(server.address, server.port);
				if(discoveredServers.insert(serverKey).second)
				{
				    logGlobal->info("Discovered server: %s", resp.c_str());
					callback(server);
				}
			}
		}
		socket->async_receive_from(boost::asio::buffer(*recvBuf), *senderEndpoint, *receiveHandler);
	};
	
	// Send handler for broadcasting discovery message
	auto sendHandler = std::make_shared<std::function<void(const boost::system::error_code&)>>();
	*sendHandler = [this, socket = this->socket, sendHandler](const boost::system::error_code& ec)
	{
		if(ec == boost::asio::error::operation_aborted || ec == boost::asio::error::bad_descriptor)
			return;
		
		if(ec)
		{
			logGlobal->error("Discovery send error: %s", ec.message());
			return;
		}
		
		std::string message = "VCMI_DISCOVERY";
		boost::asio::ip::udp::endpoint broadcastEndpoint(boost::asio::ip::address_v4::broadcast(), 3030);
		socket->async_send_to(boost::asio::buffer(message), broadcastEndpoint,
			[this, sendHandler](const boost::system::error_code& ec, std::size_t)
			{
				if(ec && ec != boost::asio::error::operation_aborted && ec != boost::asio::error::bad_descriptor)
					logGlobal->error("Discovery send error: %s", ec.message());
				else if(!ec)
				{
					// Schedule next broadcast in 1 second
					sendTimer->expires_after(std::chrono::seconds(1));
					sendTimer->async_wait(*sendHandler);
				}
			}
		);
	};
	
	// Start receiving
	socket->async_receive_from(boost::asio::buffer(*recvBuf), *senderEndpoint, *receiveHandler);
	
	// Send first broadcast immediately
	(*sendHandler)(boost::system::error_code());
}

std::vector<std::string> ServerDiscovery::ipAddresses()
{
    std::vector<std::string> addresses;

    try
	{
        boost::asio::io_context io;
        boost::asio::ip::udp::socket socket(io);

        socket.connect({
            boost::asio::ip::make_address("8.8.8.8"), 53
        });

        auto addr = socket.local_endpoint().address();
        if (addr.is_v4())
            addresses.push_back(addr.to_string());
    }
    catch (const std::exception& e)
	{
        logGlobal->error("IP address retrieval error: %s", e.what());
    }

    return addresses;
}

ServerDiscoveryListener::ServerDiscoveryListener(NetworkContext & context, std::function<bool()> isInLobby, std::function<uint16_t()> getPort, unsigned short port)
	: context(context), isInLobby(std::move(isInLobby)), getPort(std::move(getPort)), port(port)
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

				if(msg == "VCMI_DISCOVERY" && isInLobby())
				{
					std::string response = "VCMI_SERVER:" + std::to_string(getPort());
					socketPtr->send_to(boost::asio::buffer(response), remoteEndpoint);
				}
			}

			asyncReceive();
		}
	);
}

VCMI_LIB_NAMESPACE_END
