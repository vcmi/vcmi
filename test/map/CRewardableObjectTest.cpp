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
#include "../../lib/mapObjects/CRewardableObject.h"

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
