/*
 * ReplayLog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"

struct CPack;
struct CPackForClient;
class CGameState;
class IGameInfoCallback;

/// Conversion of netpacks to and from the byte blobs kept in a ReplayLog.
/// Lives in lib because both the recording server and the replaying client need it.
namespace ReplayPackSerializer
{
	DLL_LINKAGE std::vector<std::byte> write(const CPackForClient & pack);
	DLL_LINKAGE std::unique_ptr<CPack> read(const std::vector<std::byte> & data, IGameInfoCallback * cb);
}

/// Marks a single player turn inside a chapter. With simturns those ranges overlap.
struct DLL_LINKAGE ReplayTurnMark
{
	/// value of lastPack while the turn has not ended yet
	static constexpr uint32_t ongoingTurn = std::numeric_limits<uint32_t>::max();

	PlayerColor player;

	/// index of the first pack of this turn inside ReplayChapter::packs
	uint32_t firstPack = 0;

	/// index one past the last pack of this turn
	uint32_t lastPack = ongoingTurn;

	template <typename Handler> void serialize(Handler & h)
	{
		h & player;
		h & firstPack;
		h & lastPack;
	}
};

/// One game day of recording: the gamestate the day started from, plus every netpack sent afterwards.
/// A day that falls out of the retention window is dropped entirely - unless the whole game is
/// recorded, in which case only its snapshot goes: its packs are still needed to fast-forward from
/// the snapshot of the very first day to any later one.
struct DLL_LINKAGE ReplayChapter
{
	uint32_t day = 0;
	std::vector<std::byte> gamestateSnapshot;
	std::vector<std::vector<std::byte>> packs;
	std::vector<ReplayTurnMark> turns;

	template <typename Handler> void serialize(Handler & h)
	{
		h & day;
		h & gamestateSnapshot;
		h & packs;
		h & turns;
	}
};

/// Recording of the game, kept inside CGameState so that it is part of every savegame.
/// The server records every pack it sends out in CGameState::apply(), the client records every pack
/// it receives - so both sides end up with a log of their own that they can replay on their own.
class DLL_LINKAGE ReplayLog
{
	std::vector<ReplayChapter> chapters;

	/// if set, no chapter is ever dropped and the whole game stays replayable
	bool recordEntireGame = false;

	/// number of past days that stay fully replayable on top of the current one
	uint32_t roundsKept = 1;

	/// set by configure(), deliberately not serialized - only the recording side turns it on
	bool recordingPacks = false;

	void dropExpiredData();

public:
	void configure(bool recordEntireGame, uint32_t roundsKept);

	/// Resumes a log restored from a savegame, keeping the recording mode it was started with
	void reconfigureOnLoad(uint32_t roundsKept);

	/// Opens a chapter for a new game day. `gamestateSnapshot` must be the gamestate as of right now.
	void beginDay(uint32_t day, std::vector<std::byte> gamestateSnapshot);

	void addTurn(const PlayerColor & player);
	void endTurn(const PlayerColor & player);
	void addPack(std::vector<std::byte> data);

	/// Records a single netpack. Must be called before the pack is applied on `gs`, so that a
	/// snapshot taken here holds the state the new day started from.
	void recordPack(CPackForClient & pack, CGameState & gs);

	/// True if packs passing through CGameState::apply() are recorded
	bool isRecordingPacks() const;

	bool empty() const;
	bool isRecordingEntireGame() const;

	/// True if the game can be watched from its very beginning
	bool canReplayEntireGame() const;

	const std::vector<ReplayChapter> & getChapters() const;

	template <typename Handler> void serialize(Handler & h)
	{
		h & chapters;
		h & recordEntireGame;
		h & roundsKept;
	}
};
