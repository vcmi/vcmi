//
// Created by caiiiycuk on 20.11.24.
//
#include "StdInc.h"
#include "../../network/NetworkHandler.h"

#include "CThreadHelper.h"

VCMI_LIB_NAMESPACE_BEGIN

enum LoopNetworkCommandType {
    IDLE,
    SERVER_CONNECT,
    CLIENT_CONNECT,
    SERVER_TO_CLIENT,
    CLIENT_TO_SERVER,
};

struct LoopNetworkCommand {
    LoopNetworkCommandType type = IDLE;
    std::vector<std::byte> message = {};
};

class LoopServerConnection;
class LoopClientConnection;

struct LoopNetworkTimeout {
    INetworkTimerListener &listener;
    std::chrono::milliseconds executeAt;
};

struct LoopNetwork {
    boost::mutex loopMutex;
    boost::atomic_uint8_t connected = 2;
    std::shared_ptr<LoopClientConnection> client;
    std::shared_ptr<LoopServerConnection> server;
    std::list<LoopNetworkCommand> commands;
    std::list<LoopNetworkTimeout> timeouts;
};


namespace {
    std::shared_ptr<LoopNetwork> loop = nullptr;

    std::chrono::milliseconds now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    }

    void pushLoopCommand(std::shared_ptr<LoopNetwork> &loop, LoopNetworkCommandType type,
                         const std::vector<std::byte> &message) {
        boost::mutex::scoped_lock lock(loop->loopMutex);
        loop->commands.push_back({
            type, message
        });
    }
}

class LoopClientConnection final : public INetworkConnection,
                                   public std::enable_shared_from_this<LoopClientConnection> {
    std::shared_ptr<LoopNetwork> loop;

public:
    INetworkClientListener &listener;

    LoopClientConnection(std::shared_ptr<LoopNetwork> &loop,
                         INetworkClientListener &listener) : loop(loop), listener(listener) {
    }

    void sendPacket(const std::vector<std::byte> &message) override {
        // logNetwork->info(std::string("(server -> client) packet:") + std::to_string(message.size()));
        pushLoopCommand(loop, SERVER_TO_CLIENT, message);
    }

    void setAsyncWritesEnabled(bool on) override {
    }

    void close() override {
        loop->connected -= 1;
        loop->client.reset();
    }
};

class LoopServerConnection final : public INetworkConnection,
                                   public std::enable_shared_from_this<LoopServerConnection> {
    std::shared_ptr<LoopNetwork> loop;

public:
    INetworkServerListener &listener;

    LoopServerConnection(std::shared_ptr<LoopNetwork> &loop,
                         INetworkServerListener &listener) : loop(loop), listener(listener) {
    }

    void sendPacket(const std::vector<std::byte> &message) override {
        // logNetwork->info(std::string("(client -> server) packet:") + std::to_string(message.size()));
        pushLoopCommand(loop, CLIENT_TO_SERVER, message);
    }

    void setAsyncWritesEnabled(bool on) override {
    }

    void close() override {
        loop->connected -= 1;
        loop->server.reset();
    }
};

class LoopNetworkServer : public INetworkConnectionListener, public INetworkServer {
    std::shared_ptr<LoopNetwork> loop;

public:
    LoopNetworkServer(std::shared_ptr<LoopNetwork> &loop): loop(loop) {
    }

    ~LoopNetworkServer() {
        loop->connected = 0;
    }

    uint16_t start(uint16_t port) override {
        logNetwork->info("HTML5 loop server registered on " + std::to_string(port));
        return port;
    }

    void onDisconnected(const std::shared_ptr<INetworkConnection> &connection,
                        const std::string &errorMessage) override {
        abort();
    }

    void onPacketReceived(const std::shared_ptr<INetworkConnection> &connection,
                          const std::vector<std::byte> &message) override {
        abort();
    }
};

class LoopNetworkHandler : public INetworkHandler {
public:
    LoopNetworkHandler() = default;

    std::unique_ptr<INetworkServer> createServerTCP(INetworkServerListener &listener) override {
        logNetwork->info("Create a new server loop");
        loop = std::make_shared<LoopNetwork>();
        loop->server = std::make_shared<LoopServerConnection>(loop, listener);
        return std::make_unique<LoopNetworkServer>(loop);
    }

    void connectToRemote(INetworkClientListener &listener, const std::string &host, uint16_t port) override {
        if (loop && loop->connected > 0) {
            logNetwork->info("Client attached to the loop");
            loop->client = std::make_shared<LoopClientConnection>(loop, listener);
            pushLoopCommand(loop, SERVER_CONNECT, {});
            pushLoopCommand(loop, CLIENT_CONNECT, {});
        } else {
            logNetwork->error("Trying to connect to loop network that does not exitsts");
            assert(false);
        }
    }

    void createTimer(INetworkTimerListener &listener, std::chrono::milliseconds duration) override {
        if (loop && loop->connected > 0) {
            boost::mutex::scoped_lock lock(loop->loopMutex);
            loop->timeouts.push_back({listener, now() + duration});
        } else {
            logNetwork->warn("Trying to create a new timer in dead loop");
        }
    }

    void run() override {
    }

    void stop() override {
    }
};

std::unique_ptr<INetworkHandler> INetworkHandler::createHandler() {
    return std::make_unique<LoopNetworkHandler>();
}

namespace {
    boost::thread loopThread([]() {
        setThreadName("HTTP5 loop server");
        // There is no way to exit from WebVersion, so don't care about exiting
        while (true) {
            std::shared_ptr<LoopNetwork> loop = ::loop;
            if (loop == nullptr || loop->connected == 0) {
                usleep(4000);
                continue;
            }

            logNetwork->info("HTML5 loop server actually started");
            LoopNetworkCommand command;
            while (loop->connected > 0) {
                command.type = IDLE;
                std::vector<INetworkTimerListener *> timersToRun; {
                    boost::mutex::scoped_lock lock(loop->loopMutex);
                    if (!loop->commands.empty()) {
                        command = std::move(loop->commands.front());
                        loop->commands.pop_front();
                    }

                    auto nowMs = now();
                    auto it = loop->timeouts.begin();
                    while (it != loop->timeouts.end()) {
                        if (it->executeAt >= nowMs) {
                            timersToRun.push_back(&it->listener);
                            it = loop->timeouts.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                for (const auto timer: timersToRun) {
                    timer->onTimer();
                }

                switch (command.type) {
                    case IDLE:
                        usleep(4000);
                        break;
                    case SERVER_CONNECT:
                        loop->server->listener.onNewConnection(loop->client);
                        break;
                    case CLIENT_CONNECT:
                        loop->client->listener.onConnectionEstablished(loop->server);
                        break;
                    case SERVER_TO_CLIENT:
                        if (loop->connected > 0 && loop->client && loop->server) {
                            loop->client->listener.onPacketReceived(loop->server->shared_from_this(), command.message);
                        }
                        break;
                    case CLIENT_TO_SERVER:
                        if (loop->connected > 0 && loop->client && loop->server) {
                            loop->server->listener.onPacketReceived(loop->client->shared_from_this(), command.message);
                        }
                        break;
                }
            }

            loop->server.reset();
            loop->client.reset();
        }
    });
}

VCMI_LIB_NAMESPACE_END
