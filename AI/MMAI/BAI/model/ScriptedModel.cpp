/*
 * ScriptedModel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ScriptedModel.h"
#include "schema/base.h"

namespace MMAI::BAI
{

ScriptedModel::ScriptedModel(const std::string & keyword) : keyword(keyword)
{
	static const std::vector<std::string> FALLBACKS = {"StupidAI", "BattleAI"};
	auto it = std::ranges::find(FALLBACKS, keyword);
	if(it == FALLBACKS.end())
		throw std::runtime_error("Unsupported fallback keyword: " + keyword);
}

std::string ScriptedModel::getName()
{
	return keyword;
}

Schema::ModelType ScriptedModel::getType()
{
	return Schema::ModelType::SCRIPTED;
}

Schema::Side ScriptedModel::getSide()
{
	return Schema::Side::BOTH;
}

// SCRIPTED models are dummy models which should not be used for anything
// other than their getType() and getName() methods. Based on the return
// value, the corresponding scripted bot (e.g. StupidAI) should be
// used for the upcoming battle instead.
// When MMAI fails to load an ML model, it loads a SCRIPTED model instead
// as per MMAI mod's "fallback" setting in order to prevent a game crash.

// The below methods should never be called on this object:
int ScriptedModel::getVersion()
{
	warn("getVersion", -666);
	return -666;
};

int ScriptedModel::getAction(const MMAI::Schema::IState * s)
{
	warn("getAction", -666);
	return -666;
};

double ScriptedModel::getValue(const MMAI::Schema::IState * s)
{
	warn("getValue", -666);
	return -666;
};

void ScriptedModel::warn(const std::string & m, int retval) const
{
	logAi->error("WARNING: method %s called on a ScriptedModel object; returning %d\n", m.c_str(), retval);
}
}
