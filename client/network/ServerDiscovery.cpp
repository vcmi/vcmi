
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
#include <boost/asio.hpp>

#include "../lib/AsyncRunner.h"

void ServerDiscovery::discoverAsync(std::function<void(const DiscoveredServer &)> callback)
{
	ENGINE->async().run(
		[callback]
		{
			try
			{
				boost::asio::io_context io_context;
				boost::asio::ip::udp::socket socket(io_context);
				socket.open(boost::asio::ip::udp::v4());
				socket.set_option(boost::asio::socket_base::broadcast(true));

				std::string message = "VCMI_DISCOVERY";
				boost::asio::ip::udp::endpoint broadcast_endpoint(boost::asio::ip::address_v4::broadcast(), 3030);
				socket.send_to(boost::asio::buffer(message), broadcast_endpoint);

				char recv_buf[1024];
				boost::asio::ip::udp::endpoint sender_endpoint;
				socket.non_blocking(true);
				auto start = std::chrono::steady_clock::now();
				while(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(5000))
				{
					boost::system::error_code ec;
					size_t len = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint, 0, ec);
					if(!ec && len > 0)
					{
						std::string resp(recv_buf, recv_buf + len);
						if(resp.rfind("VCMI_SERVER:", 0) == 0)
						{
							logGlobal->info("Discovered server: %s", resp.c_str());
							std::string portStr = resp.substr(12);
							DiscoveredServer server;
							server.address = sender_endpoint.address().to_string();
							server.port = static_cast<uint16_t>(std::stoi(portStr));
							ENGINE->dispatchMainThread(
								[callback, server]()
								{
									callback(server);
								}
							);
							break;
						}
					}
				}
			}
			catch(const std::exception & e)
			{
				logGlobal->error("Discovery error: %s", e.what());
			}
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
