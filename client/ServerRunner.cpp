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
#include "../server/CVCMIServer.h"

#ifndef VCMI_MOBILE
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#endif

ServerThreadRunner::ServerThreadRunner() = default;
ServerThreadRunner::~ServerThreadRunner() = default;
ServerProcessRunner::ServerProcessRunner() = default;
ServerProcessRunner::~ServerProcessRunner() = default;

void ServerThreadRunner::start(uint16_t port, bool connectToLobby)
{
	setThreadName("runServer");

	server = std::make_unique<CVCMIServer>(port, connectToLobby, true);

	threadRunLocalServer = boost::thread([this]{
		server->run();
	});
}

void ServerThreadRunner::stop()
{
	server->setState(EServerState::SHUTDOWN);
}

int ServerThreadRunner::wait()
{
	threadRunLocalServer.join();
	return 0;
}

void ServerProcessRunner::stop()
{
	child->terminate();
}

int ServerProcessRunner::wait()
{
	child->wait();

	return child->exit_code();

//	if (child->exit_code() == 0)
//	{
//		logNetwork->info("Server closed correctly");
//	}
//	else
//	{
//		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
//
//		if (getState() == EClientState::CONNECTING)
//		{
//			showServerError(CGI->generaltexth->translate("vcmi.server.errors.existingProcess"));
//			setState(EClientState::CONNECTION_CANCELLED); // stop attempts to reconnect
//		}
//		logNetwork->error("Error: server failed to close correctly or crashed!");
//		logNetwork->error("Check %s for more info", logName);
//	}
}

void ServerProcessRunner::start(uint16_t port, bool connectToLobby)
{
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


