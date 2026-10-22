/*
 * WarMachineResurrectionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../server/CGameHandler.h"
#include "../../../server/battles/BattleResultProcessor.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/battle/BattleInfo.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/spells/Problem.h"

/// A destroyed war machine brought back by a spell is alive again, and keeps its artifact after the
/// battle. Double-wide war machines stand partially on the side column of the battlefield, which no
/// unit can enter, and that must not stop them from being resurrected.
class WarMachineResurrectionTest : public BattleTestFixture
{
public:
	SpellID spellID;

	CStack * findStack(CreatureID creature) const
	{
		for(const auto & unit : battle()->stacks)
			if(unit->creatureId() == creature)
				return unit.get();
		return nullptr;
	}

	CStack * startBattleWithDestroyedMachine(CreatureID creature)
	{
		startGame();
		giveArtifact(attackerSideHero, ArtifactID(ArtifactID::BALLISTA), ArtifactPosition::MACH1);
		giveArtifact(attackerSideHero, ArtifactID(ArtifactID::FIRST_AID_TENT), ArtifactPosition::MACH3);
		startBattle();

		spellID = spellByName("vcmi-test:testRestoreWarMachine");
		teachSpell(attackerSideHero, spellID);

		CStack * machine = findStack(creature);
		if(machine)
			injure(machine, machine->getAvailableHealth());
		return machine;
	}

	/// Casts the spell if it can be cast at the machine, returns whether it could
	bool resurrect(const CStack * machine)
	{
		spells::BattleCast cast(battle(), attackerSideHero, spells::Mode::HERO, spellID.toSpell());
		spells::Target destination;
		destination.emplace_back(machine->getPosition());

		spells::detail::ProblemImpl problem;
		auto mechanics = spellID.toSpell()->battleMechanics(&cast);
		if(!mechanics->canBeCastAt(destination, problem))
			return false;
		cast.cast(gameHandler->spellEnv.get(), destination);
		return true;
	}
};

class DoubleWideWarMachineResurrectionTest : public WarMachineResurrectionTest, public ::testing::WithParamInterface<int>
{
};

TEST_P(DoubleWideWarMachineResurrectionTest, ResurrectedWarMachineSurvivesBattle)
{
	CStack * machine = startBattleWithDestroyedMachine(CreatureID(GetParam()));
	ASSERT_NE(machine, nullptr);
	ASSERT_FALSE(machine->alive());

	ASSERT_TRUE(resurrect(machine));

	EXPECT_TRUE(machine->alive());
	EXPECT_EQ(machine->getCount(), 1);

	CasualtiesAfterBattle casualties(*battle(), BattleSide::ATTACKER);
	EXPECT_TRUE(casualties.removedWarMachines.empty());
}

INSTANTIATE_TEST_SUITE_P(WarMachines, DoubleWideWarMachineResurrectionTest, ::testing::Values(CreatureID::BALLISTA, CreatureID::FIRST_AID_TENT));

TEST_F(WarMachineResurrectionTest, WarMachineUnderForceFieldIsNotResurrected)
{
	CStack * machine = startBattleWithDestroyedMachine(CreatureID::BALLISTA);
	ASSERT_NE(machine, nullptr);
	addForceField(machine->getPosition());

	EXPECT_FALSE(resurrect(machine));
	EXPECT_FALSE(machine->alive());
}
