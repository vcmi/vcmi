/*
 * GameplayReplayer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameplayReplayer.h"

#include "ReplayAbortOverlay.h"
#include "ReplayPackFilter.h"

#include "../CPlayerInterface.h"
#include "../CServerHandler.h"
#include "../Client.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../battle/BattleInterface.h"
#include "../gui/CIntObject.h"
#include "../gui/WindowHandler.h"
#include "../mapView/mapHandler.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/gameState/ReplayLog.h"
#include "../../lib/networkPacks/NetPacksBase.h"

namespace
{
	/// Everything of the live session that has to step aside while a replay is on screen
	struct LiveSessionBackup
	{
		ClientSession session;
		std::unique_ptr<CMapHandler> mapHandler;
		std::vector<std::shared_ptr<IShowActivatable>> windows;
		std::shared_ptr<AdventureMapInterface> adventureMap;
		std::shared_ptr<BattleInterface> battleInterface;
		CPlayerInterface * playerInterface = nullptr;
	};

	CPackForClient & asClientPack(CPack & pack)
	{
		auto * clientPack = dynamic_cast<CPackForClient *>(&pack);

		if(clientPack == nullptr)
			throw std::runtime_error("Recorded data contains a pack that is not meant for the client!");

		return *clientPack;
	}
}

GameplayReplayer::GameplayReplayer() = default;

GameplayReplayer::~GameplayReplayer()
{
	requestStop();
	waitForFinish();

	if(worker.joinable())
		worker.join();
}

bool GameplayReplayer::isActive() const
{
	std::scoped_lock lock(stateMutex);
	return active;
}

void GameplayReplayer::requestStop()
{
	stopRequested = true;
	paused = false;
}

void GameplayReplayer::setPaused(bool value)
{
	paused = value;
}

void GameplayReplayer::waitForFinish() const
{
	std::unique_lock lock(stateMutex);
	stateChanged.wait(lock, [this]() { return !active; });
}

void GameplayReplayer::markFinished()
{
	{
		std::scoped_lock lock(stateMutex);
		active = false;
	}
	stateChanged.notify_all();
}

void GameplayReplayer::start(ReplaySequence sequence, PlayerColor observer, const Options & options)
{
	{
		std::scoped_lock lock(stateMutex);
		if(active)
			return;

		active = true;
	}

	if(worker.joinable())
		worker.join();

	stopRequested = false;
	paused = false;
	worker = std::thread(&GameplayReplayer::run, this, std::move(sequence), observer, options);
}

void GameplayReplayer::run(ReplaySequence sequence, PlayerColor observer, Options options)
{
	setThreadName("replay");

	CClient & client = *GAME->server().client;
	LiveSessionBackup backup;
	std::shared_ptr<CGameState> replayState;
	bool sessionInstalled = false;

	try
	{
		// 1. rebuild the state the replayed turn started from. No UI is attached yet, so
		// fast-forwarding through earlier turns costs nothing but CPU time.
		replayState = std::make_shared<CGameState>();
		replayState->preInit(LIBRARY);
		replayState->loadFromMemory(sequence.snapshot);

		for(const auto & data : sequence.fastForwardPacks)
		{
			auto pack = ReplayPackSerializer::read(data, replayState.get());
			replayState->apply(asClientPack(*pack));
		}

		// a recording of an earlier game may not know the color we are playing right now
		if(!replayState->players.count(observer))
			observer = sequence.replayedPlayer;

		if(!replayState->players.count(observer))
			throw std::runtime_error("Recorded game has no player to watch it with!");

		// 2. put the live session aside and let the client operate on the replayed state instead
		{
			std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

			backup.playerInterface = GAME->interface();
			backup.adventureMap = adventureInt;
			const int3 previousViewCenter = adventureInt ? adventureInt->getMapViewCenter() : int3();
			backup.battleInterface = CPlayerInterface::battleInt;
			backup.windows = ENGINE->windows().detachAll();

			ClientSession replaySession;
			replaySession.gamestate = replayState;
			backup.session = client.swapSession(std::move(replaySession));
			sessionInstalled = true;
			client.observerMode = true;

			CPlayerInterface::battleInt.reset();
			adventureInt.reset();

			backup.mapHandler = GAME->swapMapInstance(std::make_unique<CMapHandler>(&replayState->getMap()));

			client.installObserverInterface(observer);

			ENGINE->windows().pushWindow(adventureInt);

			// isHuman=false is deliberate: only that state blocks input on a throw-away game
			adventureInt->onEnemyTurnStarted(sequence.replayedPlayer, false);
			adventureInt->onMapTilesChanged(std::nullopt);

			// start off where the player was looking, instead of at the top left corner of the map
			if(replayState->isInTheMap(previousViewCenter))
				adventureInt->centerOnTile(previousViewCenter);

			// drawn outside of the window stack, so it stays reachable even during a combat
			ENGINE->windows().setOverlay(std::make_shared<ReplayAbortOverlay>(
				[this](){ requestStop(); },
				[this](bool value){ setPaused(value); }));
		}

		// 3. play the recorded turn back, one pack at a time
		for(const auto & data : sequence.replayPacks)
		{
			// waiting happens without the interface mutex, so a paused replay stays responsive
			while(paused && !stopRequested)
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

			if(stopRequested)
				break;

			{
				std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

				auto pack = ReplayPackSerializer::read(data, replayState.get());
				CPackForClient & clientPack = asClientPack(*pack);

				switch(ReplayPackFilter::classify(clientPack))
				{
					case EReplayPackKind::INTERACTIVE:
						// dialogs, queries and session bookkeeping are applied, but never shown
						client.applyPackSilently(clientPack);
						break;
					case EReplayPackKind::BATTLE:
						if(options.showBattles)
							client.handlePack(clientPack);
						else
							client.applyPackSilently(clientPack);
						break;
					case EReplayPackKind::REGULAR:
						client.handlePack(clientPack);
						break;
				}
			}

			// the mutex is released between packs so that the engine can draw what just happened
			std::this_thread::yield();
		}
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Replay failed: %s", e.what());
	}

	// 4. put the live session back. This has to happen even if the replay threw, otherwise the
	// player would be left looking at a game that no longer exists.
	if(sessionInstalled)
	{
		std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

		try
		{
			// windows go first, so that a battle window is gone before the interface it points to
			ENGINE->windows().setOverlay(nullptr);
			ENGINE->windows().clear();
			CPlayerInterface::battleInt.reset();

			// dropping the replay session destroys its interfaces and callbacks
			ClientSession replaySession = client.swapSession(std::move(backup.session));
			if(replaySession.gamestate)
				replaySession.gamestate->currentBattles.clear();
			replaySession = ClientSession();

			GAME->swapMapInstance(std::move(backup.mapHandler));

			adventureInt = backup.adventureMap;
			CPlayerInterface::battleInt = backup.battleInterface;
			GAME->setInterfaceInstance(backup.playerInterface);

			ENGINE->windows().attachAll(std::move(backup.windows));
			ENGINE->windows().totalRedraw();
		}
		catch(const std::exception & e)
		{
			logGlobal->error("Failed to restore the game after a replay: %s", e.what());
		}

		client.observerMode = false;
	}

	replayState.reset();
	markFinished();
}


namespace
{
	/// index of the newest chapter at or before `chapter` that still carries a snapshot
	size_t findAnchorChapter(const std::vector<ReplayChapter> & chapters, size_t chapter)
	{
		for(size_t i = chapter + 1; i-- > 0;)
			if(!chapters[i].snapshot.empty())
				return i;

		throw std::runtime_error("Recording has no gamestate to start this replay from!");
	}
}

std::vector<ReplayTurnOption> ReplayPlanner::availableTurns(const ReplayLog & log)
{
	std::vector<ReplayTurnOption> result;
	const auto & chapters = log.getChapters();

	for(size_t chapterIndex = 0; chapterIndex < chapters.size(); ++chapterIndex)
	{
		// a turn can only be shown if some earlier chapter still has a gamestate to start from
		bool reachable = false;
		for(size_t i = chapterIndex + 1; i-- > 0;)
			if(!chapters[i].snapshot.empty())
				reachable = true;

		if(!reachable)
			continue;

		for(size_t turnIndex = 0; turnIndex < chapters[chapterIndex].turns.size(); ++turnIndex)
		{
			const auto & turn = chapters[chapterIndex].turns[turnIndex];
			result.push_back({turn.player, turn.day, false, chapterIndex, turnIndex});
		}
	}

	if(!result.empty())
		result.back().ongoing = true;

	return result;
}

ReplaySequence ReplayPlanner::prepareTurn(const ReplayLog & log, const ReplayTurnOption & option)
{
	const auto & chapters = log.getChapters();

	if(option.chapter >= chapters.size() || option.turn >= chapters[option.chapter].turns.size())
		throw std::runtime_error("Requested replay of a turn that is no longer available!");

	const auto & chapter = chapters[option.chapter];
	const size_t anchor = findAnchorChapter(chapters, option.chapter);
	const size_t firstPack = chapter.turns[option.turn].firstPack;
	const size_t lastPack = option.turn + 1 < chapter.turns.size()
		? chapter.turns[option.turn + 1].firstPack
		: chapter.packs.size();

	ReplaySequence result;
	result.replayedPlayer = chapter.turns[option.turn].player;
	result.day = chapter.turns[option.turn].day;
	result.snapshot = chapters[anchor].snapshot;

	// everything between the anchor and the replayed turn is applied without any visuals
	for(size_t i = anchor; i < option.chapter; ++i)
		result.fastForwardPacks.insert(result.fastForwardPacks.end(), chapters[i].packs.begin(), chapters[i].packs.end());

	result.fastForwardPacks.insert(result.fastForwardPacks.end(), chapter.packs.begin(), chapter.packs.begin() + firstPack);
	result.replayPacks.assign(chapter.packs.begin() + firstPack, chapter.packs.begin() + lastPack);

	return result;
}

ReplaySequence ReplayPlanner::prepareEntireGame(const ReplayLog & log)
{
	if(!log.canReplayEntireGame())
		throw std::runtime_error("This game was not recorded from its beginning!");

	const auto & chapters = log.getChapters();

	ReplaySequence result;
	result.snapshot = chapters.front().snapshot;

	if(!chapters.front().turns.empty())
	{
		result.replayedPlayer = chapters.front().turns.front().player;
		result.day = chapters.front().turns.front().day;
	}

	for(const auto & chapter : chapters)
		result.replayPacks.insert(result.replayPacks.end(), chapter.packs.begin(), chapter.packs.end());

	return result;
}
