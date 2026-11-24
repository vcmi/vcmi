/*
 * RmgMapDumper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RmgMapDumper.h"

#include "CMapGenOptions.h"
#include "CMapGenerator.h"
#include "CRmgTemplate.h"
#include "RmgMap.h"
#include "Zone.h"
#include "modificators/Modificator.h"
#include "modificators/ObjectManager.h"
#include "../VCMIDirs.h"
#include "../logging/CLogger.h"

#include <fstream>

namespace
{
constexpr std::string_view ZONE_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

char zoneIdChar(TRmgTemplateZoneId id)
{
	if(id < 0 || static_cast<size_t>(id) >= ZONE_CHARS.size())
		return '?';
	return ZONE_CHARS[static_cast<size_t>(id)];
}

const char * zoneTypeName(ETemplateZoneType type)
{
	switch(type)
	{
		case ETemplateZoneType::PLAYER_START:
			return "playerStart";
		case ETemplateZoneType::CPU_START:
			return "cpuStart";
		case ETemplateZoneType::TREASURE:
			return "treasure";
		case ETemplateZoneType::JUNCTION:
			return "junction";
		case ETemplateZoneType::WATER:
			return "water";
		case ETemplateZoneType::SEALED:
			return "sealed";
	}
	return "?";
}

std::string ownerLabel(const Zone & zone)
{
	if(auto owner = zone.getOwner())
		return std::to_string(*owner);
	return "-";
}

std::string joinNames(const std::vector<std::string> & names)
{
	std::string listed;
	for(const auto & name : names)
	{
		if(!listed.empty())
			listed += ", ";
		listed += name;
	}
	return listed;
}

Modificator * findModificatorByName(Zone & zone, const std::string & name)
{
	for(const auto & modificator : zone.getModificators())
	{
		if(modificator->getName() == name)
			return modificator.get();
	}
	return nullptr;
}
}

RmgDumpStages::RmgDumpStages(std::set<std::string> requested)
	: requested(std::move(requested))
{
}

bool RmgDumpStages::isRequested(const std::string & stage) const
{
	return requested.count(stage) > 0;
}

void RmgDumpStages::expect(const std::string & stage)
{
	expected.insert(stage);
	if(isRequested(stage))
		++remaining[stage];
}

bool RmgDumpStages::finish(const std::string & stage)
{
	if(!isRequested(stage))
		return false;

	auto it = remaining.find(stage);
	if(it == remaining.end() || it->second <= 0)
		return false;

	--it->second;
	if(it->second != 0)
		return false;

	return completed.insert(stage).second;
}

std::vector<std::string> RmgDumpStages::unknown() const
{
	std::vector<std::string> result;
	for(const auto & name : requested)
	{
		if(name == "zones")
			continue;
		if(!expected.count(name))
			result.push_back(name);
	}
	return result;
}

RmgMapDumper::RmgMapDumper(RmgMap & map, const CMapGenerator & generator, std::set<std::string> stages)
	: map(map)
	, generator(generator)
	, stages(std::move(stages))
{
}

void RmgMapDumper::dumpZones()
{
	if(!stages.isRequested("zones"))
		return;

	writeDump("zones", [this](const int3 & tile) { return zoneTileChar(tile); }, zoneLegend());
}

void RmgMapDumper::expectModificators()
{
	for(auto & zonePair : map.getZones())
	{
		for(const auto & modificator : zonePair.second->getModificators())
		{
			stages.expect(modificator->getName());
			expectedModificatorNames.insert(modificator->getName());
		}
	}
}

void RmgMapDumper::onModificatorFinished(const Modificator & m)
{
	const std::string & name = m.getName();
	if(!stages.finish(name))
		return;

	writeDump(name, [this, name](const int3 & tile) { return modificatorTileChar(tile, name); }, modificatorLegend(name));
}

void RmgMapDumper::reportUnknownStages() const
{
	const auto unknown = stages.unknown();
	if(unknown.empty())
		return;

	logRmg->warn("Unknown RMG dump stages: %s; valid stages: %s", joinNames(unknown), validStageList());
}

void RmgMapDumper::writeDump(const std::string & stage, const std::function<char(const int3 &)> & tileChar, const std::string & legend)
{
	const auto dumpDir = VCMIDirs::get().userLogsPath() / "rmg-dumps";
	if(!dumpsDirCreated)
	{
		boost::filesystem::create_directories(dumpDir);
		dumpsDirCreated = true;
	}

	const auto fileName = boost::str(boost::format("%d_%02d_%s.txt") % generator.getRandomSeed() % dumpIndex % stage);
	++dumpIndex;

	std::ofstream out((dumpDir / fileName).c_str());
	const auto * mapTemplate = generator.getMapGenOptions().getMapTemplate();
	out << "template: " << (mapTemplate ? mapTemplate->getId() : "?") << '\n';
	out << "seed: " << generator.getRandomSeed() << '\n';
	out << "size: " << map.width() << 'x' << map.height() << '\n';
	out << "levels: " << map.levels() << '\n';
	out << "stage: " << stage << '\n';
	out << "legend:\n";
	if(!legend.empty())
		out << legend << '\n';
	out << '\n';
	out << renderDumpGrid(map.width(), map.height(), map.levels(), tileChar);
}

char RmgMapDumper::zoneTileChar(const int3 & tile) const
{
	return zoneIdChar(map.getZoneID(tile));
}

char RmgMapDumper::modificatorTileChar(const int3 & tile, const std::string & stage)
{
	const auto zoneId = map.getZoneID(tile);
	auto & zones = map.getZones();
	auto it = zones.find(zoneId);
	if(it == zones.end())
		return '?';

	if(auto * modificator = findModificatorByName(*it->second, stage))
		return modificator->dump(tile);

	if(auto * objectManager = it->second->getModificator<ObjectManager>())
		return objectManager->Modificator::dump(tile);

	return '?';
}

std::string RmgMapDumper::zoneLegend()
{
	std::ostringstream out;
	bool first = true;
	for(const auto & [id, zone] : map.getZones())
	{
		if(!first)
			out << '\n';
		first = false;
		out << zoneIdChar(id)
			<< " id=" << id
			<< " type=" << zoneTypeName(zone->getType())
			<< " owner=" << ownerLabel(*zone)
			<< " level=" << zone->getPos().z
			<< " size=" << zone->getSize()
			<< " pos=" << zone->getPos().toString();
	}
	return out.str();
}

std::string RmgMapDumper::modificatorLegend(const std::string & stage)
{
	std::ostringstream out;
	std::set<std::string> seen;
	bool usedFallback = false;
	bool first = true;

	auto appendLegend = [&](const std::string & name, const std::string & legend)
	{
		if(!first)
			out << '\n';
		first = false;
		out << name << '\n' << legend;
	};

	for(auto & zonePair : map.getZones())
	{
		if(const auto * modificator = findModificatorByName(*zonePair.second, stage))
		{
			if(seen.insert(modificator->getName()).second)
				appendLegend(modificator->getName(), modificator->dumpLegend());
		}
		else
		{
			usedFallback = true;
		}
	}

	if(usedFallback)
	{
		for(auto & zonePair : map.getZones())
		{
			if(const auto * objectManager = zonePair.second->getModificator<ObjectManager>())
			{
				if(seen.insert(objectManager->getName()).second)
					appendLegend(objectManager->getName(), objectManager->dumpLegend());
				break;
			}
		}
	}

	return out.str();
}

std::string RmgMapDumper::validStageList() const
{
	std::set<std::string> names = expectedModificatorNames;
	names.emplace("zones");
	return joinNames(std::vector<std::string>(names.begin(), names.end()));
}
