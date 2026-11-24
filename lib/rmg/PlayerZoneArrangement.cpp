/*
 * PlayerZoneArrangement.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "PlayerZoneArrangement.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
using Link = std::pair<TRmgTemplateZoneId, rmg::EConnectionType>;

std::vector<Link> sortedLinks(const PlayerZoneInfo & zone, TRmgTemplateZoneId swapA, TRmgTemplateZoneId swapB)
{
	std::vector<Link> out;
	out.reserve(zone.links.size());
	for(const auto & link : zone.links)
	{
		if(link.first == zone.id)
			continue;
		TRmgTemplateZoneId other = link.first;
		if(other == swapA)
			other = swapB;
		else if(other == swapB)
			other = swapA;
		out.emplace_back(other, link.second);
	}
	std::sort(out.begin(), out.end());
	return out;
}

bool interchangeable(const PlayerZoneInfo & a, const PlayerZoneInfo & b)
{
	if(a.id == b.id || a.level != b.level || a.size != b.size)
		return false;
	return sortedLinks(a, a.id, b.id) == sortedLinks(b, b.id, b.id)
		&& sortedLinks(b, a.id, b.id) == sortedLinks(a, a.id, a.id);
}

TRmgTemplateZoneId findRoot(std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> & parent, TRmgTemplateZoneId id)
{
	auto it = parent.find(id);
	if(it == parent.end())
		return id;
	if(it->second != id)
		it->second = findRoot(parent, it->second);
	return parent.at(id);
}

void unite(std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> & parent, TRmgTemplateZoneId a, TRmgTemplateZoneId b)
{
	const auto rootA = findRoot(parent, a);
	const auto rootB = findRoot(parent, b);
	if(rootA != rootB)
		parent.at(rootA) = rootB;
}

struct Slot
{
	float3 center;
	TRmgTemplateZoneId originalId = 0;
	bool taken = false;
};
}

std::vector<std::vector<TRmgTemplateZoneId>> findInterchangeableZones(const std::vector<PlayerZoneInfo> & zones)
{
	std::map<TRmgTemplateZoneId, TRmgTemplateZoneId> parent;
	for(const auto & zone : zones)
		parent[zone.id] = zone.id;

	for(size_t i = 0; i < zones.size(); ++i)
	{
		for(size_t j = i + 1; j < zones.size(); ++j)
		{
			if(interchangeable(zones[i], zones[j]))
				unite(parent, zones[i].id, zones[j].id);
		}
	}

	std::map<TRmgTemplateZoneId, std::vector<TRmgTemplateZoneId>> grouped;
	for(const auto & zone : zones)
		grouped[findRoot(parent, zone.id)].push_back(zone.id);

	std::vector<std::vector<TRmgTemplateZoneId>> groups;
	for(auto & [root, members] : grouped)
	{
		if(members.size() > 1)
			groups.push_back(std::move(members));
	}
	return groups;
}

std::map<TRmgTemplateZoneId, float3> arrangeByPlayerOrder(const std::vector<PlayerZoneInfo> & group)
{
	std::map<TRmgTemplateZoneId, float3> result;
	for(const auto & zone : group)
		result[zone.id] = zone.center;
	if(group.size() < 2)
		return result;

	std::vector<PlayerZoneInfo> ordered = group;
	std::sort(ordered.begin(), ordered.end(), [](const PlayerZoneInfo & a, const PlayerZoneInfo & b)
	{
		if(a.participates != b.participates)
			return a.participates && !b.participates;
		if(a.owner != b.owner)
			return a.owner < b.owner;
		return a.id < b.id;
	});

	std::vector<Slot> slots;
	slots.reserve(group.size());
	for(const auto & zone : group)
		slots.push_back(Slot{zone.center, zone.id, false});

	int bestA = -1;
	int bestB = -1;
	double bestDist = -1;
	for(size_t i = 0; i < slots.size(); ++i)
	{
		for(size_t j = i + 1; j < slots.size(); ++j)
		{
			const double distance = slots[i].center.dist2d(slots[j].center);
			bool better = distance > bestDist;
			if(!better && distance == bestDist && bestA >= 0)
			{
				const auto minId = std::min(slots[i].originalId, slots[j].originalId);
				const auto maxId = std::max(slots[i].originalId, slots[j].originalId);
				const auto bestMin = std::min(slots[bestA].originalId, slots[bestB].originalId);
				const auto bestMax = std::max(slots[bestA].originalId, slots[bestB].originalId);
				better = minId < bestMin || (minId == bestMin && maxId < bestMax);
			}
			if(better)
			{
				bestDist = distance;
				bestA = static_cast<int>(i);
				bestB = static_cast<int>(j);
			}
		}
	}

	int firstSlot = bestA;
	int secondSlot = bestB;
	if(slots[bestB].originalId < slots[bestA].originalId)
		std::swap(firstSlot, secondSlot);

	result[ordered[0].id] = slots[firstSlot].center;
	result[ordered[1].id] = slots[secondSlot].center;
	slots[firstSlot].taken = true;
	slots[secondSlot].taken = true;

	std::vector<float3> placed {slots[firstSlot].center, slots[secondSlot].center};

	for(size_t n = 2; n < ordered.size(); ++n)
	{
		int bestSlot = -1;
		double bestMinDist = -1;
		for(size_t i = 0; i < slots.size(); ++i)
		{
			if(slots[i].taken)
				continue;
			double minDist = std::numeric_limits<double>::infinity();
			for(const auto & center : placed)
				minDist = std::min(minDist, slots[i].center.dist2d(center));
			bool better = minDist > bestMinDist;
			if(!better && minDist == bestMinDist && bestSlot >= 0)
				better = slots[i].originalId < slots[bestSlot].originalId;
			if(better)
			{
				bestMinDist = minDist;
				bestSlot = static_cast<int>(i);
			}
		}
		slots[bestSlot].taken = true;
		result[ordered[n].id] = slots[bestSlot].center;
		placed.push_back(slots[bestSlot].center);
	}

	return result;
}

float playerCrowding(const std::vector<ZonePairDistance> & pairs)
{
	if(pairs.empty())
		return 0.f;

	float sum = 0.f;
	for(const auto & pair : pairs)
	{
		const float denom = std::max(pair.distance, 0.1f * pair.touching);
		sum += (denom <= 0.f) ? 10.f : pair.touching / denom;
	}
	return sum / static_cast<float>(pairs.size());
}

float playerPairWeight(int ownerA, int ownerB, bool bothParticipate)
{
	if(!bothParticipate)
		return 1.f;
	return 1.f + 2.f / static_cast<float>(ownerA * ownerB);
}
