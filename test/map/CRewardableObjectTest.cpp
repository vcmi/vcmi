/*
 * CRewardableObjectTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/mock_IGameInfoCallback.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/callback/CGameInfoCallback.h"
#include "../../lib/mapObjects/CRewardableObject.h"

namespace
{

class GameInfoCallbackForTest : public CGameInfoCallback
{
public:
	explicit GameInfoCallbackForTest(PlayerColor player)
		: player(player)
	{
	}

	CGameState & gameState() override
	{
		throw std::logic_error("Unexpected gameState access");
	}

	const CGameState & gameState() const override
	{
		throw std::logic_error("Unexpected gameState access");
	}

	std::optional<PlayerColor> getPlayerID() const override
	{
		return player;
	}

private:
	PlayerColor player;
};

}

TEST(CRewardableObject, rewardValuesRespectScoutingVisibility)
{
	const PlayerColor visitor(0);
	IGameInfoCallbackMock objectCallback;
	TeamState team;
	CRewardableObject object(&objectCallback);
	GameInfoCallbackForTest callback(visitor);
	RewardableObjectInfo info;
	object.id = ObjectInstanceID(42); // Arbitrary rewardable object instance used to track scouting.
	object.configuration.info.emplace_back();
	object.configuration.info.back().visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	object.configuration.info.back().reward.resources[GameResID::GOLD] = 1000;
	object.configuration.info.back().reward.grantedArtifacts.push_back(ArtifactID::SPELLBOOK);
	object.configuration.showScoutedPreview = true;

	EXPECT_CALL(objectCallback, getPlayerTeam(visitor)).WillRepeatedly(::testing::Return(&team));

	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info));
	EXPECT_FALSE(info.scouted);
	EXPECT_FALSE(info.rewardAvailable);
	EXPECT_FALSE(info.rewardValueKnown);
	EXPECT_EQ(info.guardStrength, 0);
	EXPECT_TRUE(info.rewards.empty());

	team.scoutedObjects.insert(object.id);
	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info));
	EXPECT_TRUE(info.scouted);
	EXPECT_TRUE(info.rewardAvailable);
	EXPECT_TRUE(info.rewardValueKnown);
	ASSERT_EQ(info.rewards.size(), 1);
	EXPECT_EQ(info.rewards.front().resources[GameResID::GOLD], 1000);
	EXPECT_EQ(info.rewards.front().grantedArtifacts, std::vector<ArtifactID>{ArtifactID::SPELLBOOK});

	object.configuration.info.back().reward.guards.emplace_back(CreatureID::NONE, 1);
	EXPECT_TRUE(callback.getRewardableObjectInfo(&object, info));
	EXPECT_FALSE(info.rewardValueKnown);
	ASSERT_EQ(info.rewards.size(), 1);
	EXPECT_EQ(info.rewards.front().resources[GameResID::GOLD], 1000);
	EXPECT_TRUE(info.rewards.front().grantedArtifacts.empty());
}

TEST(CRewardableObject, onceVisitStateIsKnownOnlyToScoutingTeam)
{
	const PlayerColor visitor(0);
	const PlayerColor ally(1);
	const PlayerColor enemy(2);

	IGameInfoCallbackMock callback;
	TeamState visitorsTeam;
	TeamState enemyTeam;
	CRewardableObject object(&callback);
	object.id = ObjectInstanceID(42); // Arbitrary rewardable object instance used to track scouting.
	object.configuration.visitMode = Rewardable::VISIT_ONCE;
	object.setPropertyDer(ObjProperty::REWARD_CLEARED, ObjPropertyID(NumericID(1)));
	visitorsTeam.scoutedObjects.insert(object.id);

	EXPECT_CALL(callback, getPlayerTeam(visitor)).WillOnce(::testing::Return(&visitorsTeam));
	EXPECT_CALL(callback, getPlayerTeam(ally)).WillOnce(::testing::Return(&visitorsTeam));
	EXPECT_CALL(callback, getPlayerTeam(enemy)).WillOnce(::testing::Return(&enemyTeam));

	EXPECT_TRUE(object.wasVisited(visitor));
	EXPECT_TRUE(object.wasVisited(ally));
	EXPECT_FALSE(object.wasVisited(enemy));
}
