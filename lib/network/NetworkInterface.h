/*
 * NetworkHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// Base class for connections with other services, either incoming or outgoing
class DLL_LINKAGE INetworkConnection : boost::noncopyable
{
public:
	virtual ~INetworkConnection() = default;
	virtual void sendPacket(const std::vector<std::byte> & message) = 0;
	virtual void close() = 0;
};

using NetworkConnectionPtr = std::shared_ptr<INetworkConnection>;
using NetworkConnectionWeakPtr = std::weak_ptr<INetworkConnection>;

/// Base class for outgoing connections support
class DLL_LINKAGE INetworkClient : boost::noncopyable
{
public:
	virtual ~INetworkClient() = default;

	virtual bool isConnected() const = 0;
	virtual void sendPacket(const std::vector<std::byte> & message) = 0;
};

/// Base class for incoming connections support
class DLL_LINKAGE INetworkServer : boost::noncopyable
{
public:
	virtual ~INetworkServer() = default;

	virtual void start(uint16_t port) = 0;
};

/// Base interface that must be implemented by user of networking API to handle any connection callbacks
class DLL_LINKAGE INetworkConnectionListener
{
public:
	virtual void onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage) = 0;
	virtual void onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message) = 0;

	virtual ~INetworkConnectionListener() = default;
};

/// Interface that must be implemented by user of networking API to handle outgoing connection callbacks
class DLL_LINKAGE INetworkClientListener : public INetworkConnectionListener
{
public:
	virtual void onConnectionFailed(const std::string & errorMessage) = 0;
	virtual void onConnectionEstablished(const std::shared_ptr<INetworkConnection> &) = 0;
};

/// Interface that must be implemented by user of networking API to handle incoming connection callbacks
class DLL_LINKAGE INetworkServerListener : public INetworkConnectionListener
{
public:
	virtual void onNewConnection(const std::shared_ptr<INetworkConnection> &) = 0;
};

/// Interface that must be implemented by user of networking API to handle timers on network thread
class DLL_LINKAGE INetworkTimerListener
{
public:
	virtual ~INetworkTimerListener() = default;

	virtual void onTimer() = 0;
};

/// Main class for handling of all network activity
class DLL_LINKAGE INetworkHandler : boost::noncopyable
{
public:
	virtual ~INetworkHandler() = default;

	/// Constructs default implementation
	static std::unique_ptr<INetworkHandler> createHandler();

	/// Creates an instance of TCP server that allows to receiving connections on a local port
	virtual std::unique_ptr<INetworkServer> createServerTCP(INetworkServerListener & listener) = 0;

	/// Creates an instance of TCP client that allows to establish single outgoing connection to a remote port
	/// On success: INetworkTimerListener::onConnectionEstablished() will be called, established connection provided as parameter
	/// On failure: INetworkTimerListener::onConnectionFailed will be called with human-readable error message
	virtual void connectToRemote(INetworkClientListener & listener, const std::string & host, uint16_t port) = 0;

	/// Creates a timer that will be called once, after specified interval has passed
	/// On success: INetworkTimerListener::onTimer() will be called
	/// On failure: no-op
	virtual void createTimer(INetworkTimerListener & listener, std::chrono::milliseconds duration) = 0;

	/// Starts network processing on this thread. Does not returns until networking processing has been terminated
	virtual void run() = 0;
	virtual void stop() = 0;
};

VCMI_LIB_NAMESPACE_END
