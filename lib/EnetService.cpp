//
//  EnetService.cpp
//  vcmi
//
//  Created by nordsoft on 17.01.2023.
//

#include "EnetService.h"
#include "StdInc.h"
#include "CThreadHelper.h"
#include <thread>

EnetConnection::EnetConnection(ENetPeer * _peer):
	peer(_peer)
{
	connected = false;
}

EnetConnection::EnetConnection(ENetHost * client, const std::string & host, ui16 port)
{
	connected = false;
	ENetAddress address;
	enet_address_set_host(&address, host.c_str());
	address.port = port;
	
	peer = enet_host_connect(client, &address, 2, 0);
	if(!peer)
	{
		throw std::runtime_error("Can't establish connection :(");
	}
}

EnetConnection::~EnetConnection()
{
	close();
	kill();
}

bool EnetConnection::isOpen() const
{
	return connected;
}

void EnetConnection::open()
{
	connected = true;
}

void EnetConnection::close()
{
	std::lock_guard<std::mutex> guard(mutexWrite);
	connected = false;
	enet_peer_disconnect(peer, 0);
}

void EnetConnection::kill()
{
	connected = false;
	if(peer)
		enet_peer_reset(peer);
	peer = nullptr;
}

const ENetPeer * EnetConnection::getPeer() const
{
	return peer;
}

void EnetConnection::dispatch(ENetPacket * packet)
{
	std::lock_guard<std::mutex> guard(mutexRead);
	packets.push_back(packet);
}

void EnetConnection::write(const void * data, unsigned size)
{
	if(size == 0 || !isOpen())
		return;
	
	std::lock_guard<std::mutex> guard(mutexWrite);
	ENetPacket * packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(peer, channel, packet);
}

void EnetConnection::read(void * data, unsigned size)
{
	if(!size)
		return;
	
	while(packets.empty())
	{
		if(!isOpen())
			return;
		
		std::this_thread::sleep_for(std::chrono::milliseconds(READ_REFRESH));
	}
	
	std::lock_guard<std::mutex> guard(mutexRead);
	auto * packet = packets.front();
	packets.pop_front();
	
	if(packet->dataLength > 0)
		memcpy(data, packet->data, packet->dataLength);
	
	assert(size == packet->dataLength);
	enet_packet_destroy(packet);
}

EnetService::EnetService():
	service(nullptr), flagValid(false)
{
	if(enet_initialize() != 0)
		throw std::runtime_error("Cannot initialize enet");
	
	doMonitoring = true;
	threadMonitoring = std::make_unique<std::thread>(&EnetService::monitor, this);
	threadMonitoring->detach();
}

EnetService::~EnetService()
{
	stop();
	doMonitoring = false;
	if(threadMonitoring)
		threadMonitoring->join();
	
	enet_deinitialize();
}

void EnetService::init(short port, const std::string & host)
{
	init(0);
	active.insert(std::make_shared<EnetConnection>(service, host, port));
}

void EnetService::init(short port)
{
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;
	
	service = enet_host_create(port ? &address : nullptr, port ? CONNECTIONS : 1, CHANNELS, 0, 0);
	if(service)
		flagValid = true;
	
	start();
}

void EnetService::start()
{
	stop();
	doPolling = true;
	
	if(service)
	{
		threadPolling = std::make_unique<std::thread>(&EnetService::poll, this);
		threadPolling->detach();
	}
}

void EnetService::monitor()
{
	setThreadName("EnetService::monitor");
	while(doMonitoring)
	{
		while(!disconnecting.empty())
		{
			std::lock_guard<std::mutex> guard(mutex);
			if(disconnecting.front()->isOpen())
				disconnecting.front()->kill();
			handleDisconnection(disconnecting.front());
			disconnecting.pop_front();
		}
		while(!connecting.empty())
		{
			std::lock_guard<std::mutex> guard(mutex);
			connecting.front()->open();
			handleConnection(connecting.front());
			connecting.pop_front();
		}
		if(service)
			enet_host_flush(service);
		std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_INTERVAL));
	}
}

void EnetService::stop()
{
	doPolling = false;
	if(threadPolling)
		threadPolling->join();
	threadPolling.reset();
}

bool EnetService::valid() const
{
	return flagValid;
}

void EnetService::poll()
{
	setThreadName("EnetService::poll");
	ENetEvent event;
	while(doPolling)
	{
		if(enet_host_service(service, &event, POLL_INTERVAL) > 0)
		{
			switch(event.type)
			{
				case ENET_EVENT_TYPE_CONNECT: {
					bool receiverFound = false;
					for(auto & c : active)
					{
						if(c->getPeer() == event.peer)
						{
							std::lock_guard<std::mutex> guard(mutex);
							connecting.push_back(c);
							receiverFound = true;
							break;
						}
					}
					
					if(!receiverFound)
					{
						auto c = std::make_shared<EnetConnection>(event.peer);
						active.insert(c);
						std::lock_guard<std::mutex> guard(mutex);
						connecting.push_back(c);
					}
					
					enet_packet_destroy(event.packet);
					break;
				}
					
				case ENET_EVENT_TYPE_RECEIVE: {
					bool receiverFound = false;
					for(auto & c : active)
					{
						if(c->getPeer() == event.peer)
						{
							c->dispatch(event.packet);
							receiverFound = true;
							break;
						}
					}
			
					if(!receiverFound)
						enet_packet_destroy(event.packet);
					
					break;
				}
					
				case ENET_EVENT_TYPE_DISCONNECT: {
					for(auto c : active)
					{
						if(c->getPeer() == event.peer)
						{
							std::lock_guard<std::mutex> guard(mutex);
							disconnecting.push_back(c);
							active.erase(c);
							break;
						}
					}
					enet_packet_destroy(event.packet);
					break;
				}
			}
		}
	}
}
