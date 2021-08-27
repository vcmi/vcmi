/*
 * ERMScriptModule.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/CScriptingModule.h"

class ERMScriptModule : public scripting::Module
{
public:
	ERMScriptModule();

	std::string compile(const std::string & name, const std::string & source, vstd::CLoggerBase * logger) const override;

	std::shared_ptr<scripting::ContextBase> createContextFor(const scripting::Script * source, const Environment * env) const override;

	void registerSpellEffect(spells::effects::Registry * registry, const scripting::Script * source) const override;
};

