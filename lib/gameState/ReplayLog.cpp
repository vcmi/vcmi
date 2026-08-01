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
#include "../serializer/BinaryDeserializer.h"
#include "../serializer/BinarySerializer.h"

namespace
{
	class ReplayPackWriter final : public IBinaryWriter
	{
	public:
		std::vector<std::byte> data;

		int write(const std::byte * source, unsigned size) final
		{
			data.insert(data.end(), source, source + size);
			return size;
		}
	};

	class ReplayPackReader final : public IBinaryReader
	{
		const std::vector<std::byte> & data;
		size_t position = 0;

	public:
		explicit ReplayPackReader(const std::vector<std::byte> & data)
			: data(data)
		{
		}

		int read(std::byte * target, unsigned size) final
		{
			if(position + size > data.size())
				throw std::runtime_error("Recorded netpack ended unexpectedly!");

			std::copy_n(data.begin() + position, size, target);
			position += size;
			return size;
		}
	};
}

std::vector<std::byte> ReplayPackSerializer::write(const CPackForClient & pack)
{
	ReplayPackWriter writer;
	BinarySerializer serializer(&writer);
	serializer.version = ESerializationVersion::CURRENT;
	serializer & &pack;

	return std::move(writer.data);
}

std::unique_ptr<CPack> ReplayPackSerializer::read(const std::vector<std::byte> & data, IGameInfoCallback * cb)
{
	ReplayPackReader reader(data);
	BinaryDeserializer deserializer(&reader);
	deserializer.version = ESerializationVersion::CURRENT;
	deserializer.cb = cb;

	std::unique_ptr<CPack> result;
	deserializer & result;

	if(result == nullptr)
		throw std::runtime_error("Failed to read a netpack from the replay log!");

	return result;
}

void ReplayLog::configure(bool recordEntireGameValue, uint32_t roundsKeptValue)
{
	recordEntireGame = recordEntireGameValue;
	roundsKept = roundsKeptValue;
}

void ReplayLog::beginDay(std::vector<std::byte> snapshot)
{
	logGlobal->debug("Replay: new chapter, snapshot of %d bytes, %d chapters kept", snapshot.size(), chapters.size() + 1);

	chapters.emplace_back();
	chapters.back().snapshot = std::move(snapshot);

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
		chapters[i].snapshot.clear();
		chapters[i].snapshot.shrink_to_fit();
	}
}

void ReplayLog::addTurn(const PlayerColor & player, uint32_t day)
{
	if(chapters.empty())
		return;

	chapters.back().turns.push_back({player, day, static_cast<uint32_t>(chapters.back().packs.size())});
}

void ReplayLog::recordPack(CPackForClient & pack, CGameState & gs)
{
	// every game day opens a new chapter, anchored by the state that day started from
	if(chapters.empty() || dynamic_cast<const NewTurn *>(&pack) != nullptr)
		beginDay(gs.saveToMemory());

	if(const auto * turnStart = dynamic_cast<const PlayerStartsTurn *>(&pack))
		addTurn(turnStart->player, gs.day);

	try
	{
		addPack(ReplayPackSerializer::write(pack));
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Failed to record netpack '%s' for replay: %s", typeid(pack).name(), e.what());
	}
}

void ReplayLog::addPack(std::vector<std::byte> data)
{
	if(chapters.empty())
		return;

	chapters.back().packs.push_back(std::move(data));
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
	return recordEntireGame && !chapters.empty() && !chapters.front().snapshot.empty();
}

const std::vector<ReplayChapter> & ReplayLog::getChapters() const
{
	return chapters;
}
