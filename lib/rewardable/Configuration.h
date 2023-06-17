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
#include "MetaString.h"
#include "NetPacksBase.h"
#include "Reward.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace Rewardable
{

enum EVisitMode
{
	VISIT_UNLIMITED, // any number of times. Side effect - object hover text won't contain visited/not visited text
	VISIT_ONCE,      // only once, first to visit get all the rewards
	VISIT_HERO,      // every hero can visit object once
	VISIT_BONUS,     // can be visited by any hero that don't have bonus from this object
	VISIT_PLAYER     // every player can visit object once
};

/// controls selection of reward granted to player
enum ESelectMode
{
	SELECT_FIRST,  // first reward that matches limiters
	SELECT_PLAYER, // player can select from all allowed rewards
	SELECT_RANDOM, // one random reward from all mathing limiters
};

enum class EEventType
{
	EVENT_INVALID = 0,
	EVENT_FIRST_VISIT,
	EVENT_ALREADY_VISITED,
	EVENT_NOT_AVAILABLE
};

const std::array<std::string, 3> SelectModeString{"selectFirst", "selectPlayer", "selectRandom"};
const std::array<std::string, 5> VisitModeString{"unlimited", "once", "hero", "bonus", "player"};

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

	template <typename Handler> void serialize(Handler &h, const int version)
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

	/// Event to which this reward is assigned
	EEventType visitType;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & limiter;
		h & reward;
		h & message;
		h & visitType;
	}
};

/// Base class that can handle granting rewards to visiting heroes.
struct DLL_LINKAGE Configuration
{
	/// Message that will be shown if player needs to select one of multiple rewards
	MetaString onSelect;

	/// Rewards that can be applied by an object
	std::vector<Rewardable::VisitInfo> info;

	/// how reward will be selected, uses ESelectMode enum
	ui8 selectMode = Rewardable::SELECT_FIRST;

	/// contols who can visit an object, uses EVisitMode enum
	ui8 visitMode = Rewardable::VISIT_UNLIMITED;

	/// how and when should the object be reset
	Rewardable::ResetInfo resetParameters;

	/// if true - player can refuse visiting an object (e.g. Tomb)
	bool canRefuse = false;

	/// if true - object info will shown in infobox (like resource pickup)
	EInfoWindowMode infoWindowType = EInfoWindowMode::AUTO;
	
	EVisitMode getVisitMode() const;
	ui16 getResetDuration() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & info;
		h & canRefuse;
		h & resetParameters;
		h & onSelect;
		h & visitMode;
		h & selectMode;
		h & infoWindowType;
	}
};

}

VCMI_LIB_NAMESPACE_END
