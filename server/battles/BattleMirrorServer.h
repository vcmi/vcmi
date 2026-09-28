/*
 * BattleMirrorServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/network/NetworkInterface.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>

struct CPackForClient;
class CGameState;

/// Headless half of the telnet battle mirror: derives text frames for the viewers from netpacks
/// applied to the game state. Reached only from the network thread that runs the mirror's
/// io_context — excepting the pre-run() phase (the constructor wires the sink before the io
/// thread exists) and teardown (see BattleMirrorServer) — so no locking.
class BattleMirrorController
{
public:
	using FrameSink = std::function<void(std::string)>;

	/// Replaces the sink that live frames are pushed to.
	void setSink(FrameSink sink);
	/// Fast-path gate: with no viewers present no frame is rendered or sent.
	void setInterested(bool viewersPresent);
	void onPackApplied(CPackForClient & pack, const CGameState & gameState);
	/// Full frame on demand, for a viewer that connects mid-battle.
	std::string snapshotFrame() const;
	bool hasBattle() const;
	/// Clears all mirrored state; used when the game state it points at is about to be dropped.
	void reset();

private:
	class PackVisitor;

	static constexpr size_t logRingCapacity = 8;

	void switchBattle(const BattleID & id);
	void followBattle(const BattleID & id);
	void trimLogRing();
	void renderAndSend();
	std::string renderFrame() const;
	void sendFrame(const std::string & frame);

	FrameSink sink;
	bool interested = false;
	std::optional<BattleID> current;
	std::deque<std::string> logRing;
	const CGameState * gameState = nullptr;
};

/// TCP half of the telnet battle mirror: accepts telnet viewer connections and pushes
/// controller frames to every connected socket.
/// Every entry point runs on the single network thread (the io_context is blocking-run by
/// INetworkHandler::run), so no locking — two exceptions: the pre-run() phase (construction,
/// start() and listenPort() run on the spawning thread, before the io thread exists) and
/// teardown: closeAll() may be called from any thread and marshals its body onto the io thread
/// via asio::post, and the destructor repeats that body idempotently for the case where the
/// posted closure never ran because the context was stopped first (safe there: it executes
/// post-join).
class BattleMirrorServer
{
public:
	BattleMirrorServer(NetworkContext & context, const std::string & hostname, uint16_t port);
	~BattleMirrorServer();

	/// Binds and starts accepting; on failure logs and leaves the listener disabled.
	void start();
	uint16_t listenPort() const;
	/// Closes the acceptor and every viewer socket; called on server shutdown. Thread-safe:
	/// the close work is posted onto the io thread.
	void closeAll();
	void onPackApplied(CPackForClient & pack, const CGameState & gameState);
	/// Clears all mirrored state; used when the game state is about to be dropped.
	void reset();

private:
	class Session;

	static constexpr size_t maxSessions = 16;

	void startAccept();
	void onAccepted(const std::shared_ptr<Session> & session, const boost::system::error_code & ec);
	void dropSession(const std::shared_ptr<Session> & session);
	void broadcast(std::string frame);
	void closeAllImpl();

	NetworkContext & context;
	std::string hostname;
	uint16_t port;
	boost::asio::ip::tcp::acceptor acceptor;
	std::set<std::shared_ptr<Session>> sessions;
	bool sessionCapLogged = false;
	BattleMirrorController controller;
};
