/*
 * GameplayReplayer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"

class ReplayLog;

/// A single turn that the player may ask to see again
struct ReplayTurnOption
{
	PlayerColor player;
	uint32_t day = 0;
	bool ongoing = false;

	size_t chapter = 0;
	size_t turn = 0;
};

/// Everything that is needed to play a stretch of recorded netpacks back
struct ReplaySequence
{
	PlayerColor replayedPlayer;
	uint32_t day = 0;

	std::vector<std::byte> snapshot;

	/// packs between the snapshot and the beginning of the replayed part - applied without visuals
	std::vector<std::vector<std::byte>> fastForwardPacks;

	/// packs that are actually shown
	std::vector<std::vector<std::byte>> replayPacks;
};

/// Turns the recording stored in the gamestate into something that can be played back
namespace ReplayPlanner
{
	/// Turns that can still be watched, oldest first
	std::vector<ReplayTurnOption> availableTurns(const ReplayLog & log);

	ReplaySequence prepareTurn(const ReplayLog & log, const ReplayTurnOption & option);

	/// Only possible when the whole game was recorded
	ReplaySequence prepareEntireGame(const ReplayLog & log);
}

/// Plays a recorded sequence of netpacks back on a throw-away copy of the game.
///
/// The live gamestate is never modified: replaying installs a second, temporary session
/// (gamestate, callbacks, human interface, map handler, window stack) into CClient and puts the
/// original one back once the replay is over. Nothing is ever sent to the server, and every pack
/// that would ask the player for input is applied without showing anything.
class GameplayReplayer final : boost::noncopyable
{
public:
	struct Options
	{
		/// if false, combats are resolved instantly instead of being shown
		bool showBattles = true;
	};

	GameplayReplayer();
	~GameplayReplayer();

	/// True between the moment a replay was started and the moment the live session is back
	bool isActive() const;

	/// Starts a replay on a background thread. Does nothing if a replay is already running.
	void start(ReplaySequence sequence, PlayerColor observer, const Options & options);

	/// Asks a running replay to stop after the pack that is currently being shown
	void requestStop();

	/// Holds the replay on the current pack, or lets it continue
	void setPaused(bool value);

	/// Blocks until no replay is running anymore. Safe to call from any thread.
	void waitForFinish() const;

private:
	std::thread worker;

	mutable std::mutex stateMutex;
	mutable std::condition_variable stateChanged;
	bool active = false;

	std::atomic<bool> stopRequested = false;
	std::atomic<bool> paused = false;

	void run(ReplaySequence sequence, PlayerColor observer, Options options);
	void markFinished();
};
