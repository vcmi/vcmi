/*
 * ScriptDefines.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
class Module;
class ScriptImpl;
class ScriptHandler;

using ModulePtr = std::shared_ptr<Module>;
using ScriptPtr = std::shared_ptr<ScriptImpl>;
using ScriptMap = std::map<std::string, ScriptPtr>;
}

VCMI_LIB_NAMESPACE_END
