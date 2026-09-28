/*
 * BattleMirrorServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleMirrorServer.h"

#include "BattleTextViewRenderer.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/networkPacks/NetPackVisitor.h"
#include "../../lib/texts/MetaString.h"

#include <array>
#include <boost/asio/post.hpp>

namespace
{
	constexpr battleTextView::RenderOptions mirrorRenderOptions{.ansi = true};
}

class BattleMirrorController::PackVisitor : public ICPackVisitor
{
public:
	explicit PackVisitor(BattleMirrorController & owner)
		: owner(owner)
	{
	}

	void visitBattleStart(BattleStart & pack) override
	{
		owner.current = pack.battleID;
		owner.logRing.clear();
		owner.renderAndSend();
	}

	void visitBattleLogMessage(BattleLogMessage & pack) override
	{
		owner.switchBattle(pack.battleID);
		for(const auto & line : pack.lines)
			owner.logRing.push_back(line.toString(LIBRARY->staticTexts()));
		owner.trimLogRing();
		owner.renderAndSend();
	}

	// terminal packs erase the battle while it is applied, so their frames come from pack fields alone
	void visitBattleEnded(BattleEnded & pack) override
	{
		owner.current = pack.battleID;
		if(owner.interested)
			owner.sendFrame(battleTextView::renderBattleSummary(pack.battleID, pack.victor, mirrorRenderOptions));
		owner.current.reset();
	}

	void visitBattleCancelled(BattleCancelled & pack) override
	{
		owner.current = pack.battleID;
		if(owner.interested)
			owner.sendFrame(battleTextView::renderBattleCancelled(pack.battleID, mirrorRenderOptions));
		owner.current.reset();
	}

	void visitBattleNextRound(BattleNextRound & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleSetActiveStack(BattleSetActiveStack & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleStackMoved(BattleStackMoved & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleUnitsChanged(BattleUnitsChanged & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleAttack(BattleAttack & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleSpellCast(BattleSpellCast & pack) override { owner.followBattle(pack.battleID); }
	void visitStacksInjured(StacksInjured & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleObstaclesChanged(BattleObstaclesChanged & pack) override { owner.followBattle(pack.battleID); }
	void visitCatapultAttack(CatapultAttack & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleSetStackProperty(BattleSetStackProperty & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleTriggerEffect(BattleTriggerEffect & pack) override { owner.followBattle(pack.battleID); }
	void visitBattleUpdateGateState(BattleUpdateGateState & pack) override { owner.followBattle(pack.battleID); }

private:
	BattleMirrorController & owner;
};

void BattleMirrorController::setSink(FrameSink newSink)
{
	sink = std::move(newSink);
}

void BattleMirrorController::setInterested(bool viewersPresent)
{
	interested = viewersPresent;
}

void BattleMirrorController::onPackApplied(CPackForClient & pack, const CGameState & state)
{
	gameState = &state;
	PackVisitor visitor(*this);
	pack.visit(visitor);
}

std::string BattleMirrorController::snapshotFrame() const
{
	return renderFrame();
}

bool BattleMirrorController::hasBattle() const
{
	return current.has_value();
}

void BattleMirrorController::reset()
{
	gameState = nullptr;
	current.reset();
	logRing.clear();
}

void BattleMirrorController::switchBattle(const BattleID & id)
{
	if(current != id)
	{
		current = id;
		logRing.clear();
	}
}

void BattleMirrorController::followBattle(const BattleID & id)
{
	switchBattle(id);
	renderAndSend();
}

void BattleMirrorController::trimLogRing()
{
	while(logRing.size() > logRingCapacity)
		logRing.pop_front();
}

void BattleMirrorController::renderAndSend()
{
	// fast path: with no viewer connected, packs cost no render work at all
	if(!interested)
		return;
	const std::string frame = renderFrame();
	if(!frame.empty())
		sendFrame(frame);
}

std::string BattleMirrorController::renderFrame() const
{
	if(!current || !gameState)
		return {};

	const BattleInfo * battle = gameState->getBattle(*current);
	if(!battle)
		return {};

	return battleTextView::renderBattleTextView(*battle, {logRing.begin(), logRing.end()}, mirrorRenderOptions);
}

void BattleMirrorController::sendFrame(const std::string & frame)
{
	if(interested && sink)
		sink(frame);
}

class BattleMirrorServer::Session : public std::enable_shared_from_this<BattleMirrorServer::Session>
{
public:
	Session(BattleMirrorServer & owner, boost::asio::ip::tcp::socket socket)
		: owner(owner)
		, socket(std::move(socket))
	{
	}

	void start()
	{
		std::string greeting = "VCMI battle telnet mirror - read-only\r\n";
		if(owner.controller.hasBattle())
			greeting += owner.controller.snapshotFrame();
		sendFrame(std::move(greeting));
		readDrain();
	}

	void sendFrame(std::string frame)
	{
		pendingFrame = std::move(frame);
		if(writing)
			return;
		writing = true;
		writeNext();
	}

	void close()
	{
		boost::system::error_code ec;
		socket.close(ec);
	}

private:
	void readDrain()
	{
		// real telnet clients emit IAC negotiation bytes on connect, so input is drained and discarded rather than treated as commands or a disconnect
		socket.async_read_some(boost::asio::buffer(readBuffer), [self = shared_from_this()](const boost::system::error_code & ec, size_t)
		{
			if(ec)
			{
				self->owner.dropSession(self);
				return;
			}
			self->readDrain();
		});
	}

	void writeNext()
	{
		activeFrame = std::move(pendingFrame);
		pendingFrame.clear();
		if(activeFrame.empty())
		{
			writing = false;
			return;
		}
		// only one async_write may be outstanding per socket — two would interleave their buffers and corrupt frames
		boost::asio::async_write(socket, boost::asio::buffer(activeFrame), [self = shared_from_this()](const boost::system::error_code & ec, size_t)
		{
			if(ec)
			{
				self->owner.dropSession(self);
				return;
			}
			self->writeNext();
		});
	}

	BattleMirrorServer & owner;
	boost::asio::ip::tcp::socket socket;
	std::string pendingFrame;
	std::string activeFrame;
	bool writing = false;
	std::array<char, 256> readBuffer;

	friend class BattleMirrorServer;
};

BattleMirrorServer::BattleMirrorServer(NetworkContext & context, const std::string & hostname, uint16_t port)
	: context(context)
	, hostname(hostname)
	, port(port)
	, acceptor(context)
{
	controller.setSink([this](std::string frame)
	{
		broadcast(std::move(frame));
	});
}

BattleMirrorServer::~BattleMirrorServer()
{
	// posted close never runs when the context was stopped first; this post-join pass is the safety net
	closeAllImpl();
}

void BattleMirrorServer::start()
{
	try
	{
		const auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(hostname), port);
		acceptor.open(endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(endpoint);
		acceptor.listen();
		logNetwork->info("Battle mirror listening at %s:%d", hostname, acceptor.local_endpoint().port());
		startAccept();
	}
	catch(const std::exception & e)
	{
		// leaving a half-open acceptor behind would leak its descriptor for the server's lifetime
		boost::system::error_code closeEc;
		if(acceptor.is_open())
			acceptor.close(closeEc);
		logNetwork->error("Battle mirror failed to start: %s", e.what());
	}
}

uint16_t BattleMirrorServer::listenPort() const
{
	boost::system::error_code ec;
	const auto endpoint = acceptor.local_endpoint(ec);
	if(ec)
		return 0;
	return endpoint.port();
}

void BattleMirrorServer::closeAll()
{
	// server shutdown may call this from a non-io thread while run() is still inside the context
	boost::asio::post(context, [this]()
	{
		closeAllImpl();
	});
}

// idempotent on purpose: it runs as the posted close and again in the destructor
void BattleMirrorServer::closeAllImpl()
{
	boost::system::error_code ec;
	if(acceptor.is_open())
		acceptor.close(ec);
	for(const auto & session : sessions)
		session->close();
	sessions.clear();
	controller.setInterested(false);
}

void BattleMirrorServer::onPackApplied(CPackForClient & pack, const CGameState & gameState)
{
	controller.onPackApplied(pack, gameState);
}

void BattleMirrorServer::reset()
{
	controller.reset();
}

void BattleMirrorServer::startAccept()
{
	auto session = std::make_shared<Session>(*this, boost::asio::ip::tcp::socket(context));
	acceptor.async_accept(session->socket, [this, session](const boost::system::error_code & ec)
	{
		onAccepted(session, ec);
	});
}

void BattleMirrorServer::onAccepted(const std::shared_ptr<Session> & session, const boost::system::error_code & ec)
{
	if(ec)
	{
		// aborted means teardown closed the acceptor; any other failure is transient and must re-arm
		// (accept completions are event-driven, so an immediate restart cannot busy-loop)
		if(ec != boost::asio::error::operation_aborted)
		{
			logNetwork->error("Battle mirror accept failed: %s", ec.message());
			startAccept();
		}
		return;
	}

	if(sessions.size() >= maxSessions)
	{
		if(!sessionCapLogged)
		{
			sessionCapLogged = true;
			logNetwork->error("Battle mirror full (%d viewers), rejecting further connections", static_cast<int>(maxSessions));
		}
		const std::string busyLine = "battle mirror busy, try again later\r\n";
		boost::system::error_code writeEc;
		boost::asio::write(session->socket, boost::asio::buffer(busyLine), writeEc);
		session->close();
		startAccept();
		return;
	}

	sessions.insert(session);
	if(sessions.size() == 1)
		controller.setInterested(true);
	try
	{
		session->start();
	}
	catch(const std::exception & e)
	{
		logNetwork->error("Battle mirror error: %s", e.what());
		dropSession(session);
	}
	startAccept();
}

void BattleMirrorServer::dropSession(const std::shared_ptr<Session> & session)
{
	session->close();
	sessions.erase(session);
	sessionCapLogged = false;
	if(sessions.empty())
		controller.setInterested(false);
}

void BattleMirrorServer::broadcast(std::string frame)
{
	for(const auto & session : sessions)
		session->sendFrame(frame);
}
