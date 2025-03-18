/*
 * ServerRunner.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct StartInfo;
class INetworkHandler;
class INetworkClientListener;

VCMI_LIB_NAMESPACE_END

class CVCMIServer;

class IServerRunner
{
public:
	virtual void start(bool listenForConnections, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) = 0;
	virtual void shutdown() = 0;
	virtual void wait() = 0;
	virtual int exitCode() = 0;

	virtual void connect(INetworkHandler & network, INetworkClientListener & listener) = 0;

	virtual ~IServerRunner() = default;
};

/// Class that runs server instance as a thread of client process
class ServerThreadRunner final : public IServerRunner, boost::noncopyable
{
	std::unique_ptr<CVCMIServer> server;
	std::thread threadRunLocalServer;
	uint16_t serverPort = 0;
	bool lobbyMode = false;

public:
	void start(bool listenForConnections, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) override;
	void shutdown() override;
	void wait() override;
	int exitCode() override;

	void connect(INetworkHandler & network, INetworkClientListener & listener) override;

	ServerThreadRunner();
	~ServerThreadRunner();
};

#ifndef VCMI_MOBILE
// Enable support for running vcmiserver as separate process. Unavailable on mobile systems
#define ENABLE_SERVER_PROCESS
#endif

#ifdef ENABLE_SERVER_PROCESS

#if BOOST_VERSION >= 108600
namespace boost::process {
inline namespace v1 {
class child;
}
}
#else
namespace boost::process {
class child;
}
#endif

/// Class that runs server instance as a child process
/// Available only on desktop systems where process management is allowed
class ServerProcessRunner final : public IServerRunner, boost::noncopyable
{
	std::unique_ptr<boost::process::child> child;

public:
	void start(bool listenForConnections, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) override;
	void shutdown() override;
	void wait() override;
	int exitCode() override;

	void connect(INetworkHandler & network, INetworkClientListener & listener) override;

	ServerProcessRunner();
	~ServerProcessRunner();
};
#endif
