/*
* ArmyUpgrade.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Goals/CGoal.h"
#include "../Pathfinding/AINodeStorage.h"
#include "../Analyzers/ArmyManager.h"
#include "../Analyzers/DangerHitMapAnalyzer.h"

namespace Goals
{
	class DLL_EXPORT DefendTown : public CGoal<DefendTown>
	{
	private:
		uint64_t defenceArmyStrength;
		HitMapInfo treat;
		uint8_t turn;

	public:
		DefendTown(const CGTownInstance * town, const HitMapInfo & treat, const AIPath & defencePath);
		DefendTown(const CGTownInstance * town, const HitMapInfo & treat, const CGHeroInstance * defender);

		virtual bool operator==(const DefendTown & other) const override;
		virtual std::string toString() const override;

		const HitMapInfo & getTreat() const { return treat; }

		uint64_t getDefenceStrength() const { return defenceArmyStrength; }

		uint8_t getTurn() const { return turn; }
	};
}