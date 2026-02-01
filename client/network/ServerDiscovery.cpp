
/*
 * ServerDiscovery.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../GameEngine.h"
#include "ServerDiscovery.h"

#include "../../lib/network/NetworkDefines.h"

void ServerDiscovery::discoverAsync(NetworkContext & context, std::function<void(const DiscoveredServer &)> callback)
{
	auto socket = std::make_shared<boost::asio::ip::udp::socket>(context);
	socket->open(boost::asio::ip::udp::v4());
	socket->set_option(boost::asio::socket_base::broadcast(true));

	std::string message = "VCMI_DISCOVERY";
	boost::asio::ip::udp::endpoint broadcastEndpoint(boost::asio::ip::address_v4::broadcast(), 3030);
	socket->async_send_to(boost::asio::buffer(message), broadcastEndpoint,
		[socket, callback](const boost::system::error_code& ec, std::size_t)
		{
			if(ec)
			{
				logGlobal->error("Discovery send error: %s", ec.message());
				return;
			}

			auto recvBuf = std::make_shared<std::array<char, 1024>>();
			auto senderEndpoint = std::make_shared<boost::asio::ip::udp::endpoint>();
			auto timer = std::make_shared<boost::asio::steady_timer>(socket->get_executor(), std::chrono::milliseconds(5000));
			
			std::function<void(const boost::system::error_code&, std::size_t)> receiveHandler;
			receiveHandler = [socket, recvBuf, senderEndpoint, timer, callback, &receiveHandler](const boost::system::error_code& ec, std::size_t len)
			{
				if(!ec && len > 0)
				{
					std::string resp(recvBuf->data(), recvBuf->data() + len);
					if(resp.rfind("VCMI_SERVER:", 0) == 0)
					{
						logGlobal->info("Discovered server: %s", resp.c_str());
						std::string portStr = resp.substr(12);
						DiscoveredServer server;
						server.address = senderEndpoint->address().to_string();
						server.port = static_cast<uint16_t>(std::stoi(portStr));
						timer->cancel();
						ENGINE->dispatchMainThread(
							[callback, server]()
							{
								callback(server);
							}
						);
						return;
					}
				}
				socket->async_receive_from(boost::asio::buffer(*recvBuf), *senderEndpoint, receiveHandler);
			};
			
			socket->async_receive_from(boost::asio::buffer(*recvBuf), *senderEndpoint, receiveHandler);
			
			timer->async_wait([socket](const boost::system::error_code&) {
				boost::system::error_code ec;
				socket->close(ec);
			});
		}
	);
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
