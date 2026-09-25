/*
 * ClassicDecisionTrace.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

struct ClassicDecisionTraceEntry
{
	std::string stage;
	std::string key;
	int64_t value = 0;
};

/// Optional deterministic trace of tactical evaluations and decision choices.
class ClassicDecisionTrace
{
	std::vector<ClassicDecisionTraceEntry> entries;

public:
	void clear();
	void record(std::string stage, std::string key, int64_t value);
	const std::vector<ClassicDecisionTraceEntry> & getEntries() const;
};
