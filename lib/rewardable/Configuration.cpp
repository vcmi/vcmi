/*
 * Configuration.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Configuration.h"
#include "../callback/Calendar.h"
#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

Rewardable::EVisitMode Rewardable::Configuration::getVisitMode() const
{
	return static_cast<EVisitMode>(visitMode);
}

ui32 Rewardable::Configuration::getResetDuration(const Calendar & calendar) const
{
	return resetParameters.days
		+ resetParameters.weeks * calendar.getDaysInWeek()
		+ resetParameters.months * calendar.getDaysInMonth();
}

std::optional<int> Rewardable::Configuration::getVariable(const std::string & category, const std::string & name) const
{
	std::string variableID = category + '@' + name;

	if (variables.values.count(variableID))
		return variables.values.at(variableID);

	return std::nullopt;
}

const JsonNode & Rewardable::Configuration::getPresetVariable(const std::string & category, const std::string & name) const
{
	static const JsonNode emptyNode;

	std::string variableID = category + '@' + name;

	if (variables.preset.count(variableID))
		return variables.preset.at(variableID);
	else
		return emptyNode;
}

void Rewardable::Configuration::presetVariable(const std::string & category, const std::string & name, const JsonNode & value)
{
	std::string variableID = category + '@' + name;
	variables.preset[variableID] = value;
}

void Rewardable::Configuration::initVariable(const std::string & category, const std::string & name, int value)
{
	std::string variableID = category + '@' + name;
	variables.values[variableID] = value;
}

void Rewardable::ResetInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("days", days);
	handler.serializeInt("weeks", weeks);
	handler.serializeInt("months", months);
	handler.serializeBool("visitors", visitors);
	handler.serializeBool("rewards", rewards);

	// MODS COMPATIBILITY accept legacy "period" field as alias for "days"
	if (!handler.saving && days == 0 && weeks == 0 && months == 0)
		days = static_cast<ui32>(handler.getCurrent()["period"].Float());
}

void Rewardable::VisitInfo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeStruct("limiter", limiter);
	handler.serializeStruct("reward", reward);
	handler.serializeStruct("message", message);
	handler.serializeInt("visitType", visitType);
}

void Rewardable::Variables::serializeJson(JsonSerializeFormat & handler)
{
	if (handler.saving)
	{
		JsonNode presetNode;
		for (auto const & entry : preset)
			presetNode[entry.first] = entry.second;

		handler.serializeRaw("preset", presetNode, {});
	}
	else
	{
		preset.clear();
		JsonNode presetNode;
		handler.serializeRaw("preset", presetNode, {});

		for (auto const & entry : presetNode.Struct())
			preset[entry.first] = entry.second;
	}
}

void Rewardable::Configuration::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeStruct("onSelect", onSelect);
	handler.enterArray("info").serializeStruct(info);
	handler.serializeEnum("selectMode", selectMode, std::vector<std::string>{SelectModeString.begin(), SelectModeString.end()});
	handler.serializeEnum("visitMode", visitMode, std::vector<std::string>{VisitModeString.begin(), VisitModeString.end()});
	handler.serializeStruct("resetParameters", resetParameters);
	handler.serializeBool("canRefuse", canRefuse);
	handler.serializeBool("showScoutedPreview", showScoutedPreview);
	handler.serializeBool("forceCombat", forceCombat);
	handler.serializeBool("coastVisitable", coastVisitable);
	handler.serializeInt("infoWindowType", infoWindowType);
	variables.serializeJson(handler);
}

VCMI_LIB_NAMESPACE_END
