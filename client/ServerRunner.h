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

class CVCMIServer;

class IServerRunner
{
public:
	virtual void start(uint16_t port, bool connectToLobby) = 0;
	virtual void stop() = 0;
	virtual int wait() = 0;

	virtual ~IServerRunner() = default;
};

/// Server instance will run as a thread of client process
class ServerThreadRunner : public IServerRunner, boost::noncopyable
{
	std::unique_ptr<CVCMIServer> server;
	boost::thread threadRunLocalServer;
public:
	void start(uint16_t port, bool connectToLobby) override;
	void stop() override;
	int wait() override;

	ServerThreadRunner();
	~ServerThreadRunner();
};

#ifndef VCMI_MOBILE

namespace boost::process {
class child;
}

/// Server instance will run as a separate process
class ServerProcessRunner : public IServerRunner, boost::noncopyable
{
	std::unique_ptr<boost::process::child> child;

public:
	void start(uint16_t port, bool connectToLobby) override;
	void stop() override;
	int wait() override;

	ServerProcessRunner();
	~ServerProcessRunner();
};
#endif
