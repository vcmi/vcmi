/*
 * unit.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CCreatureHandler.h"
#include "CStack.h"

#include "BAI/v15/fastbfs.h"
#include "BAI/v15/graph/nodes/base.h"
#include "schema/v15/constants.h"
#include "schema/v15/graph.h"

namespace MMAI::BAI::V15::Graph::Nodes
{
namespace S15 = Schema::V15;

namespace detail
{
	using Unit_Traits = S15::EncodingTraits<S15::Graph::NodeAttributes::Unit>;
	using Unit_Base = Base<Unit_Traits>;
}

class Unit : public detail::Unit_Base
{
	using GA = S15::Graph::NodeAttributes::Global;
	using UA = S15::Graph::NodeAttributes::Unit;

public:
	struct extra_index_type
	{
		using result_type = uint32_t;
		result_type operator()(const std::shared_ptr<const Unit> & unit) const
		{
			return unit->cstack.unitId();
		}
	};

	struct Args
	{
		const CStack & cstack;
		const bool isActive;
		const bool isEnemy;
		const bool isFlying;
		const int speed;
		const int bfieldValue;
		const FastBFS::Distances & distances;
	};

	static std::shared_ptr<const Unit> Create(const Args & args)
	{
		return std::make_shared<const Unit>(args);
	}

	explicit Unit(const Args & args);

	std::string name() const override
	{
		std::stringstream ss;
		ss << detail::Unit_Base::name();
		ss << "(isActive=" << isActive;
		ss << ",owner=" << cstack.unitOwner().toString();
		ss << ",slot=" << cstack.unitSlot();
		ss << ",count=" << cstack.getCount();
		ss << ",type=" << cstack.unitType()->getJsonKey();
		ss << ")";

		return ss.str();
	}

	// Many remain unset, prevent validation errors for unset attributes
	// Unit state may change as a result of battle effects in-between rounds
	// => no use of guardflags => override to not set any

	int attr(Attribute a) const;
	void setattr(Attribute a, int value);

	static int GetValue(const CCreature * creature, bool isClone = false, bool isSummon = false);

	const CStack & cstack;
	const std::string alias;
	const FastBFS::Distances distances;
	const bool isActive;
	const bool isFlying;
	const int speed;
	const int valueOne;

private:
	void processBonuses();
};

}
