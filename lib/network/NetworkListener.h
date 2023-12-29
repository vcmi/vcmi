/*
 * NetworkListener.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class NetworkConnection;
class NetworkServer;
class NetworkClient;

using NetworkConnectionPtr = std::shared_ptr<NetworkConnection>;
using NetworkConnectionWeakPtr = std::weak_ptr<NetworkConnection>;

class DLL_LINKAGE INetworkConnectionListener
{
	friend class NetworkConnection;
protected:
	virtual void onDisconnected(const std::shared_ptr<NetworkConnection> & connection) = 0;
	virtual void onPacketReceived(const std::shared_ptr<NetworkConnection> & connection, const std::vector<uint8_t> & message) = 0;

public:
	virtual ~INetworkConnectionListener() = default;
};

class DLL_LINKAGE INetworkServerListener : public INetworkConnectionListener
{
	friend class NetworkServer;
protected:
	virtual void onNewConnection(const std::shared_ptr<NetworkConnection> &) = 0;
	virtual void onTimer() = 0;

public:
	virtual ~INetworkServerListener() = default;
};

class DLL_LINKAGE INetworkClientListener : public INetworkConnectionListener
{
	friend class NetworkClient;
protected:
	virtual void onConnectionFailed(const std::string & errorMessage) = 0;
	virtual void onConnectionEstablished(const std::shared_ptr<NetworkConnection> &) = 0;
	virtual void onTimer() = 0;

public:
	virtual ~INetworkClientListener() = default;
};


VCMI_LIB_NAMESPACE_END
