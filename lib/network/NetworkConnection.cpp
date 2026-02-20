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

NetworkConnection::NetworkConnection(INetworkConnectionListener & listener, const std::shared_ptr<NetworkSocket> & socket, NetworkContext & context)
	: socket(socket)
	, timer(std::make_shared<NetworkTimer>(context))
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
	heartbeat();
	startReceiving();
}

void NetworkConnection::startReceiving()
{
	boost::asio::async_read(*socket,
							readBuffer,
							boost::asio::transfer_exactly(messageHeaderSize),
							[self = shared_from_this()](const auto & ec, const auto & endpoint) { self->onHeaderReceived(ec); });
}

void NetworkConnection::heartbeat()
{
	constexpr auto heartbeatInterval = std::chrono::seconds(10);

	timer->expires_after(heartbeatInterval);
	timer->async_wait( [self = weak_from_this()](const auto & ec)
	{
		if (ec)
			return;

		auto locked = self.lock();
		if (!locked)
			return;

		locked->sendPacket({});
		locked->heartbeat();
	});
}

void NetworkConnection::onHeaderReceived(const boost::system::error_code & ecHeader)
{
	if (ecHeader)
	{
		onError(ecHeader.message());
		return;
	}

	if (readBuffer.size() < messageHeaderSize)
		throw std::runtime_error("Failed to read header!");

	uint32_t messageSize;
	readBuffer.sgetn(reinterpret_cast<char *>(&messageSize), sizeof(messageSize));

	if (messageSize > messageMaxSize)
	{
		onError("Invalid packet size!");
		return;
	}

	if (messageSize == 0)
	{
		//heartbeat package with no payload - wait for next packet
		startReceiving();
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
		onError(ec.message());
		return;
	}

	if (readBuffer.size() < expectedPacketSize)
	{
		// FIXME: figure out what causes this. This should not be possible without error set
		std::string errorMessage = "Failed to read packet! " + std::to_string(readBuffer.size()) + " bytes read, but " + std::to_string(expectedPacketSize) + " bytes expected!";
		onError(errorMessage);
	}

	std::vector<std::byte> message(expectedPacketSize);
	readBuffer.sgetn(reinterpret_cast<char *>(message.data()), expectedPacketSize);
	listener.onPacketReceived(shared_from_this(), message);

	startReceiving();
}

void NetworkConnection::setAsyncWritesEnabled(bool on)
{
	asyncWritesEnabled = on;
}

void NetworkConnection::sendPacket(const std::vector<std::byte> & message)
{
	std::lock_guard lock(writeMutex);
	std::vector<std::byte> headerVector(sizeof(uint32_t));
	uint32_t messageSize = message.size();
	std::memcpy(headerVector.data(), &messageSize, sizeof(uint32_t));

	// At the moment, vcmilobby *requires* async writes in order to handle multiple connections with different speeds and at optimal performance
	// However server (and potentially - client) can not handle this mode and may shutdown either socket or entire asio service too early, before all writes are performed
	if (asyncWritesEnabled)
	{

		bool messageQueueEmpty = dataToSend.empty();
		dataToSend.push_back(headerVector);
		if (!message.empty())
			dataToSend.push_back(message);

		if (messageQueueEmpty)
			doSendData();
		//else - data sending loop is still active and still sending previous messages
	}
	else
	{
		boost::system::error_code ec;
		boost::asio::write(*socket, boost::asio::buffer(headerVector), ec );
		if (!message.empty())
			boost::asio::write(*socket, boost::asio::buffer(message), ec );
	}
}

void NetworkConnection::doSendData()
{
	if (dataToSend.empty())
		throw std::runtime_error("Attempting to sent data but there is no data to send!");

	boost::asio::async_write(*socket, boost::asio::buffer(dataToSend.front()), [self = shared_from_this()](const auto & error, const auto & )
	{
		self->onDataSent(error);
	});
}

void NetworkConnection::onDataSent(const boost::system::error_code & ec)
{
	std::lock_guard lock(writeMutex);
	dataToSend.pop_front();
	if (ec)
	{
		onError(ec.message());
		return;
	}

	if (!dataToSend.empty())
		doSendData();
}

void NetworkConnection::onError(const std::string & message)
{
	listener.onDisconnected(shared_from_this(), message);
	close();
}

void NetworkConnection::close()
{
	boost::system::error_code ec;
	socket->close(ec);
#if BOOST_VERSION >= 108700
	timer->cancel();
#else
	timer->cancel(ec);
#endif

	//NOTE: ignoring error code, intended
}

InternalConnection::InternalConnection(INetworkConnectionListener & listener, NetworkContext & context)
	: context(context)
	, listener(listener)
{
}

void InternalConnection::receivePacket(const std::vector<std::byte> & message)
{
	boost::asio::post(context, [self = std::static_pointer_cast<InternalConnection>(shared_from_this()), message](){
		if (self->connectionActive)
			self->listener.onPacketReceived(self, message);
	});
}

void InternalConnection::disconnect()
{
	boost::asio::post(context, [self = std::static_pointer_cast<InternalConnection>(shared_from_this())](){
		self->listener.onDisconnected(self, "Internal connection has been terminated");
		self->otherSideWeak.reset();
		self->connectionActive = false;
	});
}

void InternalConnection::connectTo(std::shared_ptr<IInternalConnection> connection)
{
	otherSideWeak = connection;
	connectionActive = true;
}

void InternalConnection::sendPacket(const std::vector<std::byte> & message)
{
	auto otherSide = otherSideWeak.lock();

	if (otherSide)
		otherSide->receivePacket(message);
}

void InternalConnection::setAsyncWritesEnabled(bool on)
{
	// no-op
}

void InternalConnection::close()
{
	auto otherSide = otherSideWeak.lock();

	if (otherSide)
		otherSide->disconnect();

	otherSideWeak.reset();
	connectionActive = false;
}

VCMI_LIB_NAMESPACE_END
