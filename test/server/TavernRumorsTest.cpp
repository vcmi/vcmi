/*
 * TavernRumorsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CGameHandler.h"

#include "mock/GameHandlerTestServer.h"
#include "mock/TinyMapGameTest.h"

#include "lib/GameLibrary.h"
#include "lib/entities/RumorTypeHandler.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapping/CMap.h"

class TavernRumorsTest : public TinyMapGameTest
{
protected:
	const Services * gameServices() override { return LIBRARY; }

	void startWithTown(FactionID faction, bool twoLevel = false)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, twoLevel)
			.playerActive(PlayerColor(0))
			.town({10, 10, 0}, faction, PlayerColor::NEUTRAL);
		startWithMap(std::move(builder));
	}

	/// Returns rumors picked at the start of each week
	std::vector<MetaString> tellRumors(int weeks)
	{
		GameHandlerTestServer server(gameState());
		CGameHandler gameHandler(server, gameState());

		std::vector<MetaString> result;
		for(int day = 0; day < weeks * 7; ++day)
		{
			gameHandler.onNewTurn();
			if(gameState()->getCalendar().getDayOfWeek() == 1)
				result.push_back(gameState()->currentRumor.text);
		}
		return result;
	}
};

TEST_F(TavernRumorsTest, genericRumorsOfFactionsOnMapAreToldWithoutRepeats)
{
	startWithTown(FactionID::CASTLE);

	std::vector<std::pair<RumorId, MetaString>> allRumors;
	for(const auto & rumorID : LIBRARY->rumorHandler->getAllObjects())
		allRumors.emplace_back(rumorID, MetaString::createFromTextID(rumorID.toEntity(LIBRARY)->getNameTextID()));

	std::set<RumorId> eligible;
	for(const auto & [rumorID, text] : allRumors)
	{
		FactionID faction = rumorID.toEntity(LIBRARY)->getFaction();
		if(faction == FactionID::NONE || faction == FactionID::CASTLE)
			eligible.insert(rumorID);
	}

	std::vector<RumorId> told;
	for(const auto & text : tellRumors(200))
		for(const auto & [rumorID, rumorText] : allRumors)
			if(rumorText == text)
				told.push_back(rumorID);

	ASSERT_GE(told.size(), eligible.size());
	std::set<RumorId> firstPass(told.begin(), told.begin() + eligible.size());
	EXPECT_EQ(firstPass, eligible);
}

TEST_F(TavernRumorsTest, mapRumorsAreToldWithoutRepeats)
{
	startWithTown(FactionID::CASTLE);
	for(const std::string text : {"A", "B", "C"})
		map()->rumors.emplace_back().text = MetaString::createFromRawString(text);

	std::vector<int32_t> told;
	for(const auto & text : tellRumors(100))
		for(int32_t i = 0; i < map()->rumors.size(); ++i)
			if(map()->rumors[i].text == text)
				told.push_back(i);

	const std::set<int32_t> allRumors = {0, 1, 2};
	ASSERT_GE(told.size(), 6);
	EXPECT_EQ(std::set<int32_t>(told.begin(), told.begin() + 3), allRumors);
	EXPECT_EQ(std::set<int32_t>(told.begin() + 3, told.begin() + 6), allRumors);
}

TEST_F(TavernRumorsTest, grailLocationRumorNamesRegionAndLayer)
{
	startWithTown(FactionID::CASTLE, true);
	map()->grailPos = int3(30, 5, 1);

	MetaString expected = MetaString::createFromTextID("core.genrltxt.264");
	expected.replaceTextID("core.arraytxt", 149);

	auto weeks = tellRumors(200);
	EXPECT_NE(std::find(weeks.begin(), weeks.end(), expected), weeks.end());
}
