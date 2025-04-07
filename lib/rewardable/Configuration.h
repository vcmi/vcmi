/*
 * Configuration.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Limiter.h"
#include "Reward.h"
#include "../networkPacks/EInfoWindowMode.h"
#include "../texts/MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace Rewardable
{

enum EVisitMode
{
	VISIT_UNLIMITED, // any number of times. Side effect - object hover text won't contain visited/not visited text
	VISIT_ONCE,      // only once, first to visit get all the rewards
	VISIT_HERO,      // every hero can visit object once
	VISIT_BONUS,     // can be visited by any hero that don't have bonus from this object
	VISIT_LIMITER,   // can be visited by heroes that don't fulfill provided limiter
	VISIT_PLAYER     // every player can visit object once
};

/// controls selection of reward granted to player
enum ESelectMode
{
	SELECT_FIRST,  // first reward that matches limiters
	SELECT_PLAYER, // player can select from all allowed rewards
	SELECT_RANDOM, // one random reward from all matching limiters
	SELECT_ALL // grant all rewards that match limiters
};

enum class EEventType
{
	EVENT_INVALID = 0,
	EVENT_FIRST_VISIT,
	EVENT_ALREADY_VISITED,
	EVENT_NOT_AVAILABLE,
	EVENT_GUARDED
};

constexpr std::array<std::string_view, 4> SelectModeString{"selectFirst", "selectPlayer", "selectRandom", "selectAll"};
constexpr std::array<std::string_view, 6> VisitModeString{"unlimited", "once", "hero", "bonus", "limiter", "player"};

struct DLL_LINKAGE ResetInfo
{
	ResetInfo()
		: period(0)
		, visitors(false)
		, rewards(false)
	{}

	/// if above zero, object state will be reset each resetDuration days
	ui32 period;

	/// if true - reset list of visitors (heroes & players) on reset
	bool visitors;

	/// if true - re-randomize rewards on a new week
	bool rewards;
	
	void serializeJson(JsonSerializeFormat & handler);
	
	template <typename Handler> void serialize(Handler &h)
	{
		h & period;
		h & visitors;
		h & rewards;
	}
};

struct DLL_LINKAGE VisitInfo
{
	Limiter limiter;
	Reward reward;

	/// Message that will be displayed on granting of this reward, if not empty
	MetaString message;

	/// Object description that will be shown on right-click, after object name
	/// Used only after player have "scouted" object and knows internal state of an object
	MetaString description;

	/// Event to which this reward is assigned
	EEventType visitType;

	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h)
	{
		h & limiter;
		h & reward;
		h & message;
		h & description;
		h & visitType;
	}
};

struct DLL_LINKAGE Variables
{
	/// List of variables used by this object in their current values
	std::map<std::string, int> values;

	/// List of per-instance preconfigured variables, e.g. from map
	std::map<std::string, JsonNode> preset;

	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h)
	{
		h & values;
		h & preset;
	}
};

/// Base class that can handle granting rewards to visiting heroes.
struct DLL_LINKAGE Configuration
{
	/// Message that will be shown if player needs to select one of multiple rewards
	MetaString onSelect;

	/// Object description that will be shown on right-click, after object name
	/// Used only if player is not aware of object internal state, e.g. have never visited it
	MetaString description;

	/// Text that will be shown if hero has not visited this object
	MetaString notVisitedTooltip;

	/// Text that will be shown after hero has visited this object
	MetaString visitedTooltip;

	/// Rewards that can be applied by an object
	std::vector<Rewardable::VisitInfo> info;

	/// how reward will be selected, uses ESelectMode enum
	ui8 selectMode = Rewardable::SELECT_FIRST;

	/// controls who can visit an object, uses EVisitMode enum
	ui8 visitMode = Rewardable::VISIT_UNLIMITED;

	/// how and when should the object be reset
	Rewardable::ResetInfo resetParameters;

	/// List of variables shoread between all limiters and rewards
	Rewardable::Variables variables;

	/// Limiter that will be used to determine that object is visited. Only if visit mode is set to "limiter"
	Rewardable::Limiter visitLimiter;

	std::string guardsLayout;

	/// if true - player can refuse visiting an object (e.g. Tomb)
	bool canRefuse = false;

	/// if true - right-clicking object will show preview of object rewards
	bool showScoutedPreview = false;

	bool coastVisitable = false;

	/// if true - object info will shown in infobox (like resource pickup)
	EInfoWindowMode infoWindowType = EInfoWindowMode::AUTO;
	
	EVisitMode getVisitMode() const;
	ui16 getResetDuration() const;

	std::optional<int> getVariable(const std::string & category, const std::string & name) const;
	const JsonNode & getPresetVariable(const std::string & category, const std::string & name) const;
	void presetVariable(const std::string & category, const std::string & name, const JsonNode & value);
	void initVariable(const std::string & category, const std::string & name, int value);
	
	void serializeJson(JsonSerializeFormat & handler);
	
	template <typename Handler> void serialize(Handler &h)
	{
		h & onSelect;
		h & description;
		h & notVisitedTooltip;
		h & visitedTooltip;
		h & info;
		h & selectMode;
		h & visitMode;
		h & resetParameters;
		h & variables;
		h & visitLimiter;
		h & canRefuse;
		h & showScoutedPreview;
		h & infoWindowType;
		h & coastVisitable;
		h & guardsLayout;
	}
};

}

VCMI_LIB_NAMESPACE_END
