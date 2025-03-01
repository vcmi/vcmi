/*
 * ServerRunner.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ServerRunner.h"

#include "../lib/VCMIDirs.h"
#include "../lib/CThreadHelper.h"
#include "../lib/network/NetworkInterface.h"
#include "../lib/CConfigHandler.h"
#include "../server/CVCMIServer.h"

#ifdef ENABLE_SERVER_PROCESS

#if BOOST_VERSION >= 108600
// TODO: upgrade code to use v2 API instead of deprecated v1
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#else
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#endif

#endif

#include <future>

ServerThreadRunner::ServerThreadRunner() = default;
ServerThreadRunner::~ServerThreadRunner() = default;

void ServerThreadRunner::start(bool listenForConnections, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo)
{
	// cfgport may be 0 -- the real port is returned after calling prepare()
	uint16_t port = settings["server"]["localPort"].Integer();
	server = std::make_unique<CVCMIServer>(port, true);
	lobbyMode = connectToLobby;

	if (startingInfo)
	{
		server->si = startingInfo; //Else use default
	}

	std::promise<uint16_t> promise;

	threadRunLocalServer = std::thread([this, connectToLobby, listenForConnections, &promise]{
		setThreadName("runServer");
		uint16_t port = server->prepare(connectToLobby, listenForConnections);
		promise.set_value(port);
		server->run();
	});

	logNetwork->trace("Waiting for server port...");
	serverPort = promise.get_future().get();
	logNetwork->debug("Server port: %d", serverPort);
}

void ServerThreadRunner::shutdown()
{
	server->setState(EServerState::SHUTDOWN);
}

void ServerThreadRunner::wait()
{
	threadRunLocalServer.join();
}

int ServerThreadRunner::exitCode()
{
	return 0;
}

void ServerThreadRunner::connect(INetworkHandler & network, INetworkClientListener & listener)
{
	if (lobbyMode)
	{
		std::string host = settings["server"]["localHostname"].String();
		uint16_t port = settings["server"]["localPort"].Integer();
		logNetwork->info("Establishing connection to %s:%d...", host, port);

		network.connectToRemote(listener, host, port);
	}
	else
	{
		network.createInternalConnection(listener, server->getNetworkServer());
	}
}

#ifdef ENABLE_SERVER_PROCESS

ServerProcessRunner::ServerProcessRunner() = default;
ServerProcessRunner::~ServerProcessRunner() = default;

void ServerProcessRunner::shutdown()
{
	child->terminate();
}

void ServerProcessRunner::wait()
{
	child->wait();
}

int ServerProcessRunner::exitCode()
{
	return child->exit_code();
}

void ServerProcessRunner::start(bool listenForConnections, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo)
{
	uint16_t port = settings["server"]["localPort"].Integer();
	boost::filesystem::path serverPath = VCMIDirs::get().serverPath();
	boost::filesystem::path logPath = VCMIDirs::get().userLogsPath() / "server_log.txt";
	std::vector<std::string> args;
	args.push_back("--port=" + std::to_string(port));
	args.push_back("--run-by-client");
	if(connectToLobby)
		args.push_back("--lobby");

	std::error_code ec;
	child = std::make_unique<boost::process::child>(serverPath, args, ec, boost::process::std_out > logPath);

	if (ec)
		throw std::runtime_error("Failed to start server! Reason: " + ec.message());
}

void ServerProcessRunner::connect(INetworkHandler & network, INetworkClientListener & listener)
{
	std::string host = settings["server"]["localHostname"].String();
	uint16_t port = settings["server"]["localPort"].Integer();
	logNetwork->info("Establishing connection to %s:%d...", host, port);

	network.connectToRemote(listener, host, port);
}

#endif
