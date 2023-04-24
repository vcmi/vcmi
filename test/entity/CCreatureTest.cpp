/*
 * CCreatureTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/CCreatureHandler.h"

namespace test
{

using namespace ::testing;

class CCreatureTest : public Test
{
public:
	MOCK_METHOD4(registarCb, void(int32_t, int32_t, const std::string &, const std::string &));
protected:
	std::shared_ptr<CCreature> subject;

	void SetUp() override
	{
		subject = std::make_shared<CCreature>();
	}
};

TEST_F(CCreatureTest, RegistersIcons)
{
	subject->smallIconName = "Test1";
	subject->largeIconName = "Test2";

	auto cb = std::bind(&CCreatureTest::registarCb, this, _1, _2, _3, _4);

	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "CPRSMALL", "Test1"));
	EXPECT_CALL(*this, registarCb(Eq(4242), Eq(0), "TWCRPORT", "Test2"));

	subject->registerIcons(cb);
}

TEST_F(CCreatureTest, JsonUpdate)
{
	JsonNode data(JsonNode::JsonType::DATA_STRUCT);

	JsonNode & config = data["config"];
	config["cost"]["gold"].Integer() = 750;
	config["attack"].Integer() = 17;
	config["defense"].Integer() = 15;
	config["hitPoints"].Integer() = 50;

	config["speed"].Integer() = 9;
	config["damage"]["min"].Integer() = 12;
	config["damage"]["max"].Integer() = 20;
	config["shots"].Integer() = 100;

	config["growth"].Integer() = 1;
	config["spellPoints"].Integer() = 2;
	config["horde"].Integer() = 123;
	config["aiValue"].Integer() = 3388;

	config["fightValue"].Integer() = 2420;
	config["level"].Integer() = 6;
	config["faction"].Integer() = 55;
	config["doubleWide"].Bool() = true;

	subject->updateFrom(data);

	EXPECT_EQ(subject->getRecruitCost(EGameResID::GOLD), 750);
	EXPECT_EQ(subject->getBaseAttack(), 17);
	EXPECT_EQ(subject->getAttack(false), 17);
	EXPECT_EQ(subject->getAttack(true), 17);

	EXPECT_EQ(subject->getBaseDefense(), 15);
	EXPECT_EQ(subject->getDefense(false), 15);
	EXPECT_EQ(subject->getDefense(true), 15);
	EXPECT_EQ(subject->getBaseHitPoints(), 50);

	EXPECT_EQ(subject->getBaseSpeed(), 9);
	EXPECT_EQ(subject->getBaseDamageMin(), 12);
	EXPECT_EQ(subject->getMinDamage(false), 12);
	EXPECT_EQ(subject->getMinDamage(true), 12);
	EXPECT_EQ(subject->getBaseDamageMax(), 20);
	EXPECT_EQ(subject->getMaxDamage(false), 20);
	EXPECT_EQ(subject->getMaxDamage(true), 20);
	EXPECT_EQ(subject->getBaseShots(), 100);

	EXPECT_EQ(subject->getGrowth(), 1);
	EXPECT_EQ(subject->getBaseSpellPoints(), 2);
	EXPECT_EQ(subject->getHorde(), 123);
	EXPECT_EQ(subject->getAIValue(), 3388);

	EXPECT_EQ(subject->getFightValue(), 2420);
	EXPECT_EQ(subject->getLevel(), 6);
	EXPECT_EQ(subject->getFaction(), 55);
	EXPECT_TRUE(subject->isDoubleWide());
}

TEST_F(CCreatureTest, JsonAddBonus)
{
	JsonNode data(JsonNode::JsonType::DATA_STRUCT);

	std::shared_ptr<Bonus> b = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::BLOCKS_RETALIATION, Bonus::CREATURE_ABILITY, 17, 42, 43, Bonus::BASE_NUMBER);

	JsonNode & toAdd = data["bonuses"]["toAdd"];

	toAdd.Vector().push_back(b->toJsonNode());

	subject->updateFrom(data);

	auto selector = [](const Bonus * bonus)
	{
		return (bonus->duration == Bonus::PERMANENT)
			&& (bonus->type == Bonus::BLOCKS_RETALIATION)
			&& (bonus->source == Bonus::CREATURE_ABILITY)
			&& (bonus->val == 17)
			&& (bonus->sid == 42)
			&& (bonus->subtype == 43)
			&& (bonus->valType == Bonus::BASE_NUMBER);
	};

	EXPECT_TRUE(subject->hasBonus(selector));
}

TEST_F(CCreatureTest, JsonRemoveBonus)
{
	JsonNode data(JsonNode::JsonType::DATA_STRUCT);

	std::shared_ptr<Bonus> b1 = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::BLOCKS_RETALIATION, Bonus::CREATURE_ABILITY, 17, 42, 43, Bonus::BASE_NUMBER);
	subject->addNewBonus(b1);

	std::shared_ptr<Bonus> b2 = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::BLOCKS_RETALIATION, Bonus::CREATURE_ABILITY, 18, 42, 43, Bonus::BASE_NUMBER);
	subject->addNewBonus(b2);


	JsonNode & toRemove = data["bonuses"]["toRemove"];

	toRemove.Vector().push_back(b2->toJsonNode());

	subject->updateFrom(data);

	auto selector1 = [](const Bonus * bonus)
	{
		return (bonus->duration == Bonus::PERMANENT)
			&& (bonus->type == Bonus::BLOCKS_RETALIATION)
			&& (bonus->source == Bonus::CREATURE_ABILITY)
			&& (bonus->val == 17)
			&& (bonus->sid == 42)
			&& (bonus->subtype == 43)
			&& (bonus->valType == Bonus::BASE_NUMBER);
	};

	EXPECT_TRUE(subject->hasBonus(selector1));

	auto selector2 = [](const Bonus * bonus)
	{
		return (bonus->duration == Bonus::PERMANENT)
			&& (bonus->type == Bonus::BLOCKS_RETALIATION)
			&& (bonus->source == Bonus::CREATURE_ABILITY)
			&& (bonus->val == 18)
			&& (bonus->sid == 42)
			&& (bonus->subtype == 43)
			&& (bonus->valType == Bonus::BASE_NUMBER);
	};

	EXPECT_FALSE(subject->hasBonus(selector2));
}

}
