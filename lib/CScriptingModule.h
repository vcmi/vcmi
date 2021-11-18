/*
 * CScriptingModule.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/scripting/Service.h>

namespace spells
{
	namespace effects
	{
		class Registry;
	}
}

namespace scripting
{

class DLL_LINKAGE ContextBase : public Context
{
public:
	ContextBase(vstd::CLoggerBase * logger_);
	virtual ~ContextBase();
protected:
	vstd::CLoggerBase * logger;
};

class DLL_LINKAGE Module
{
public:
	Module();
	virtual ~Module();

	virtual std::string compile(const std::string & name, const std::string & source, vstd::CLoggerBase * logger) const = 0;

	virtual std::shared_ptr<ContextBase> createContextFor(const Script * source, const Environment * env) const = 0;

	virtual void registerSpellEffect(spells::effects::Registry * registry, const Script * source) const = 0;
};

}
