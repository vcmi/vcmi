/*
 * FuzzyHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/
#pragma once
#include "FuzzyEngines.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBank;

VCMI_LIB_NAMESPACE_END

class DLL_EXPORT FuzzyHelper
{
public:
	TacticalAdvantageEngine tacticalAdvantageEngine;
	VisitTileEngine visitTileEngine;
	VisitObjEngine visitObjEngine;

	float evaluate(Goals::Explore & g);
	float evaluate(Goals::RecruitHero & g);
	float evaluate(Goals::VisitTile & g);
	float evaluate(Goals::VisitObj & g);
	float evaluate(Goals::VisitHero & g);
	float evaluate(Goals::BuildThis & g);
	float evaluate(Goals::DigAtTile & g);
	float evaluate(Goals::CollectRes & g);
	float evaluate(Goals::Build & g);
	float evaluate(Goals::BuyArmy & g);
	float evaluate(Goals::BuildBoat & g);
	float evaluate(Goals::GatherArmy & g);
	float evaluate(Goals::ClearWayTo & g);
	float evaluate(Goals::CompleteQuest & g);
	float evaluate(Goals::AdventureSpellCast & g);
	float evaluate(Goals::Invalid & g);
	float evaluate(Goals::AbstractGoal & g);
	void setPriority(Goals::TSubgoal & g);

	ui64 estimateBankDanger(const CBank * bank); //TODO: move to another class?

	Goals::TSubgoal chooseSolution(Goals::TGoalVec vec);
	//std::shared_ptr<AbstractGoal> chooseSolution (std::vector<std::shared_ptr<AbstractGoal>> & vec);

	ui64 evaluateDanger(const CGObjectInstance * obj, const VCAI * ai);
	ui64 evaluateDanger(crint3 tile, const CGHeroInstance * visitor, const VCAI * ai);
	ui64 evaluateDanger(crint3 tile, const CGHeroInstance * visitor);
};

extern FuzzyHelper * fh;
