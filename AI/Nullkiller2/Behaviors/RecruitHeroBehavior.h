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
#include "lib/ResourceSet.h"
#include "lib/constants/NumericConstants.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGTownInstance.h"

#include <algorithm>

namespace NK2AI
{
namespace Goals
{
	struct RecruitHeroChoice
	{
		mutable float score = 0;
		mutable const CGHeroInstance * hero = nullptr;
		mutable const CGTownInstance * town = nullptr;
		mutable bool defensiveEmergency = false;
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
			const std::vector<HitMapInfo> & townThreats,
			float safeAttackRatio,
			float visitabilityRatio
		);

		/// Returns true when recruiting a hero is urgent enough for town defence.
		/// Same-turn threats require the hero to cover the known danger.
		/// Next-turn threats must be meaningful, stronger than the stable
		/// town defence at safeAttackRatio, and covered by the recruited hero.
		/// Visiting heroes are mobile and must be locked separately to count.
		static bool isDefensiveRecruitEmergency(
			const CGTownInstance & town,
			const CGHeroInstance & hero,
			const HitMapInfo & threat,
			float safeAttackRatio
		)
		{
			if(threat.danger == 0 || threat.turn > 1)
				return false;

			const auto currentDefence = town.getUpperArmy()->getArmyStrength();
			const auto recruitedDefence = hero.getTotalStrength();

			if(threat.turn == 0)
				return recruitedDefence >= threat.danger;

			const auto requiredDefence = threat.danger * safeAttackRatio;
			if(currentDefence >= requiredDefence)
				return false;

			if(recruitedDefence < requiredDefence)
				return false;

			return threat.danger >= GameConstants::HERO_GOLD_COST / 2;
		}

		static bool shouldRecruitHero(
			size_t heroesCount,
			const RecruitHeroChoice & bestChoice,
			bool haveCapitol,
			int treasureSourcesCount,
			TResource freeGold,
			bool goldPressureOverMax
		)
		{
			if(bestChoice.hero == nullptr || vstd::isAlmostZero(bestChoice.score))
				return false;

			return heroesCount == 0
				   || treasureSourcesCount > heroesCount * 5
				   // TODO: Mircea: The next condition should always consider a hero if under attack especially if it has towers
				   || (bestChoice.hero->getArmyCost() > GameConstants::HERO_GOLD_COST / 2.0
					   && (bestChoice.defensiveEmergency || !goldPressureOverMax))
				   || (freeGold > 10000 && !goldPressureOverMax && haveCapitol)
				   || (freeGold > 30000 && !goldPressureOverMax);
		}

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
