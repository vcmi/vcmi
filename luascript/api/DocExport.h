/*
 * DocExport.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <boost/filesystem/path.hpp>

namespace scripting::api
{

/// Walks Registry::getAllTypes(), drives a DocRegistrar per type and writes two reference
/// files into outDir: API.md (Markdown) and api.lua (Lua Language Server definitions).
/// Creates outDir if it does not exist.
void exportLuaApiDocs(const boost::filesystem::path & outDir);

}
