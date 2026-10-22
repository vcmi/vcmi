/*
 * AIGatewayDialogTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "NullkillerTest.h"

#include "lib/AsyncRunner.h"
#include "lib/callback/IClient.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);

/// Records answers that AI sends to the server
class AnswerRecordingClient : public IClient
{
public:
	std::vector<QuestionAnswer> answers;

	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		if(const auto * answer = dynamic_cast<const QuestionAnswer *>(&request))
			answers.push_back(*answer);
		return static_cast<int>(answers.size());
	}
};

class Nullkiller2_AIGatewayDialog : public NullkillerTest
{
};
}

/// Dialogs such as commander resurrection are not caused by a visit, so AI has no target tile for them
TEST_F(Nullkiller2_AIGatewayDialog, answersDialogWithoutTargetTile)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false).playerActive(PLAYER).hero({5, 5, 0}, HeroTypeID(0), PLAYER);
	startWithMap(std::move(builder));

	AnswerRecordingClient client;
	auto gateway = makeGateway(PLAYER, &client);

	gateway->showBlockingDialog("resurrect commander?", {}, QuestionID(55), 0, false, true, false);
	gateway->asyncTasks->wait();

	ASSERT_EQ(client.answers.size(), 1u);
	EXPECT_EQ(client.answers.front().questionID, QuestionID(55));
	EXPECT_EQ(client.answers.front().reply, 1);

	// server would resolve the question once it applies the answer
	gateway->status.removeQuestion(QuestionID(55));
}
