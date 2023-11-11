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

NetworkConnection::NetworkConnection(const std::shared_ptr<NetworkSocket> & socket)
	: socket(socket)
{

}

void NetworkConnection::start()
{
	boost::asio::async_read(*socket,
							readBuffer,
							boost::asio::transfer_exactly(messageHeaderSize),
							std::bind(&NetworkConnection::onHeaderReceived,this, _1));
}

void NetworkConnection::onHeaderReceived(const boost::system::error_code & ec)
{
	uint32_t messageSize = readPacketSize(ec);

	boost::asio::async_read(*socket,
							readBuffer,
							boost::asio::transfer_exactly(messageSize),
							std::bind(&NetworkConnection::onPacketReceived,this, _1, messageSize));
}

uint32_t NetworkConnection::readPacketSize(const boost::system::error_code & ec)
{
	if (ec)
	{
		throw std::runtime_error("Connection aborted!");
	}

	if (readBuffer.size() < messageHeaderSize)
	{
		throw std::runtime_error("Failed to read header!");
	}

	std::istream istream(&readBuffer);

	uint32_t messageSize;
	istream.read(reinterpret_cast<char *>(&messageSize), messageHeaderSize);

	if (messageSize > messageMaxSize)
	{
		throw std::runtime_error("Invalid packet size!");
	}

	return messageSize;
}

void NetworkConnection::onPacketReceived(const boost::system::error_code & ec, uint32_t expectedPacketSize)
{
	if (ec)
	{
		throw std::runtime_error("Connection aborted!");
	}

	if (readBuffer.size() < expectedPacketSize)
	{
		throw std::runtime_error("Failed to read header!");
	}

	std::vector<uint8_t> message;

	message.resize(expectedPacketSize);
	std::istream istream(&readBuffer);
	istream.read(reinterpret_cast<char *>(message.data()), messageHeaderSize);

	start();
}

void NetworkConnection::sendPacket(const std::vector<uint8_t> & message)
{
	NetworkBuffer writeBuffer;

	std::ostream ostream(&writeBuffer);
	uint32_t messageSize = message.size();
	ostream.write(reinterpret_cast<const char *>(&messageSize), messageHeaderSize);
	ostream.write(reinterpret_cast<const char *>(message.data()), message.size());

	boost::asio::write(*socket, writeBuffer );
}

VCMI_LIB_NAMESPACE_END
