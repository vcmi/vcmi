//
//  EnetService.hpp
//  vcmi
//
//  Created by nordsoft on 17.01.2023.
//

#pragma once
#include <thread>
#include <enet/enet.h>

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE EnetConnection
{
private:
	
	const unsigned int READ_REFRESH = 20; //ms
	
	std::list<ENetPacket*> packets;
	int channel = 0;
	ENetPeer * peer = nullptr;
	std::atomic<bool> connected;
	
public:
	EnetConnection(ENetPeer * peer);
	EnetConnection(ENetHost * client, const std::string & host, ui16 port);
	virtual ~EnetConnection();
	
	bool isOpen() const;
	void open();
	void close();
	void kill();

	const ENetPeer * getPeer() const;
	void dispatch(ENetPacket * packet);
	
	void write(const void * data, unsigned size);
	void read(void * data, unsigned size);
	
	std::mutex mutexRead, mutexWrite;
};

class DLL_LINKAGE EnetService
{
private:
	
	const unsigned int CHANNELS = 2;
	const unsigned int CONNECTIONS = 16;
	const unsigned int POLL_INTERVAL = 4; //ms
	const unsigned int MONITOR_INTERVAL = 50; //ms
	
	ENetHost * service;
	
	std::set<std::shared_ptr<EnetConnection>> active;
	std::list<std::shared_ptr<EnetConnection>> connecting, disconnecting;
	
	std::atomic<bool> doPolling, doMonitoring;
	bool flagValid;
	
	std::unique_ptr<std::thread> threadPolling, threadMonitoring;
	std::mutex mutex;
	
	void poll();
	void monitor();
	void start();
	
public:
	
	EnetService();
	virtual ~EnetService();
	
	void init(short port);
	void init(short port, const std::string & host);
	
	void stop();
	
	bool valid() const;
	
	virtual void handleDisconnection(std::shared_ptr<EnetConnection>) = 0;
	virtual void handleConnection(std::shared_ptr<EnetConnection>) = 0;
};

VCMI_LIB_NAMESPACE_END
