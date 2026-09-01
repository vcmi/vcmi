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
#include "../../lib/int3.h"

class ReplayLog;
class CGameState;
class CMapHandler;
class CClient;

/// A single turn that the player may ask to see again
struct ReplayTurnOption
{
	PlayerColor player;
	uint32_t day = 0;
	bool ongoing = false;

	size_t chapter = 0;
	size_t turn = 0;
};

/// One look at a recorded day, from its start up to `packCount`
struct ReplayPass
{
	/// player the camera follows - unset for a day that was not played with simultaneous turns
	std::optional<PlayerColor> followedPlayer;

	/// how many packs of the day are shown, counted from the start of the day
	size_t packCount = 0;
};

/// One recorded day. A day of simultaneous turns holds one pass per player, since its packs are
/// interleaved - every pass ends with the last pack of the player it is watched with.
struct ReplayDay
{
	std::vector<std::vector<std::byte>> packs;
	std::vector<ReplayPass> passes;
};

/// Everything that is needed to play a stretch of recorded netpacks back
struct ReplaySequence
{
	PlayerColor replayedPlayer;
	uint32_t day = 0;

	std::vector<std::byte> snapshot;

	/// packs between the snapshot and the beginning of the replayed part - applied without visuals
	std::vector<std::vector<std::byte>> fastForwardPacks;

	/// days that are actually shown
	std::vector<ReplayDay> days;
};

/// Player the camera has to follow because a day of simultaneous turns is being replayed.
/// Empty whenever the camera shall behave the way it does in a live game.
std::optional<PlayerColor> replayFollowedPlayer();

/// Turns the recording stored in the gamestate into something that can be played back
namespace ReplayPlanner
{
	/// Turns that can still be watched, oldest first
	std::vector<ReplayTurnOption> availableTurns(const ReplayLog & log);

	ReplaySequence prepareTurn(const ReplayLog & log, const ReplayTurnOption & option);

	/// Only possible when the whole game was recorded
	ReplaySequence prepareEntireGame(const ReplayLog & log);
}

/// Plays recorded netpacks back on a throw-away session that is swapped into CClient and out again.
/// Nothing is sent to the server, and packs that would ask for input are applied without showing.
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

	/// Player the camera has to follow, set only while a day of simultaneous turns is replayed.
	/// Everywhere else the camera keeps following whoever it would follow in a live game.
	std::optional<PlayerColor> followedPlayer() const;

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

	bool stopRequested = false;
	bool paused = false;
	std::optional<PlayerColor> followed;

	void run(ReplaySequence sequence, PlayerColor observer, Options options);
	void markFinished();

	/// Blocks while the replay is paused. Returns false if the replay shall end instead of continuing.
	bool waitWhilePaused();
	bool isStopRequested() const;
	void setFollowedPlayer(const std::optional<PlayerColor> & value);

	/// Builds the interface one pass is watched through, and returns the map handler it replaced.
	/// Interface mutex must be held.
	std::unique_ptr<CMapHandler> beginPass(CClient & client, CGameState & state, PlayerColor observer, PlayerColor turnPlayer, const int3 & viewCenter);

	/// Drops everything beginPass() built, keeping the gamestate itself. Interface mutex must be held.
	void endPass(CClient & client, const std::shared_ptr<CGameState> & state);
};
