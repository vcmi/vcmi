#include "StdInc.h"
#include "../lib/json/JsonUtils.h"

TEST(JsonTest, conflictDetectionTestNoConflict)
{
	constexpr char textA[] = R"({ "keyA" : 1, "sameKey" : { "keyInner" : 5 } })";
	constexpr char textB[] = R"({ "keyB" : 1, "sameKey" : { "keyInner" : 10 } })";

	JsonNode jsonA(textA, std::size(textA), "Test A");
	JsonNode jsonB(textB, std::size(textB), "Test B");
	JsonNode result;

	jsonA.setModScope("modA");
	jsonB.setModScope("modB");

	JsonUtils::detectConflicts(result, jsonA, jsonB, "test");

	EXPECT_EQ(result.Struct().size(), 1);
	EXPECT_EQ(result.Struct().count("test/sameKey/keyInner"), 1);
	EXPECT_EQ(result["test/sameKey/keyInner"].Struct().size(), 1);
}

TEST(JsonTest, conflictDetectionTestSimpleConflict)
{
	constexpr char textA[] = R"({ "keyA" : 1, "sameKey" : { "keyInner" : 5 } })";
	constexpr char textB[] = R"({ "keyB" : 1, "sameKey" : { "keyInner" : 10 } })";
	constexpr char textC[] = R"({ "keyC" : 1, "sameKey" : { "keyInner" : 15 } })";

	JsonNode jsonA(textA, std::size(textA), "Test A");
	JsonNode jsonB(textB, std::size(textB), "Test B");
	JsonNode jsonC(textC, std::size(textC), "Test C");
	JsonNode result;

	jsonA.setModScope("modA");
	jsonB.setModScope("modB");
	jsonC.setModScope("modC");

	JsonUtils::detectConflicts(result, jsonA, jsonB, "test");
	JsonUtils::detectConflicts(result, jsonA, jsonC, "test");

	EXPECT_EQ(result.Struct().size(), 1);
	EXPECT_EQ(result.Struct().count("test/sameKey/keyInner"), 1);
	EXPECT_EQ(result["test/sameKey/keyInner"].Struct().size(), 2);
}

TEST(JsonTest, conflictDetectionTestArrayConflict)
{
	constexpr char textA[] = R"({ "keyA" : 1, "sameKey" : { "keyInner" : [ 10 ] } })";
	constexpr char textB[] = R"({ "keyB" : 1, "sameKey" : { "keyInner" : [ 20 ] } })";
	constexpr char textC[] = R"({ "keyC" : 1, "sameKey" : { "keyInner" : [ 30 ] } })";

	JsonNode jsonA(textA, std::size(textA), "Test A");
	JsonNode jsonB(textB, std::size(textB), "Test B");
	JsonNode jsonC(textC, std::size(textC), "Test C");
	JsonNode result;

	jsonA.setModScope("modA");
	jsonB.setModScope("modB");
	jsonC.setModScope("modC");

	JsonUtils::detectConflicts(result, jsonA, jsonB, "test");
	JsonUtils::detectConflicts(result, jsonA, jsonC, "test");

	EXPECT_EQ(result.Struct().size(), 1);
	EXPECT_EQ(result.Struct().count("test/sameKey/keyInner"), 1);
	EXPECT_EQ(result["test/sameKey/keyInner"].Struct().size(), 2);
}

TEST(JsonTest, conflictDetectionTestArrayModifyConflict)
{
	constexpr char textA[] = R"({ "keyA" : 1, "sameKey" : { "keyInner" : [ 10, 20 ] } })";
	constexpr char textB[] = R"({ "keyB" : 1, "sameKey" : { "keyInner" : { "modify@1" : 20 } })";
	constexpr char textC[] = R"({ "keyC" : 1, "sameKey" : { "keyInner" : { "modify@1" : 30 } })";

	JsonNode jsonA(textA, std::size(textA), "Test A");
	JsonNode jsonB(textB, std::size(textB), "Test B");
	JsonNode jsonC(textC, std::size(textC), "Test C");
	JsonNode result;

	jsonA.setModScope("modA");
	jsonB.setModScope("modB");
	jsonC.setModScope("modC");

	JsonUtils::detectConflicts(result, jsonA, jsonB, "test");
	JsonUtils::detectConflicts(result, jsonA, jsonC, "test");

	EXPECT_EQ(result.Struct().size(), 1);
	EXPECT_EQ(result.Struct().count("test/sameKey/keyInner/1"), 1);
	EXPECT_EQ(result["test/sameKey/keyInner/1"].Struct().size(), 2);
}

TEST(JsonTest, conflictDetectionTestArrayModifySafe)
{
	constexpr char textA[] = R"({ "keyA" : 1, "sameKey" : { "keyInner" : [ 10, 20 ] } })";
	constexpr char textB[] = R"({ "keyB" : 1, "sameKey" : { "keyInner" : { "modify@1" : 20 } })";
	constexpr char textC[] = R"({ "keyC" : 1, "sameKey" : { "keyInner" : { "modify@2" : 30 } })";

	JsonNode jsonA(textA, std::size(textA), "Test A");
	JsonNode jsonB(textB, std::size(textB), "Test B");
	JsonNode jsonC(textC, std::size(textC), "Test C");
	JsonNode result;

	jsonA.setModScope("modA");
	jsonB.setModScope("modB");
	jsonC.setModScope("modC");

	JsonUtils::detectConflicts(result, jsonA, jsonB, "test");
	JsonUtils::detectConflicts(result, jsonA, jsonC, "test");

	EXPECT_EQ(result.Struct().size(), 2);
	EXPECT_EQ(result.Struct().count("test/sameKey/keyInner/1"), 1);
	EXPECT_EQ(result.Struct().count("test/sameKey/keyInner/2"), 1);
	EXPECT_EQ(result["test/sameKey/keyInner/1"].Struct().size(), 1);
	EXPECT_EQ(result["test/sameKey/keyInner/2"].Struct().size(), 1);
}

TEST(JsonTest, conflictDetectionTestArrayAppendAlwaysSafe)
{
	constexpr char textA[] = R"({ "keyA" : 1, "sameKey" : { "keyInner" : [ 10, 20 ] } })";
	constexpr char textB[] = R"({ "keyB" : 1, "sameKey" : { "keyInner" : { "append" : 20 } })";
	constexpr char textC[] = R"({ "keyC" : 1, "sameKey" : { "keyInner" : { "append" : 30 } })";

	JsonNode jsonA(textA, std::size(textA), "Test A");
	JsonNode jsonB(textB, std::size(textB), "Test B");
	JsonNode jsonC(textC, std::size(textC), "Test C");
	JsonNode result;

	jsonA.setModScope("modA");
	jsonB.setModScope("modB");
	jsonC.setModScope("modC");

	JsonUtils::detectConflicts(result, jsonA, jsonB, "test");
	JsonUtils::detectConflicts(result, jsonA, jsonC, "test");

	EXPECT_EQ(result.Struct().size(), 0);
}
