/*
 * scripting/Service.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Environment.h>

#include <boost/filesystem/path.hpp>

VCMI_LIB_NAMESPACE_BEGIN

namespace spells::effects
{
    class SpellEffectService;
}

namespace scripting
{

using BattleCb = Environment::BattleCb;
using GameCb = Environment::GameCb;

class DLL_LINKAGE Context
{
public:
	virtual ~Context() = default;
};

class DLL_LINKAGE Script
{
public:
	virtual ~Script() = default;

	virtual std::string getIdentifier() const = 0;
};

class DLL_LINKAGE Pool
{
public:
	virtual ~Pool() = default;

	virtual std::shared_ptr<Context> getContext(const Script * script) const = 0;
};

class DLL_LINKAGE Service
{
public:
	virtual ~Service() = default;

	virtual void installScripting(spells::effects::SpellEffectService * spellEffects) = 0;

	virtual std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const = 0;

	/// Writes Markdown and Lua Language Server reference files describing every exposed API type
	/// into the given output directory. Used by `vcmiserver --export-lua-docs <path>` to keep
	/// the modder-facing scripting reference in sync with the host bindings.
	virtual void exportDocs(const boost::filesystem::path & outDir) const = 0;
};

}

VCMI_LIB_NAMESPACE_END
