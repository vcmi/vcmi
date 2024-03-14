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

VCMI_LIB_NAMESPACE_END

class CVCMIServer;

class IServerRunner
{
public:
	virtual void start(uint16_t port, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) = 0;
	virtual void shutdown() = 0;
	virtual void wait() = 0;
	virtual int exitCode() = 0;

	virtual ~IServerRunner() = default;
};

/// Class that runs server instance as a thread of client process
class ServerThreadRunner : public IServerRunner, boost::noncopyable
{
	std::unique_ptr<CVCMIServer> server;
	boost::thread threadRunLocalServer;
public:
	void start(uint16_t port, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) override;
	void shutdown() override;
	void wait() override;
	int exitCode() override;

	ServerThreadRunner();
	~ServerThreadRunner();
};

#ifndef VCMI_MOBILE

namespace boost::process {
class child;
}

/// Class that runs server instance as a child process
/// Available only on desktop systems where process management is allowed
class ServerProcessRunner : public IServerRunner, boost::noncopyable
{
	std::unique_ptr<boost::process::child> child;

public:
	void start(uint16_t port, bool connectToLobby, std::shared_ptr<StartInfo> startingInfo) override;
	void shutdown() override;
	void wait() override;
	int exitCode() override;

	ServerProcessRunner();
	~ServerProcessRunner();
};
#endif
