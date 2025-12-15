/*
 * stack.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CStack.h"
#include "battle/IBattleInfoCallback.h"

#include "BAI/v13/global_stats.h"
#include "battle/ReachabilityInfo.h"
#include "schema/v13/constants.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13
{

namespace S13 = Schema::V13;
using StackAttribute = Schema::V13::StackAttribute;
using StackAttrs = Schema::V13::StackAttrs;
using StackFlag1 = Schema::V13::StackFlag1;
using StackFlag2 = Schema::V13::StackFlag2;
using StackFlags1 = Schema::V13::StackFlags1;
using StackFlags2 = Schema::V13::StackFlags2;

using Queue = std::vector<uint32_t>; // item=unit id
using BitQueue = std::bitset<S13::STACK_QUEUE_SIZE>;

static_assert(1 << S13::STACK_QUEUE_SIZE < std::numeric_limits<int>::max(), "BitQueue must be convertible to int");

/*
 * A wrapper around CStack
 */
class Stack : public Schema::V13::IStack
{
public:
	static int GetValue(const CCreature * creature);

	// not the quantum version :)
	static std::pair<BitQueue, int> QBits(const CStack *, const Queue &);

	struct Stats
	{
		int dmgDealtNow = 0;
		int dmgDealtTotal = 0;
		int dmgReceivedNow = 0;
		int dmgReceivedTotal = 0;
		int valueKilledNow = 0;
		int valueKilledTotal = 0;
		int valueLostNow = 0;
		int valueLostTotal = 0;
	};

	// struct for reducing constructor args to avoid sonarcloud warning...
	struct StatsContainer
	{
		const GlobalStats * oldgstats;
		const GlobalStats * gstats;
		const Stats stackStats;
	};

	Stack(
		const CStack * cstack,
		const Queue & q,
		const StatsContainer & statsContainer,
		const ReachabilityInfo & rinfo,
		bool blocked,
		bool blocking,
		const DamageEstimation & estdmg
	);

	// IStack impl
	const StackAttrs & getAttrs() const override;
	int getAttr(StackAttribute a) const override;
	int getFlag(StackFlag1 sf) const override;
	int getFlag(StackFlag2 sf) const override;
	char getAlias() const override;
	char alias;

	const CStack * const cstack;
	const ReachabilityInfo rinfo;
	StackAttrs attrs = {};
	StackFlags1 flags1 = 0; //
	StackFlags2 flags2 = 0; //

	int attr(StackAttribute a) const;
	bool flag(StackFlag1 f) const;
	bool flag(StackFlag2 f) const;
	int shots;
	int qposFirst;

private:
	void setflag(StackFlag1 f);
	void setflag(StackFlag2 f);
	void setattr(StackAttribute a, int value);
	void addattr(StackAttribute a, int value);
	void finalize();

	void processBonuses();
};
}
