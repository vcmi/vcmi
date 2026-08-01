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

/// Marks the beginning of a single player turn inside a chapter
struct DLL_LINKAGE ReplayTurnMark
{
	PlayerColor player;
	uint32_t day = 0;

	/// index of the first pack of this turn inside ReplayChapter::packs
	uint32_t firstPack = 0;

	template <typename Handler> void serialize(Handler & h)
	{
		h & player;
		h & day;
		h & firstPack;
	}
};

/// One game day worth of recording: the gamestate as it was when the day started, plus every
/// netpack sent afterwards. Replaying a chapter from its snapshot always reproduces the very
/// same gamestate. The snapshot is dropped once a chapter falls out of the retention window,
/// which leaves the packs replayable only by fast-forwarding from an older chapter.
struct DLL_LINKAGE ReplayChapter
{
	std::vector<std::byte> snapshot;
	std::vector<std::vector<std::byte>> packs;
	std::vector<ReplayTurnMark> turns;

	template <typename Handler> void serialize(Handler & h)
	{
		h & snapshot;
		h & packs;
		h & turns;
	}
};

/// Recording of the game, kept inside CGameState so that it is part of every savegame.
/// Filled by the server, which is the only place where all netpacks pass through.
class DLL_LINKAGE ReplayLog
{
	std::vector<ReplayChapter> chapters;

	/// if set, no chapter is ever dropped and the whole game stays replayable
	bool recordEntireGame = false;

	/// number of past days that stay fully replayable on top of the current one
	uint32_t roundsKept = 1;

	void dropExpiredData();

public:
	void configure(bool recordEntireGame, uint32_t roundsKept);

	/// Opens a chapter for a new game day. `snapshot` must be the gamestate as of right now.
	void beginDay(std::vector<std::byte> snapshot);

	void addTurn(const PlayerColor & player, uint32_t day);
	void addPack(std::vector<std::byte> data);

	/// Records a single netpack. Must be called before the pack is applied on `gs`, so that a
	/// snapshot taken here holds the state the new day started from.
	void recordPack(CPackForClient & pack, CGameState & gs);

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
