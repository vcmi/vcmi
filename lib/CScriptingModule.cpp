/*
 * CScriptingModule.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CScriptingModule.h"

namespace scripting
{

ContextBase::ContextBase(vstd::CLoggerBase * logger_)
	: logger(logger_)
{

}

ContextBase::~ContextBase() = default;


Module::Module()
{
}

Module::~Module() = default;

}
