/*
 * ClassicDecisionTrace.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicDecisionTrace.h"

void ClassicDecisionTrace::clear()
{
	entries.clear();
}

void ClassicDecisionTrace::record(std::string stage, std::string key, int64_t value)
{
	entries.push_back({std::move(stage), std::move(key), value});
}

const std::vector<ClassicDecisionTraceEntry> & ClassicDecisionTrace::getEntries() const
{
	return entries;
}
