/*
 * action.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include "BAI/v15/graph/nodes/base_v15.h"
#include "BAI/v15/graph/nodes/hex_v15.h"
#include "BAI/v15/graph/nodes/unit_v15.h"
#include "battle/BattleSide.h"
#include "schema/v15/constants.h"
#include "schema/v15/graph.h"
#include "schema/v15/types.h"

namespace MMAI::BAI::V15::Graph::Nodes
{
namespace S15 = Schema::V15;

namespace detail
{
	using Action_Traits = S15::EncodingTraits<S15::Graph::NodeAttributes::Action>;
	using Action_Base = Base<Action_Traits>;
}

class Action : public detail::Action_Base
{
public:
	enum class Flag : uint8_t
	{
		RETURN_AFTER_STRIKE,
		_count
	};

	using Flags = std::bitset<EU(Flag::_count)>;

	struct Args
	{
		const S15::ActionType actionType;
		const std::shared_ptr<const Nodes::Unit> & by;
		const std::shared_ptr<const Nodes::Unit> & target;
		const std::vector<std::shared_ptr<const Nodes::Hex>> & endsAt;
		const Flags & flags;
	};

	static std::shared_ptr<const Action> Create(const Args & args)
	{
		return std::make_shared<const Action>(args);
	}

	explicit Action(const Args & args)
		: actionType(args.actionType)
		, by(args.by)
		, target(args.target)
		, endsAt(args.endsAt)
		, flags(args.flags)
		, isActive(args.by == nullptr || args.by->isActive) // RETREAT has no `by`, but is still active
	{
		setattr(A::ACTION_TYPE, EU(actionType));
		setattr(A::IS_ACTIVE, isActive);
	}

	std::string name() const override
	{
		// XXX: for action type, prefer explicit cast to int over EU() because enum type is uint8_t (char)
		auto type = static_cast<int>(actionType);

		std::stringstream ss;
		ss << detail::Action_Base::name() << "(type=" << type;
		switch(static_cast<S15::ActionType>(type))
		{
			case S15::ActionType::WAIT:
				ss << "(WAIT)";
				break;
			case S15::ActionType::DEFEND:
				ss << "(DEFEND)";
				break;
			case S15::ActionType::MOVE:
				ss << "(MOVE)";
				break;
			case S15::ActionType::AMOVE:
				ss << "(AMOVE)";
				break;
			case S15::ActionType::SHOOT:
				ss << "(SHOOT)";
				break;
			default:
				throwf("Unexpected action type: {}", type);
		}

		ss << ",by=" << by->name();
		ss << ",isActive=" << isActive;
		ss << ",endsAt=" << endsAt.front()->name();
		ss << ",flags=" << flags.to_string();
		ss << ",target=" << (target ? target->name() : "-");
		ss << ")";

		return ss.str();
	}

	std::string humanName(BattleSide side) const
	{
		const auto & endhex = endsAt.at(0);

		auto stackstr = [&side](const auto & unit)
		{
			std::string targetside = (side == BattleSide::LEFT_SIDE) ? "L" : "R";
			return targetside + "-" + unit->alias;
		};

		switch(actionType)
		{
			case S15::ActionType::WAIT:
				return "Wait";
			case S15::ActionType::DEFEND:
				return "Defend";
			case S15::ActionType::MOVE:
				return "Move to " + endhex->name();
			case S15::ActionType::AMOVE:
				ASSERT(target != nullptr, "AMOVE with no valid targets");
				return "Attack Stack(" + stackstr(target) + ") from " + endhex->name();
			case S15::ActionType::SHOOT:
				ASSERT(target != nullptr, "SHOOT with no valid targets");
				return "Attack Stack(" + stackstr(target) + ") from " + endhex->name();
			default:
				throwf("Unexpected action type: {}", EI(actionType));
		}
	}

	const S15::ActionType actionType;
	const std::shared_ptr<const Nodes::Unit> by;
	const std::shared_ptr<const Nodes::Unit> target;
	const std::vector<std::shared_ptr<const Nodes::Hex>> endsAt; // primary hex is always first
	const Flags flags;
	bool isActive;
};
}
