/*
 * DocRegistrar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "MethodRegistrar.h"

namespace scripting::api
{

/// MethodRegistrar implementation that collects bindings as metadata.
/// Drives the same `registerMethods` code path the runtime uses, but instead of pushing
/// closures onto a Lua table it accumulates fully-structured DocEntry records that
/// downstream code (the docs exporter) can inspect directly without
/// any string parsing.
class DocRegistrar final : public MethodRegistrar
{
public:
	using Entry = DocEntry;

	void addEntry(DocEntry entry, lua_CFunction /*fn*/) override
	{
		entries.push_back(std::move(entry));
	}

	const std::vector<Entry> & get() const { return entries; }

private:
	std::vector<Entry> entries;
};

}
