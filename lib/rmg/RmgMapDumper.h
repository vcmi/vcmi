/*
 * RmgMapDumper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../int3.h"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

class RmgMap;
class CMapGenerator;
class Modificator;

/// Tracks which requested dump stages have completed. A stage is "zones" or a modificator name.
class DLL_LINKAGE RmgDumpStages
{
public:
	explicit RmgDumpStages(std::set<std::string> requested);
	bool isRequested(const std::string & stage) const;
	/// Called once per modificator instance before the jobs run.
	void expect(const std::string & stage);
	/// True exactly once: when the last outstanding instance of a requested stage finishes.
	bool finish(const std::string & stage);
	/// Requested stages that are neither "zones" nor any expected modificator.
	std::vector<std::string> unknown() const;

private:
	std::set<std::string> requested;
	std::set<std::string> expected;
	std::map<std::string, int> remaining;
	std::set<std::string> completed;
};

/// One char per tile, x along a row, y down the rows, levels separated by an empty line.
template<typename TileChar>
std::string renderDumpGrid(int width, int height, int levels, const TileChar & tileChar)
{
	std::string out;
	out.reserve(static_cast<size_t>(std::max(0, levels)) * (static_cast<size_t>(std::max(0, height)) * (static_cast<size_t>(std::max(0, width)) + 1) + 1));
	for(int z = 0; z < levels; ++z)
	{
		if(z > 0)
			out.push_back('\n');
		for(int y = 0; y < height; ++y)
		{
			for(int x = 0; x < width; ++x)
				out.push_back(tileChar(int3(x, y, z)));
			out.push_back('\n');
		}
	}
	return out;
}

/// Writes whole-map snapshots to <userLogsPath>/rmg-dumps/<seed>_<NN>_<stage>.txt
class RmgMapDumper
{
public:
	RmgMapDumper(RmgMap & map, const CMapGenerator & generator, std::set<std::string> stages);
	void dumpZones();
	void expectModificators();
	void onModificatorFinished(const Modificator & m);
	void reportUnknownStages() const;

private:
	void writeDump(const std::string & stage, const std::function<char(const int3 &)> & tileChar, const std::string & legend);
	char zoneTileChar(const int3 & tile) const;
	char modificatorTileChar(const int3 & tile, const std::string & stage);
	std::string zoneLegend();
	std::string modificatorLegend(const std::string & stage);
	std::string validStageList() const;

	RmgMap & map;
	const CMapGenerator & generator;
	RmgDumpStages stages;
	std::set<std::string> expectedModificatorNames;
	int dumpIndex = 0;
	bool dumpsDirCreated = false;
};
