/*
 * LuaScriptInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

#include "../lib/filesystem/ResourcePath.h"

class JsonNode;
class JsonSerializeFormat;
class Services;

namespace scripting
{

class LuaModule;
class LuaContext;

/// Holds the source code of a script (optionally with one or more patch layers stacked over a base)
/// and metadata; one instance per logical script.
/// Owned by LuaModule and used as a factory to create LuaContext instances for each game session.
class LuaScriptInstance final : public Script
{
public:
	struct Layer
	{
		std::string sourceText;
		std::string identifier; ///< modScope + ':' + sourcePath, used for error reporting and chunk naming
	};

	/// Builds the chain from resource files: layer[0] is the base, layer[1..] are patches in declared order.
	/// patches entries are (modScope, sourcePath) pairs.
	/// Failed-to-load patch layers are skipped with a logged error; failed base load leaves layers empty.
	LuaScriptInstance(const LuaModule & host,
		const std::string & baseScope, const ScriptPath & basePath,
		const std::vector<std::pair<std::string, std::string>> & patches);

	/// Builds a single-layer script directly from source text, for scripts generated at runtime
	/// (e.g. a map's converted event system) that have no backing resource file.
	LuaScriptInstance(const LuaModule & host, const std::string & baseScope, std::string sourceText);

	virtual ~LuaScriptInstance();

	std::vector<Layer> layers;

	const LuaModule & host;

	std::shared_ptr<LuaContext> createContext(const Environment * ENV) const;

	std::string getIdentifier() const override { return baseModScope + baselSourcePath; }

private:
	std::string baseModScope;
	std::string baselSourcePath;
	void loadLayer(const std::string & modScope, const ScriptPath & sourcePath);
	void loadLayer(const std::string & modScope, const std::string & sourcePath);
};
}
