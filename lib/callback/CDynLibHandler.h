/*
 * CDynLibHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class CGlobalAI;
class CBattleGameInterface;

#if SCRIPTING_ENABLED
namespace scripting
{
	class Module;
}
#endif

class DLL_LINKAGE CDynLibHandler
{
public:
	static std::shared_ptr<CGlobalAI> getNewAI(const std::string & dllname);
	static std::shared_ptr<CBattleGameInterface> getNewBattleAI(const std::string & dllname);
#if SCRIPTING_ENABLED
	static std::shared_ptr<scripting::Module> getNewScriptingModule(const boost::filesystem::path & dllname);
#endif
};

VCMI_LIB_NAMESPACE_END
