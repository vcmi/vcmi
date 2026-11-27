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

struct StartInfo;
class INetworkHandler;
class INetworkClientListener;

class CVCMIServer;
class ThreadServerLibraryReloader;

class IServerRunner
{
public:
	virtual void start(bool listenForConnections, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) = 0;
	virtual void shutdown() = 0;
	virtual void wait() = 0;
	virtual int exitCode() = 0;

	virtual void connect(INetworkHandler & network, INetworkClientListener & listener) = 0;

	/// Called after the client has reloaded its game library on LobbyReloadMods
	virtual void onLibraryReloaded(bool success) = 0;
	/// Returns true if the server uses the game library of the client, so it keeps its library when the client fails to load another one
	virtual bool sharesLibrary() const = 0;

	virtual ~IServerRunner() = default;
};

/// Class that runs server instance as a thread of client process
class ServerThreadRunner final : public IServerRunner, boost::noncopyable
{
	std::unique_ptr<CVCMIServer> server;
	/// Owned by server
	ThreadServerLibraryReloader * libraryReloader = nullptr;
	std::thread threadRunLocalServer;
	uint16_t serverPort = 0;
	bool lobbyMode = false;

public:
	void start(bool listenForConnections, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) override;
	void shutdown() override;
	void wait() override;
	int exitCode() override;

	void connect(INetworkHandler & network, INetworkClientListener & listener) override;
	void onLibraryReloaded(bool success) override;
	bool sharesLibrary() const override;

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
	void onLibraryReloaded(bool success) override;
	bool sharesLibrary() const override;

	ServerProcessRunner();
	~ServerProcessRunner();
};
#endif
