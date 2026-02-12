/*
 * ScriptImpl.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ScriptImpl.h"

#include "ScriptHandler.h"
#include "ScriptModule.h"

#include "../filesystem/Filesystem.h"
#include "../serializer/JsonSerializer.h"

#include <vcmi/Services.h>

VCMI_LIB_NAMESPACE_BEGIN

static const std::vector<std::string> IMPLEMENTS_MAP =
{
	"ANYTHING",
	"BATTLE_EFFECT"
};

namespace scripting
{

ScriptImpl::ScriptImpl(const ScriptHandler * owner_) : owner(owner_), implements(Implements::ANYTHING) {}

ScriptImpl::~ScriptImpl() = default;

void ScriptImpl::compile(vstd::CLoggerBase * logger)
{
	code = host->compile(sourcePath, sourceText, logger);
}

std::shared_ptr<Context> ScriptImpl::createContext(const Environment * env) const
{
	return host->createContextFor(this, env);
}

std::string ScriptImpl::getJsonKey() const
{
	return modScope + ':' + identifier;
}

const std::string & ScriptImpl::getSource() const
{
	return sourceText;
}

void ScriptImpl::performRegistration(Services * services) const
{
	switch(implements)
	{
		case Implements::ANYTHING:
			break;
		case Implements::BATTLE_EFFECT:
			host->registerSpellEffect(services->spellEffects(), this);
			break;
	}
}

void ScriptImpl::serializeJson(vstd::CLoggerBase * logger, JsonSerializeFormat & handler)
{
	handler.serializeString("source", sourcePath);
	handler.serializeEnum("implements", implements, Implements::ANYTHING, IMPLEMENTS_MAP);

	if(!handler.saving)
	{
		resolveHost();

		ResourcePath sourcePathId(sourcePath);

		auto rawData = CResourceHandler::get()->load(sourcePathId)->readAll();

		sourceText = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);

		compile(logger);
	}
}

void ScriptImpl::serializeJsonState(JsonSerializeFormat & handler)
{
	handler.serializeString("sourcePath", sourcePath);
	handler.serializeString("sourceText", sourceText);
	handler.serializeString("code", code);
	handler.serializeEnum("implements", implements, Implements::ANYTHING, IMPLEMENTS_MAP);

	if(!handler.saving)
	{
		host = owner->lua;
	}
}

void ScriptImpl::resolveHost()
{
	ResourcePath sourcePathId(sourcePath);

	if(sourcePathId.getType() == EResType::LUA_SCRIPT)
		host = owner->lua;
	else
		throw std::runtime_error("Unknown script language in:" + sourcePath);
}

}

VCMI_LIB_NAMESPACE_END
