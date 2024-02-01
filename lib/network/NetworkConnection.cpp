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
	socket->set_option(boost::asio::socket_base::send_buffer_size(4194304));
	socket->set_option(boost::asio::socket_base::receive_buffer_size(4194304));
}

void NetworkConnection::start()
{
	boost::asio::async_read(*socket,
							readBuffer,
							boost::asio::transfer_exactly(messageHeaderSize),
							[this](const auto & ec, const auto & endpoint) { onHeaderReceived(ec); });
}

void NetworkConnection::onHeaderReceived(const boost::system::error_code & ec)
{
	if (ec)
	{
		listener.onDisconnected(shared_from_this(), ec.message());
		return;
	}

	uint32_t messageSize = readPacketSize();

	boost::asio::async_read(*socket,
							readBuffer,
							boost::asio::transfer_exactly(messageSize),
							[this, messageSize](const auto & ec, const auto & endpoint) { onPacketReceived(ec, messageSize); });
}

uint32_t NetworkConnection::readPacketSize()
{
	if (readBuffer.size() < messageHeaderSize)
		throw std::runtime_error("Failed to read header!");

	uint32_t messageSize;
	readBuffer.sgetn(reinterpret_cast<char *>(&messageSize), sizeof(messageSize));

	if (messageSize > messageMaxSize)
		throw std::runtime_error("Invalid packet size!");

	return messageSize;
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
		throw std::runtime_error("Failed to read header!");
	}

	std::vector<std::byte> message(expectedPacketSize);
	readBuffer.sgetn(reinterpret_cast<char *>(message.data()), expectedPacketSize);
	listener.onPacketReceived(shared_from_this(), message);

	start();
}

void NetworkConnection::sendPacket(const std::vector<std::byte> & message)
{
	boost::system::error_code ec;

	std::array<uint32_t, 1> messageSize{static_cast<uint32_t>(message.size())};

	boost::asio::write(*socket, boost::asio::buffer(messageSize), ec );
	boost::asio::write(*socket, boost::asio::buffer(message), ec );

	// FIXME: handle error?
}

VCMI_LIB_NAMESPACE_END
