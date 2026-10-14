/*
 * ReplayLog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ReplayLog.h"

#include "CGameState.h"

#include "../networkPacks/NetPacksBase.h"
#include "../networkPacks/PacksForClient.h"
#include "../serializer/CMemorySerializer.h"

std::vector<std::byte> ReplayPackSerializer::write(const CPackForClient & pack)
{
	CMemorySerializer serializer;
	serializer.oser & &pack;

	return serializer.extractBuffer();
}

std::unique_ptr<CPack> ReplayPackSerializer::read(const std::vector<std::byte> & data, IGameInfoCallback * cb)
{
	CMemorySerializer serializer(data);
	serializer.iser.cb = cb;

	std::unique_ptr<CPack> result;
	serializer.iser & result;

	if(result == nullptr)
		throw std::runtime_error("Failed to read a netpack from the replay log!");

	return result;
}

void ReplayLog::configure(bool recordEntireGameValue, uint32_t roundsKeptValue)
{
	recordEntireGame = recordEntireGameValue;
	roundsKept = roundsKeptValue;
	recordingPacks = true;
}

void ReplayLog::reconfigureOnLoad(uint32_t roundsKeptValue)
{
	roundsKept = roundsKeptValue;
	recordingPacks = true;
}

void ReplayLog::beginDay(uint32_t day, std::vector<std::byte> gamestateSnapshot)
{
	logGlobal->debug("Replay: new chapter, snapshot of %d bytes, %d chapters kept", gamestateSnapshot.size(), chapters.size() + 1);

	chapters.emplace_back();
	chapters.back().day = day;
	chapters.back().gamestateSnapshot = std::move(gamestateSnapshot);

	dropExpiredData();
}

void ReplayLog::dropExpiredData()
{
	// the current day plus `roundsKept` days before it stay fully replayable
	const size_t chaptersToKeep = roundsKept + 1;

	if(chapters.size() <= chaptersToKeep)
		return;

	if(!recordEntireGame)
	{
		chapters.erase(chapters.begin(), chapters.end() - chaptersToKeep);
		return;
	}

	// when the whole game is recorded no packs may be lost, but snapshots of days that fell out
	// of the window are not needed - those days are reached by fast-forwarding from the first one
	for(size_t i = 1; i < chapters.size() - chaptersToKeep; ++i)
	{
		chapters[i].gamestateSnapshot.clear();
		chapters[i].gamestateSnapshot.shrink_to_fit();
	}
}

void ReplayLog::addTurn(const PlayerColor & player)
{
	if(chapters.empty())
		return;

	// the server re-announces a turn that is already running, e.g. after loading a save
	for(const auto & turn : chapters.back().turns)
		if(turn.player == player && turn.lastPack == ReplayTurnMark::ongoingTurn)
			return;

	ReplayTurnMark mark;
	mark.player = player;
	mark.firstPack = static_cast<uint32_t>(chapters.back().packs.size());

	chapters.back().turns.push_back(mark);
}

void ReplayLog::endTurn(const PlayerColor & player)
{
	if(chapters.empty())
		return;

	// with simturns several turns are open at once, so the right one has to be picked
	auto & turns = chapters.back().turns;
	for(size_t i = turns.size(); i-- > 0;)
	{
		if(turns[i].player == player && turns[i].lastPack == ReplayTurnMark::ongoingTurn)
		{
			turns[i].lastPack = static_cast<uint32_t>(chapters.back().packs.size());
			return;
		}
	}
}

void ReplayLog::recordPack(CPackForClient & pack, CGameState & gs)
{
	// every game day opens a new chapter, anchored by the state that day started from.
	// NewTurn is recorded before it is applied, so the new day is only known from the pack itself
	const auto * newTurn = dynamic_cast<const NewTurn *>(&pack);
	if(chapters.empty() || newTurn != nullptr)
		beginDay(newTurn ? newTurn->day : gs.day, gs.saveToMemory());

	if(const auto * turnStart = dynamic_cast<const PlayerStartsTurn *>(&pack))
		addTurn(turnStart->player);

	if(const auto * turnEnd = dynamic_cast<const PlayerEndsTurn *>(&pack))
		endTurn(turnEnd->player);

	addPack(ReplayPackSerializer::write(pack));
}

void ReplayLog::addPack(std::vector<std::byte> data)
{
	if(chapters.empty())
		return;

	chapters.back().packs.push_back(std::move(data));
}

bool ReplayLog::isRecordingPacks() const
{
	return recordingPacks;
}

bool ReplayLog::empty() const
{
	return chapters.empty();
}

bool ReplayLog::isRecordingEntireGame() const
{
	return recordEntireGame;
}

bool ReplayLog::canReplayEntireGame() const
{
	return recordEntireGame && !chapters.empty() && !chapters.front().gamestateSnapshot.empty();
}

const std::vector<ReplayChapter> & ReplayLog::getChapters() const
{
	return chapters;
}
