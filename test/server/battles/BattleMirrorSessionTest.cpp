/*
 * BattleMirrorSessionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../server/battles/BattleMirrorServer.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/texts/MetaString.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <poll.h>
#include <sys/socket.h>
#endif

namespace
{
	constexpr int pumpBound = 1000;

	size_t countFrames(const std::string & received)
	{
		size_t count = 0;
		for(size_t at = received.find("battle #"); at != std::string::npos; at = received.find("battle #", at + 1))
			++count;
		return count;
	}
}

class BattleMirrorSessionTest : public BattleTestFixture
{
public:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
	}

protected:
	// everything holding an asio handle must die before the io_context it belongs to
	boost::asio::io_context io;
	std::unique_ptr<BattleMirrorServer> mirror;
	boost::asio::ip::tcp::socket client{io};

	void startMirror()
	{
		mirror = std::make_unique<BattleMirrorServer>(io, "127.0.0.1", 0);
		mirror->start();
		ASSERT_NE(mirror->listenPort(), 0u);
	}

	// io_context::poll() permanently stops the context once invoked with no outstanding work,
	// so every pump must revive it first or later handlers would never dispatch
	void pump()
	{
		io.restart();
		while(io.poll()) {}
	}

	boost::system::error_code connectClient(boost::asio::ip::tcp::socket & socket)
	{
		const auto done = std::make_shared<bool>(false);
		const auto outcome = std::make_shared<boost::system::error_code>();
		socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), mirror->listenPort()),
			[done, outcome](const boost::system::error_code & ec)
			{
				*outcome = ec;
				*done = true;
			});
		pump();
		if(!*done)
			ADD_FAILURE() << "client connect never completed";
		return *outcome;
	}

	// reads until the predicate holds for everything received so far; a read that completes with
	// eof or reset returns that error code instead
	boost::system::error_code readUntil(boost::asio::ip::tcp::socket & socket, std::string & received, const std::function<bool(const std::string &)> & predicate)
	{
		if(predicate(received))
			return {};

		for(int attempt = 0; attempt < pumpBound; ++attempt)
		{
			const auto done = std::make_shared<bool>(false);
			const auto outcome = std::make_shared<boost::system::error_code>();
			auto buffer = boost::asio::dynamic_buffer(received);
			socket.async_read_some(buffer.prepare(512),
				[done, outcome, buffer](const boost::system::error_code & ec, std::size_t bytes) mutable
				{
					buffer.commit(bytes);
					*outcome = ec;
					*done = true;
				});
			pump();
			if(!*done)
			{
				ADD_FAILURE() << "expected bytes never arrived: io quiescent with a client read pending";
				return {};
			}
			if(*outcome)
				return *outcome;
			if(predicate(received))
				return {};
		}
		ADD_FAILURE() << "read predicate unmet within the pump bound";
		return {};
	}

	void primeBattle()
	{
		BattleStart pack;
		pack.battleID = battle()->battleID;
		mirror->onPackApplied(pack, *server.gameState);
	}

	void applyLog(const std::string & text)
	{
		BattleLogMessage pack;
		pack.battleID = battle()->battleID;

		MetaString line;
		line.appendRawString(text);
		pack.lines.push_back(std::move(line));
		mirror->onPackApplied(pack, *server.gameState);
	}

	void shutdownMirror()
	{
		boost::system::error_code ec;
		client.close(ec);
		if(mirror)
		{
			mirror->closeAll();
			pump();
			mirror.reset();
		}
	}
};

TEST_F(BattleMirrorSessionTest, ConnectMidBattleGetsSnapshot)
{
	startMirror();
	primeBattle();

	std::string received;
	ASSERT_FALSE(connectClient(client));
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s)
	{
		return s.find("VCMI battle telnet mirror") != std::string::npos
			&& s.find("battle #0") != std::string::npos
			&& s.find("00|") != std::string::npos;
	}));
	EXPECT_NE(received.find("battle #0"), std::string::npos);
	EXPECT_NE(received.find("00|"), std::string::npos);

	shutdownMirror();
}

TEST_F(BattleMirrorSessionTest, BurstCoalescesToLatestFrame)
{
	startMirror();

	std::string received;
	ASSERT_FALSE(connectClient(client));
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("VCMI battle telnet mirror") != std::string::npos; }));

	applyLog("t1");
	applyLog("t2");
	applyLog("t3");
	pump();

	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("| t3") != std::string::npos; }));
	EXPECT_NE(received.find("| t1"), std::string::npos);
	// every frame re-renders the whole log ring, so the dropped t2 frame is observable only as a missing frame
	EXPECT_EQ(countFrames(received), 2u) << "depth-1 coalescing must keep exactly the t1 and t3 frames on the wire";

	applyLog("t4");
	pump();
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("| t4") != std::string::npos; }));
	EXPECT_EQ(countFrames(received), 3u) << "connection must survive the burst uncorrupted";

	shutdownMirror();
}

TEST_F(BattleMirrorSessionTest, IacBytesDoNotKillSession)
{
	startMirror();

	std::string received;
	ASSERT_FALSE(connectClient(client));
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("VCMI battle telnet mirror") != std::string::npos; }));

	const auto done = std::make_shared<bool>(false);
	const auto outcome = std::make_shared<boost::system::error_code>();
	const char iacBytes[] = {static_cast<char>(0xFF), static_cast<char>(0xFD), static_cast<char>(0x2F)};
	client.async_write_some(boost::asio::buffer(iacBytes, sizeof(iacBytes)),
		[done, outcome](const boost::system::error_code & ec, std::size_t)
		{
			*outcome = ec;
			*done = true;
		});
	pump();
	EXPECT_TRUE(*done);
	EXPECT_FALSE(*outcome);

	applyLog("post-iac");
	pump();
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("| post-iac") != std::string::npos; }));

	shutdownMirror();
}

TEST_F(BattleMirrorSessionTest, CloseAllGivesOrderlyEof)
{
	startMirror();

	std::string received;
	ASSERT_FALSE(connectClient(client));
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("read-only\r\n") != std::string::npos; }));

	mirror->closeAll();
	pump();

	const auto ec = readUntil(client, received, [](const std::string &) { return false; });
	EXPECT_EQ(ec, boost::asio::error::eof);

	shutdownMirror();
}

TEST_F(BattleMirrorSessionTest, AbruptClientCloseThenReconnect)
{
	startMirror();

	std::string received;
	ASSERT_FALSE(connectClient(client));
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("VCMI battle telnet mirror") != std::string::npos; }));

	boost::system::error_code ec;
	client.close(ec);
	pump();

	boost::asio::ip::tcp::socket fresh(io);
	ASSERT_FALSE(connectClient(fresh));
	std::string freshReceived;
	EXPECT_FALSE(readUntil(fresh, freshReceived, [](const std::string & s) { return s.find("VCMI battle telnet mirror") != std::string::npos; }));

	fresh.close(ec);
	shutdownMirror();
}

TEST_F(BattleMirrorSessionTest, SessionCapRejectsWithBusyLine)
{
	startMirror();
	primeBattle();

	constexpr size_t viewerCount = 16;
	std::vector<boost::asio::ip::tcp::socket> viewers;
	std::vector<std::string> received;
	for(size_t i = 0; i < viewerCount; ++i)
	{
		viewers.emplace_back(io);
		received.emplace_back();
		ASSERT_FALSE(connectClient(viewers.back())) << "viewer " << i;
		EXPECT_FALSE(readUntil(viewers.back(), received.back(), [](const std::string & s) { return s.find("read-only\r\n") != std::string::npos; })) << "viewer " << i;
	}

	boost::asio::ip::tcp::socket rejected(io);
	ASSERT_FALSE(connectClient(rejected));

	std::string rejectedReceived;
	EXPECT_FALSE(readUntil(rejected, rejectedReceived, [](const std::string & s) { return s.find("battle mirror busy, try again later\r\n") != std::string::npos || s.find("read-only\r\n") != std::string::npos; }));
	EXPECT_NE(rejectedReceived.find("battle mirror busy, try again later\r\n"), std::string::npos) << "overflow viewer must get the busy line, not a session";

	const auto ec = readUntil(rejected, rejectedReceived, [](const std::string &) { return false; });
	EXPECT_EQ(ec, boost::asio::error::eof);

	// the rejected overflow must not disturb the established sessions
	applyLog("cap-followup");
	pump();
	EXPECT_FALSE(readUntil(viewers.front(), received.front(), [](const std::string & s) { return s.find("| cap-followup") != std::string::npos; }));

	boost::system::error_code ignored;
	for(auto & viewer : viewers)
		viewer.close(ignored);
	rejected.close(ignored);
	shutdownMirror();
}

TEST_F(BattleMirrorSessionTest, TeardownFromNonIoThread)
{
	startMirror();

	std::string received;
	ASSERT_FALSE(connectClient(client));
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("read-only\r\n") != std::string::npos; }));

	std::promise<boost::system::error_code> readOutcome;
	auto readFuture = readOutcome.get_future();
	auto buffer = boost::asio::dynamic_buffer(received);
	client.async_read_some(buffer.prepare(512),
		[promise = std::move(readOutcome), buffer](const boost::system::error_code & ec, std::size_t bytes) mutable
		{
			buffer.commit(bytes);
			promise.set_value(ec);
		});

	std::thread ioThread([&] { io.run(); });
	mirror->closeAll();

	// the assertions below pin the post-fix teardown semantics; the data-race class itself is only observable
	// under TSan — the linux-gcc-tsan preset and its scoped CI job (test-tsan) now run these suites under TSan
	// in CI, while regular CI still runs this canary deterministically on a plain build; the sibling
	// CloseAllBodyRunsOnIoThreadNotCaller pins the io-thread marshalling of the close body deterministically
	// bounded: a regression that never closes the socket must fail fast, not hang until CI timeout
	if(readFuture.wait_for(std::chrono::seconds(30)) != std::future_status::ready)
	{
		io.stop();
		ioThread.join();
		FAIL() << "closeAll did not deliver EOF within 30 s";
		return;
	}
	EXPECT_EQ(readFuture.get(), boost::asio::error::eof);
	ioThread.join();
	mirror.reset();

	boost::system::error_code ignored;
	client.close(ignored);
	io.stop();
}

TEST_F(BattleMirrorSessionTest, CloseAllBodyRunsOnIoThreadNotCaller)
{
#ifdef _WIN32
	GTEST_SKIP() << "POSIX poll-based marshalling tripwire";
#else
	startMirror();

	std::string received;
	ASSERT_FALSE(connectClient(client));
	EXPECT_FALSE(readUntil(client, received, [](const std::string & s) { return s.find("read-only\r\n") != std::string::npos; }));

	std::promise<void> release;
	auto releaseFuture = release.get_future();
	std::atomic<bool> gateEntered{false};
	boost::asio::post(io, [&]
	{
		gateEntered = true;
		releaseFuture.wait_for(std::chrono::seconds(5));
	});

	std::thread ioThread([&] { io.run(); });
	const auto gateDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while(!gateEntered.load() && std::chrono::steady_clock::now() < gateDeadline)
		std::this_thread::yield();
	if(!gateEntered.load())
	{
		release.set_value(); // release the gate handler if it is parked or still queued
		io.stop();
		ioThread.join();
		FAIL() << "gate handler never ran on the io thread";
		return;
	}

	mirror->closeAll();
	// the gate handler and the closeAll body are both posted from this thread, and asio runs same-thread posts FIFO,
	// so the body cannot run while the only run() thread is parked inside the gate
	pollfd watched{};
	watched.fd = client.native_handle();
	watched.events = POLLIN;
	int polled = ::poll(&watched, 1, 200);
	while(polled == -1 && errno == EINTR)
		polled = ::poll(&watched, 1, 200);
	if(polled == -1)
	{
		release.set_value();
		io.stop();
		ioThread.join();
		FAIL() << "poll on the mirror socket failed: " << strerror(errno);
		return;
	}
	if(polled == 1 && (watched.revents & POLLIN))
	{
		char probe = 0;
		if(::recv(watched.fd, &probe, 1, MSG_PEEK) == 0)
		{
			release.set_value();
			io.stop();
			ioThread.join();
			FAIL() << "closeAll body executed out-of-band (EOF observable while the io thread is parked)";
			return;
		}
	}
	EXPECT_EQ(polled, 0) << "the mirror socket must stay quiet while the only run() thread is parked";

	std::promise<boost::system::error_code> readOutcome;
	auto readFuture = readOutcome.get_future();
	auto buffer = boost::asio::dynamic_buffer(received);
	client.async_read_some(buffer.prepare(512),
		[promise = std::move(readOutcome), buffer](const boost::system::error_code & ec, std::size_t bytes) mutable
		{
			buffer.commit(bytes);
			promise.set_value(ec);
		});
	// armed before the gate opens: the pending read keeps run() alive, so the EOF can only come from the posted body
	release.set_value();

	if(readFuture.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
	{
		io.stop();
		ioThread.join();
		FAIL() << "posted closeAll body never delivered EOF";
		return;
	}
	EXPECT_EQ(readFuture.get(), boost::asio::error::eof);

	ioThread.join();
	mirror.reset();

	boost::system::error_code teardownEc;
	client.close(teardownEc);
	io.stop();
#endif
}
