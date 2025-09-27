/*
* RecruitHeroBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../AIUtility.h"
#include "../Analyzers/DangerHitMapAnalyzer.h"
#include "../Analyzers/HeroManager.h"
#include "../Goals/CGoal.h"
#include "lib/GameLibrary.h"

namespace NK2AI
{
namespace Goals
{
	struct RecruitHeroChoice
	{
		mutable float score = 0;
		mutable const CGHeroInstance * hero = nullptr;
		mutable const CGTownInstance * town = nullptr;
		mutable int closestThreat = 0;
	};

	class RecruitHeroBehavior : public CGoal<RecruitHeroBehavior>
	{
	public:
		RecruitHeroBehavior() : CGoal(RECRUIT_HERO_BEHAVIOR) {}
		~RecruitHeroBehavior() override = default;

		TGoalVec decompose(const Nullkiller * aiNk) const override;
		std::string toString() const override;

		bool operator==(const RecruitHeroBehavior & other) const override
		{
			return true; // TODO: Mircea: How does that make sense?
		}

		static void calculateTreasureSources(
			const std::vector<const CGObjectInstance *> & nearbyObjects,
			const PlayerColor & playerID,
			const DangerHitMapAnalyzer & dangerHitMap,
			int & treasureSourcesCount,
			const CGTownInstance & town
		);
		static void calculateBestHero(
			const std::vector<const CGHeroInstance *> & availableHeroes,
			const HeroManager & heroManager,
			const RecruitHeroChoice & bestChoice,
			const CGTownInstance & town,
			uint8_t closestThreatTurn,
			float visitabilityRatio
		);
		static void calculateFinalDecision(
			const Nullkiller & aiNk,
			Goals::TGoalVec & tasks,
			const std::vector<const CGHeroInstance *> & ourHeroes,
			const RecruitHeroChoice & bestChoice,
			bool haveCapitol,
			int treasureSourcesCount
		);
	};
}

}
