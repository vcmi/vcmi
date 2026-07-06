/*
 * QuestScenarios.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

#include "../mock/TinyH3MBuilder.h"
#include "../../lib/GameConstants.h"
#include "../../lib/int3.h"

/// Pre-canned SOD scenarios for quest tests. Each factory returns a builder
/// that's one .build() away from a playable map plus the positions of the
/// objects it placed, so tests can locate what they want without scanning.
namespace QuestScenarios
{

// Identifiers used by scenario factories and loader tests. Hero/creature ids
// deliberately avoid 0/1 — zero-initialised state has been misread as "Orrin"
// or "pikeman" in the past, masking field-initialisation bugs.
constexpr HeroTypeID kHeroChristian       {6};   // knight
constexpr HeroTypeID kHeroTyris           {7};   // second knight for two-hero scenarios
constexpr CreatureID kCreatureGriffin     {4};
constexpr CreatureID kCreatureRoyalGriffin{5};
constexpr CreatureID kCreatureImp         {42};

constexpr ArtifactID kArtifactHelm        {36};  // helmOfHeavenlyEnlightenment (component of angelic alliance)
constexpr ArtifactID kArtifactSash        {68};  // ambassadorsSash
constexpr ArtifactID kArtifactAngelicAlly {129}; // angelicAlliance (combination artifact)

/// A scenario builder plus the named positions it placed. Factories override
/// only the fields they need; common positions default below.
struct Scenario
{
	TinyH3M::TinyH3MBuilder builder;

	int3 heroPos       {5, 5, 0};    ///< Visiting hero (red).
	int3 secondHeroPos {};           ///< Optional second hero (target / second visitor).
	int3 questPos      {10, 10, 0};  ///< Primary quest object (seer hut, quest guard, keymaster, ...).
	int3 questPos2     {};           ///< Optional second quest object (paired keymaster/border-guard, easy/hard seers, ...).
};

// ---- Seer-hut bring-X scenarios -------------------------------------------

Scenario seerArtifact();                          ///< heroPos: visitor with sash in backpack. questPos: seer hut.
Scenario seerArtifactAssembledInBackpack();       ///< heroPos: visitor with Angelic Alliance in backpack. questPos: seer hut asking for the Helm component.
Scenario seerArtifactAssembledEquipped();         ///< heroPos: visitor with Angelic Alliance equipped (right hand). questPos: seer hut asking for the Helm component.
Scenario seerArmy();                    ///< heroPos: visitor with garrison (10 pikemen + 5 archers). questPos: seer hut.
Scenario seerResources();               ///< heroPos: visitor. questPos: seer hut. Test grants resources post-load.
Scenario seerHero();                    ///< heroPos: visitor. secondHeroPos: target. questPos: seer hut.
Scenario seerPlayer();                  ///< heroPos: red visitor. secondHeroPos: blue hero. questPos: seer hut.
Scenario seerLevel();                   ///< heroPos: visitor (XP for L5). questPos: easy seer (>=3). questPos2: hard seer (>=10).
Scenario seerPrimarySkill();            ///< heroPos: visitor (attack=5). questPos: easy. questPos2: hard.

// ---- Kill-quest scenarios (cross-object handles) --------------------------

Scenario seerKillCreature();            ///< heroPos: visitor. secondHeroPos: monster position. questPos: seer hut.
Scenario seerKillHero();                ///< heroPos: red visitor. secondHeroPos: blue target hero. questPos: seer hut.

// ---- Timer / payload-shape scenarios --------------------------------------

Scenario seerTimeout();                 ///< heroPos: visitor. questPos: seer hut with lastDay=7, trivial resources limiter.
Scenario seerEmptyArmyToggle();         ///< heroPos: visitor with single 1-pikeman stack. questPos: seer hut (bring 1 pikeman, no reward creatures).

// ---- Quest Guard scenarios ------------------------------------------------

Scenario questGuard();                  ///< heroPos: visitor. questPos: quest guard with bring-1000-wood (resources granted post-load).

// ---- Border / Keymaster scenarios -----------------------------------------

Scenario questKeymasterTent();          ///< heroPos: visitor. questPos: keymaster tent of colour 0.
Scenario questBorderGuard();            ///< heroPos: visitor. questPos: keymaster. questPos2: border guard (matching colour).
Scenario questBorderGate();             ///< heroPos: visitor. questPos: keymaster. questPos2: border gate (matching colour).

} // namespace QuestScenarios
