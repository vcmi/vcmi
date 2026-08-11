/*
 * LuaScriptModule.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

namespace scripting
{

class LuaScriptFactory;
class LuaScriptStore;

/// Top-level Lua scripting service; owns every loaded Lua script and creates script pools.
class DLL_LINKAGE LuaModule final : public Service
{
public:
	LuaModule();
	~LuaModule();

	void installScripting(ScriptService & scripts) override;

	std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const override;

	std::unique_ptr<MapEventDispatcher> createMapScriptDispatcher(CGameState & gs, bool runInit) const override;

	void exportDocs(const boost::filesystem::path & outDir) const override;

private:
	std::unique_ptr<LuaScriptStore> store;
	std::shared_ptr<LuaScriptFactory> factory;
};
}
