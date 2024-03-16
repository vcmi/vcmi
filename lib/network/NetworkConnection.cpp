/*
 * NetworkConnection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "NetworkConnection.h"

VCMI_LIB_NAMESPACE_BEGIN

NetworkConnection::NetworkConnection(INetworkConnectionListener & listener, const std::shared_ptr<NetworkSocket> & socket)
	: socket(socket)
	, listener(listener)
{
	socket->set_option(boost::asio::ip::tcp::no_delay(true));

	// iOS throws exception on attempt to set buffer size
	constexpr auto bufferSize = 4 * 1024 * 1024;

	try
	{
		socket->set_option(boost::asio::socket_base::send_buffer_size{bufferSize});
	}
	catch(const boost::system::system_error & e)
	{
		logNetwork->error("error setting 'send buffer size' socket option: %s", e.what());
	}

	try
	{
		socket->set_option(boost::asio::socket_base::receive_buffer_size{bufferSize});
	}
	catch(const boost::system::system_error & e)
	{
		logNetwork->error("error setting 'receive buffer size' socket option: %s", e.what());
	}
}

void NetworkConnection::start()
{
	boost::asio::async_read(*socket,
							readBuffer,
							boost::asio::transfer_exactly(messageHeaderSize),
							[self = shared_from_this()](const auto & ec, const auto & endpoint) { self->onHeaderReceived(ec); });
}

void NetworkConnection::onHeaderReceived(const boost::system::error_code & ecHeader)
{
	if (ecHeader)
	{
		listener.onDisconnected(shared_from_this(), ecHeader.message());
		return;
	}

	if (readBuffer.size() < messageHeaderSize)
		throw std::runtime_error("Failed to read header!");

	uint32_t messageSize;
	readBuffer.sgetn(reinterpret_cast<char *>(&messageSize), sizeof(messageSize));

	if (messageSize > messageMaxSize)
	{
		listener.onDisconnected(shared_from_this(), "Invalid packet size!");
		return;
	}

	if (messageSize == 0)
	{
		listener.onDisconnected(shared_from_this(), "Zero-sized packet!");
		return;
	}

	boost::asio::async_read(*socket,
							readBuffer,
							boost::asio::transfer_exactly(messageSize),
							[self = shared_from_this(), messageSize](const auto & ecPayload, const auto & endpoint) { self->onPacketReceived(ecPayload, messageSize); });
}

void NetworkConnection::onPacketReceived(const boost::system::error_code & ec, uint32_t expectedPacketSize)
{
	if (ec)
	{
		listener.onDisconnected(shared_from_this(), ec.message());
		return;
	}

	if (readBuffer.size() < expectedPacketSize)
	{
		throw std::runtime_error("Failed to read packet!");
	}

	std::vector<std::byte> message(expectedPacketSize);
	readBuffer.sgetn(reinterpret_cast<char *>(message.data()), expectedPacketSize);
	listener.onPacketReceived(shared_from_this(), message);

	start();
}

void NetworkConnection::sendPacket(const std::vector<std::byte> & message)
{
	boost::system::error_code ec;

	// create array with single element - boost::asio::buffer can be constructed from containers, but not from plain integer
	std::array<uint32_t, 1> messageSize{static_cast<uint32_t>(message.size())};

	boost::asio::write(*socket, boost::asio::buffer(messageSize), ec );
	boost::asio::write(*socket, boost::asio::buffer(message), ec );

	//Note: ignoring error code, intended
}

void NetworkConnection::close()
{
	boost::system::error_code ec;
	socket->close(ec);

	//NOTE: ignoring error code, intended
}

VCMI_LIB_NAMESPACE_END
