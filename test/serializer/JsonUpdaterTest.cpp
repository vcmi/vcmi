/*
 * JsonUpdaterTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/serializer/JsonUpdater.h"

namespace test
{

using ::testing::Test;


class JsonUpdaterTest : public Test
{
public:
	class Object
	{
	public:
		class Inner
		{
		public:
			int64_t i = 424242424242LL;

			Inner() = default;

			void serializeJson(JsonSerializeFormat & handler)
			{
				handler.serializeInt("i", i);
			}
		};

		int32_t i1 = 42;
		int32_t i2 = 59;
		double d1 = 0.123;
		double d2 = 0.321;
		bool b1 = true;
		bool b2 = true;
		int b3 = 333;
		int b4 = 444;

		std::string s1 = "s1s1s1s1";
		std::string s2 = "s2s2s2s2";

		JsonNode j1 = JsonUtils::stringNode("j1");
		JsonNode j2 = JsonUtils::stringNode("j2");

		Inner inner1;
		Inner inner2;

		Object() = default;

		void serializeJson(JsonSerializeFormat & handler)
		{
			handler.serializeInt("i1", i1);
			handler.serializeInt("i2", i2);
			handler.serializeFloat("d1", d1);
			handler.serializeFloat("d2", d2);

			handler.serializeBool("b1", b1);
			handler.serializeBool("b2", b2);
			handler.serializeBool<int>("b3", b3, 333, 332, 331);
			handler.serializeBool<int>("b4", b4, 444, 443, 442);

			handler.serializeString("s1", s1);
			handler.serializeString("s2", s2);

			handler.serializeRaw("j1", j1, std::nullopt);
			handler.serializeRaw("j2", j2, std::nullopt);

			handler.serializeStruct("inner1", inner1);
			handler.serializeStruct("inner2", inner2);
		}

		void compare(const Object & rhs)
		{
			EXPECT_EQ(i1, rhs.i1);
			EXPECT_EQ(i2, rhs.i2);
			EXPECT_EQ(d1, rhs.d1);
			EXPECT_EQ(d2, rhs.d2);

			EXPECT_EQ(b1, rhs.b1);
			EXPECT_EQ(b2, rhs.b2);
			EXPECT_EQ(b3, rhs.b3);
			EXPECT_EQ(b4, rhs.b4);

			EXPECT_EQ(s1, rhs.s1);
			EXPECT_EQ(s2, rhs.s2);

			EXPECT_EQ(j1, rhs.j1);
			EXPECT_EQ(j2, rhs.j2) << j2.toJson(true) << " " << rhs.j2.toJson(true);

			EXPECT_EQ(inner1.i, rhs.inner1.i);
			EXPECT_EQ(inner2.i, rhs.inner2.i);
		}
	};
};

TEST_F(JsonUpdaterTest, Int)
{
	JsonNode j;
	j["i2"].Integer() = 63;

	Object obj;
	Object exp = obj;
	exp.i2 = 63;

	JsonUpdater subject(nullptr, j);
	obj.serializeJson(subject);
	obj.compare(exp);
}

TEST_F(JsonUpdaterTest, Float)
{
	JsonNode j;
	j["d2"].Float() = .456;

	Object obj;
	Object exp = obj;
	exp.d2 = .456;

	JsonUpdater subject(nullptr, j);
	obj.serializeJson(subject);
	obj.compare(exp);
}

TEST_F(JsonUpdaterTest, Bool)
{
	JsonNode j;
	j["b2"].Bool() = false;

	Object obj;
	Object exp = obj;
	exp.b2 = false;

	JsonUpdater subject(nullptr, j);
	obj.serializeJson(subject);
	obj.compare(exp);
}

TEST_F(JsonUpdaterTest, BoolAny)
{
	JsonNode j;
	j["b4"].Bool() = false;

	Object obj;
	Object exp = obj;
	exp.b4 = 443;

	JsonUpdater subject(nullptr, j);
	obj.serializeJson(subject);
	obj.compare(exp);
}

TEST_F(JsonUpdaterTest, String)
{
	JsonNode j;
	j["s2"].String() = "ssssss";

	Object obj;
	Object exp = obj;
	exp.s2 = "ssssss";

	JsonUpdater subject(nullptr, j);
	obj.serializeJson(subject);
	obj.compare(exp);
}

TEST_F(JsonUpdaterTest, Raw)
{
	JsonNode j;
	j["j2"] = JsonUtils::intNode(42);

	Object obj;
	Object exp = obj;
	exp.j2 = JsonUtils::intNode(42);

	JsonUpdater subject(nullptr, j);
	obj.serializeJson(subject);
	obj.compare(exp);
}

TEST_F(JsonUpdaterTest, Struct)
{
	JsonNode j;
	j["inner2"]["i"].Integer() = 424242424241LL;

	Object obj;
	Object exp = obj;
	exp.inner2.i = 424242424241LL;

	JsonUpdater subject(nullptr, j);
	obj.serializeJson(subject);
	obj.compare(exp);
}

}

