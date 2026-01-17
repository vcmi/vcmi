/*
 * SpellTargetsEvaluatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/mock_spells_Mechanics.h"
#include "../mock/BattleFake.h"
#include "AI/BattleAI/SpellTargetsEvaluator.h"
#include "lib/battle/CBattleInfoCallback.h"
#include "lib/CStack.h"


namespace test
{
using namespace ::testing;
using namespace spells;
using PossiblePositions = std::vector<BattleHex>;

class CBattleInfoCallbackMock : public CBattleInfoCallback
{
public:
    MOCK_CONST_METHOD1(battleGetAllStacks, TStacks(bool));
    MOCK_CONST_METHOD0(getBattle, IBattleInfo*());
    MOCK_CONST_METHOD0(getPlayerID, std::optional<PlayerColor>());
};

class CStackMock : public CStack
{
public:
    MOCK_CONST_METHOD0(getPosition, BattleHex());
    MOCK_CONST_METHOD0(unitSide, BattleSide());
};


class SpellTargetEvaluatorTest : public ::testing::Test
{
public:

    MechanicsMock mechMock;
    CBattleInfoCallbackMock battleMock;
    TStacks allStacks;
    BattleSide casterSide = BattleSide::ATTACKER;
    BattleSide enemySide = BattleSide::DEFENDER;

    void SetUp() override
    {
        mechMock.casterSide = casterSide;
        ON_CALL(mechMock, battle()).WillByDefault(Return(&battleMock));
        ON_CALL(mechMock, canBeCastAt(_,_)).WillByDefault(Return(true));
    }

    void TearDown() override
    {
        for (const auto * stack : allStacks)
            delete stack;
        allStacks.clear();
    }

    void spellTargetTypes(std::vector<AimType> targetTypes)
    {
        ON_CALL(mechMock, getTargetTypes()).WillByDefault(Return(std::move(targetTypes)));
    }

    CStackMock * addStack(BattleHex position, bool isSuspectible = true)
    {
        auto * stack = new CStackMock();
        ON_CALL(*stack, getPosition()).WillByDefault(Return(position));
        allStacks.push_back(stack);

        if (!isSuspectible)
            ON_CALL(mechMock, canBeCastAt(Contains(Field(&Destination::hexValue, position)),_)).WillByDefault(Return(false));
        return stack;
    }

    void areAlliedStacks(std::vector<const CStackMock *> stacks)
    {
        for (const CStackMock * stack : stacks)
            ON_CALL(*stack, unitSide()).WillByDefault(Return(casterSide));
    }

    void areEnemyStacks(std::vector<const CStackMock *> stacks)
    {
        for (const CStackMock * stack : stacks)
            ON_CALL(*stack, unitSide()).WillByDefault(Return(enemySide));
    }

    void setAffectedStacksForCast(BattleHex position, std::vector<const CStack *> stacks)
    {
        ON_CALL(mechMock, getAffectedStacks(Contains(Field(&Destination::hexValue, position)))).WillByDefault(Return(stacks));
    }

    void confirmResults(std::vector<PossiblePositions> allRequiredCasts)
    {
        std::vector<Target> result = SpellTargetEvaluator::getViableTargets(&mechMock);
        basicCheck(result);
        ASSERT_EQ(result.size(), allRequiredCasts.size());

        std::vector<BattleHex> targetedHexes;
        targetedHexes.reserve(result.size());
        for (Target target : result)
            targetedHexes.push_back(target.front().hexValue);

        for (const PossiblePositions & requiredCast : allRequiredCasts)
            EXPECT_TRUE(containCommonValue(requiredCast, targetedHexes));
    }

    void basicCheck(std::vector<Target> & result)
    {
     for (const Target & target : result)
         EXPECT_EQ(target.size(), 1);       //multi-destination spells are not handled by targetEvaluator
    }


    template<typename T>
    bool containCommonValue(const std::vector<T> & v1, const std::vector<T> & v2)
    {
        for (T val1 : v1)
        {
            for (T val2 : v2)
            {
                if (val1 == val2)
                    return true;
            }
        }
        return false;
    }

};


TEST_F(SpellTargetEvaluatorTest, ReturnsEmptyIfMechanicsNullptr)
{
    spellTargetTypes({AimType::CREATURE});
    std::vector<Target> result = SpellTargetEvaluator::getViableTargets(nullptr);
    EXPECT_TRUE(result.empty());
}

TEST_F(SpellTargetEvaluatorTest, ReturnsEmptyIfMultiDestinationSpell)
{
    spellTargetTypes({AimType::CREATURE, AimType::LOCATION});
    std::vector<Target> result = SpellTargetEvaluator::getViableTargets(&mechMock);
    EXPECT_TRUE(result.empty());
}

TEST_F(SpellTargetEvaluatorTest, ReturnSingleEmptyDestinationIfTargetIsNone)
{
    spellTargetTypes({AimType::NO_TARGET});
    std::vector<Target> result = SpellTargetEvaluator::getViableTargets(&mechMock);
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.front().empty());
}

TEST_F(SpellTargetEvaluatorTest, ReturnsSuspectibleCreaturePositionsIfSpellTargetsCreatures)
{
    spellTargetTypes({AimType::CREATURE});

    addStack(BattleHex(1));
    addStack(BattleHex(2), false);
    addStack(BattleHex(3));
    ON_CALL(battleMock, battleGetAllStacks(Eq(true))).WillByDefault(Return(allStacks));

    confirmResults({{BattleHex(1)}, {BattleHex(3)}});
}

TEST_F(SpellTargetEvaluatorTest, ReturnsSuspectibleCreaturePositionsAndSingleRandomSurroundingHexForEachStackIfNeutralLocationSpell)
{
     spellTargetTypes({AimType::LOCATION});
     ON_CALL(mechMock, isNeutralSpell()).WillByDefault(Return(true));

     addStack(BattleHex(72));
     addStack(BattleHex(159), false);
     addStack(BattleHex(23));
     ON_CALL(battleMock, battleGetAllStacks(Eq(true))).WillByDefault(Return(allStacks));

     confirmResults(
                    {
                        {BattleHex(72)},  BattleHex(72).getAllNeighbouringTiles().toVector(),
                        BattleHex(159).getAllNeighbouringTiles().toVector(),
                        {BattleHex(23)}, BattleHex(23).getAllNeighbouringTiles().toVector()
                    });
}

TEST_F(SpellTargetEvaluatorTest, ReturnsOneCaseOfEachOptimalCastIfNegativeLocationSpell)
{
    spellTargetTypes({AimType::LOCATION});
    ON_CALL(mechMock, isNegativeSpell()).WillByDefault(Return(true));

    auto *enemyStack1 = addStack(BattleHex(90));
    auto *enemyStack2 = addStack(BattleHex(106));
    auto *enemyStack3 = addStack(BattleHex(1));
    auto *enemyStack4 = addStack(BattleHex(37));
    auto *enemyStack5 = addStack(BattleHex(41));

    areEnemyStacks({enemyStack1, enemyStack2, enemyStack3, enemyStack4, enemyStack5});

    auto *alliedStack1 = addStack(BattleHex(107));
    auto *alliedStack2 = addStack(BattleHex(19));

    areAlliedStacks({alliedStack1, alliedStack2});


    //optimal
    setAffectedStacksForCast(BattleHex(71), {enemyStack1, enemyStack2});
    setAffectedStacksForCast(BattleHex(88), {enemyStack1, enemyStack2});
    //suboptimal
    setAffectedStacksForCast(BattleHex(55), {enemyStack1});
    setAffectedStacksForCast(BattleHex(89), {enemyStack1, enemyStack2, alliedStack1});

    //optimal
    setAffectedStacksForCast(BattleHex(18), {enemyStack3, alliedStack2});
    setAffectedStacksForCast(BattleHex(2), {enemyStack3, alliedStack2});
    //suboptimal
    setAffectedStacksForCast(BattleHex(53), {alliedStack2});

    //optimal
    setAffectedStacksForCast(BattleHex(39), {enemyStack4, enemyStack5});
    //suboptimal
    setAffectedStacksForCast(BattleHex(21), {enemyStack4});
    setAffectedStacksForCast(BattleHex(25), {enemyStack5});

    confirmResults(
                {
                    {BattleHex(71), BattleHex(88)},
                    {BattleHex(2), BattleHex(18)},
                    {BattleHex(39)}
                });
}

}




