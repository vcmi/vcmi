/*
 * JsonComparer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "JsonComparer.h"

#include "../lib/ScopeGuard.h"

//JsonComparer
JsonComparer::JsonComparer(bool strict_)
	: strict(strict_)
{

}

vstd::ScopeGuard<JsonComparer::TScopeGuard> JsonComparer::pushName(const std::string & name)
{
	namePath.push_back(name);
	return vstd::makeScopeGuard<TScopeGuard>([this](){namePath.pop_back();});
}

std::string JsonComparer::buildMessage(const std::string & message)
{
	std::stringstream buf;

	for(auto & s : namePath)
		buf << s << "|";
	buf << " " << message;
	return buf.str();
}

bool JsonComparer::isEmpty(const JsonNode & value)
{
	switch (value.getType())
	{
	case JsonNode::JsonType::DATA_NULL:
		return true;
	case JsonNode::JsonType::DATA_BOOL:
		return !value.Bool();
	case JsonNode::JsonType::DATA_FLOAT:
		return value.Float() == 0;
	case JsonNode::JsonType::DATA_INTEGER:
		return value.Integer() == 0;
	case JsonNode::JsonType::DATA_STRING:
		return value.String() == "";
	case JsonNode::JsonType::DATA_VECTOR:
		return value.Vector().empty();
	case JsonNode::JsonType::DATA_STRUCT:
		return value.Struct().empty();
		break;
	default:
		EXPECT_TRUE(false) << "Unknown Json type";
		return false;
	}
}

void JsonComparer::check(const bool condition, const std::string & message)
{
	if(!condition)
	{
		if(strict)
		{
			GTEST_FAIL() << buildMessage(message);
		}
		else
		{
			ADD_FAILURE() << buildMessage(message);
		}
	}
}

void JsonComparer::checkEqualInteger(const si64 actual, const si64 expected)
{
	if(actual != expected)
	{
		check(false, boost::str(boost::format("actual: '%d' ,expected: '%d'") % actual % expected));
	}
}

void JsonComparer::checkEqualFloat(const double actual, const double expected)
{
	if(std::abs(actual - expected) > 1e-8)
	{
		check(false, boost::str(boost::format("actual: '%lf' ,expected: '%lf' (diff %lf)") % actual % expected % (expected - actual)));
	}
}

void JsonComparer::checkEqualString(const std::string & actual, const std::string & expected)
{
	if(actual != expected)
	{
		check(false, boost::str(boost::format("actual: '%s' , expected: '%s'") % actual % expected));
	}
}

void JsonComparer::checkEqualJson(const JsonMap & actual, const JsonMap & expected)
{
	for(const auto & p : expected)
		checkStructField(actual, p.first, p.second);
	for(const auto & p : actual)
		checkExcessStructField(p.second, p.first, expected);
}

void JsonComparer::checkEqualJson(const JsonVector & actual, const JsonVector & expected)
{
	check(actual.size() == expected.size(), "size mismatch");

	size_t sz = std::min(actual.size(), expected.size());

	for(size_t idx = 0; idx < sz; idx ++)
	{
		auto guard = pushName(boost::to_string(idx));

		checkEqualJson(actual.at(idx), expected.at(idx));
	}
}

void JsonComparer::checkEqualJson(const JsonNode & actual, const JsonNode & expected)
{
	//name has been pushed before

	const bool validType = actual.getType() == expected.getType();

	//do detail checks avoiding assertions in JsonNode

	if(validType)
	{
		switch (actual.getType())
		{
		case JsonNode::JsonType::DATA_NULL:
			break; //do nothing
		case JsonNode::JsonType::DATA_BOOL:
			check(actual.Bool() == expected.Bool(), "mismatch");
			break;
		case JsonNode::JsonType::DATA_FLOAT:
			checkEqualFloat(actual.Float(),expected.Float());
			break;
		case JsonNode::JsonType::DATA_STRING:
			checkEqualString(actual.String(),expected.String());
			break;
		case JsonNode::JsonType::DATA_VECTOR:
			checkEqualJson(actual.Vector(), expected.Vector());
			break;
		case JsonNode::JsonType::DATA_STRUCT:
			checkEqualJson(actual.Struct(), expected.Struct());
			break;
		case JsonNode::JsonType::DATA_INTEGER:
			checkEqualInteger(actual.Integer(), expected.Integer());
			break;
		default:
			check(false, "Unknown Json type");
			break;
		}
	}
	else if(actual.isNumber() && expected.isNumber())
	{
		checkEqualFloat(actual.Float(),expected.Float());
	}
	else
	{
		check(false, "type mismatch. \n expected:\n"+expected.toJson(true)+"\n actual:\n" +actual.toJson(true));
	}
}

void JsonComparer::checkExcessStructField(const JsonNode & actualValue, const std::string & name, const JsonMap & expected)
{
	auto guard = pushName(name);

	if(!vstd::contains(expected, name))
		check(isEmpty(actualValue), "excess");
}

void JsonComparer::checkStructField(const JsonMap & actual, const std::string & name, const JsonNode & expectedValue)
{
	auto guard = pushName(name);

	if(!vstd::contains(actual, name))
		check(isEmpty(expectedValue), "missing");
	else
		checkEqualJson(actual.at(name), expectedValue);
}

void JsonComparer::compare(const std::string & name, const JsonNode & actual, const JsonNode & expected)
{
	auto guard = pushName(name);
	checkEqualJson(actual, expected);
}


