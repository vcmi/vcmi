/*
 * JsonParser.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonParser.h"

#include "../ScopeGuard.h"
#include "../texts/TextOperations.h"
#include "JsonFormatException.h"

VCMI_LIB_NAMESPACE_BEGIN

JsonParser::JsonParser(const std::byte * inputString, size_t stringSize, const JsonParsingSettings & settings)
    : JsonParser(reinterpret_cast<const char*>(inputString), stringSize, settings) // NOSONAR
{
}

JsonParser::JsonParser(const char * inputString, size_t stringSize, const JsonParsingSettings & settings)
	: settings(settings)
	, input(inputString, stringSize)
	, lineCount(1)
	, currentDepth(0)
	, lineStart(0)
	, pos(0)
{
}

JsonNode JsonParser::parse(const std::string & fileName)
{
	JsonNode root;

	if(input.empty())
	{
		error("File is empty", false);
	}
	else
	{
		if(!TextOperations::isValidUnicodeString(input.data(), input.size()))
			error("Not a valid UTF-8 file", false);

		// If file starts with BOM - skip it
		uint32_t firstCharacter = TextOperations::getUnicodeCodepoint(input.data(), input.size());
		if (firstCharacter == 0xFEFF)
			pos += TextOperations::getUnicodeCharacterSize(input[0]);

		extractValue(root);
		extractWhitespace(false);

		//Warn if there are any non-whitespace symbols left
		if(pos < input.size())
			error("Not all file was parsed!", true);
	}

	if(!errors.empty())
	{
		logMod->warn("%s is not valid JSON!", fileName);
		logMod->warn(errors);
	}
	return root;
}

bool JsonParser::isValid()
{
	return errors.empty();
}

bool JsonParser::extractSeparator()
{
	if(!extractWhitespace())
		return false;

	if(input[pos] != ':')
		return error("Separator expected");

	pos++;
	return true;
}

bool JsonParser::extractValue(JsonNode & node)
{
	if(!extractWhitespace())
		return false;

	switch(input[pos])
	{
		case '\"':
		case '\'':
			return extractString(node);
		case 'n':
			return extractNull(node);
		case 't':
			return extractTrue(node);
		case 'f':
			return extractFalse(node);
		case '{':
			return extractStruct(node);
		case '[':
			return extractArray(node);
		case '-':
		case '+':
		case '.':
			return extractFloat(node);
		default:
		{
			if(input[pos] >= '0' && input[pos] <= '9')
				return extractFloat(node);
			return error("Value expected!");
		}
	}
}

bool JsonParser::extractWhitespace(bool verbose)
{
	//TODO: JSON5 - C-style multi-line comments
	//TODO: JSON5 - Additional white space characters are allowed

	while(true)
	{
		while(pos < input.size() && static_cast<ui8>(input[pos]) <= ' ')
		{
			if(input[pos] == '\n')
			{
				lineCount++;
				lineStart = pos + 1;
			}
			pos++;
		}

		if(pos >= input.size() || input[pos] != '/')
			break;

		if(settings.mode == JsonParsingSettings::JsonFormatMode::JSON)
			error("Comments are not permitted in json!", true);

		pos++;
		if(pos == input.size())
			break;
		if(input[pos] == '/')
			pos++;
		else
			error("Comments must consist of two slashes!", true);

		while(pos < input.size() && input[pos] != '\n')
			pos++;
	}

	if(pos >= input.size() && verbose)
		return error("Unexpected end of file!");
	return true;
}

bool JsonParser::extractEscaping(std::string & str)
{
	// TODO: support unicode escaping:
	// \u1234

	switch(input[pos])
	{
		case '\r':
			if(settings.mode == JsonParsingSettings::JsonFormatMode::JSON5 && input.size() > pos && input[pos+1] == '\n')
			{
				pos += 2;
				return true;
			}
			break;
		case '\n':
			if(settings.mode == JsonParsingSettings::JsonFormatMode::JSON5)
			{
				pos += 1;
				return true;
			}
			break;
		case '\"':
			str += '\"';
			pos++;
			return true;
		case '\\':
			str += '\\';
			pos++;
			return true;
		case 'b':
			str += '\b';
			pos++;
			return true;
		case 'f':
			str += '\f';
			pos++;
			return true;
		case 'n':
			str += '\n';
			pos++;
			return true;
		case 'r':
			str += '\r';
			pos++;
			return true;
		case 't':
			str += '\t';
			pos++;
			return true;
		case '/':
			str += '/';
			pos++;
			return true;
	}
	return error("Unknown escape sequence!", true);
}

bool JsonParser::extractString(std::string & str)
{
	if(settings.mode < JsonParsingSettings::JsonFormatMode::JSON5)
	{
		if(input[pos] != '\"')
			return error("String expected!");
	}
	else
	{
		if(input[pos] != '\"' && input[pos] != '\'')
			return error("String expected!");
	}

	char lineTerminator = input[pos];
	pos++;

	size_t first = pos;

	while(pos != input.size())
	{
		if(input[pos] == lineTerminator) // Correct end of string
		{
			str.append(&input[first], pos - first);
			pos++;
			return true;
		}
		else if(input[pos] == '\\') // Escaping
		{
			str.append(&input[first], pos - first);
			pos++;
			if(pos == input.size())
				break;

			extractEscaping(str);
			first = pos;
		}
		else if(input[pos] == '\n') // end-of-line
		{
			str.append(&input[first], pos - first);
			return error("Closing quote not found!", true);
		}
		else if(static_cast<unsigned char>(input[pos]) < ' ') // control character
		{
			str.append(&input[first], pos - first);
			pos++;
			first = pos;
			error("Illegal character in the string!", true);
		}
		else
			pos++;
	}
	return error("Unterminated string!");
}

bool JsonParser::extractString(JsonNode & node)
{
	std::string str;
	if(!extractString(str))
		return false;

	node.setType(JsonNode::JsonType::DATA_STRING);
	node.String() = str;
	return true;
}

bool JsonParser::extractLiteral(std::string & literal)
{
	while(pos < input.size())
	{
		bool isUpperCase = input[pos] >= 'A' && input[pos] <= 'Z';
		bool isLowerCase = input[pos] >= 'a' && input[pos] <= 'z';
		bool isNumber = input[pos] >= '0' && input[pos] <= '9';

		if(!isUpperCase && !isLowerCase && !isNumber)
			break;

		literal += input[pos];
		pos++;
	}

	return true;
}

bool JsonParser::extractAndCompareLiteral(const std::string & expectedLiteral)
{
	std::string literal;
	if(!extractLiteral(literal))
		return false;

	if(literal != expectedLiteral)
	{
		return error("Expected " + expectedLiteral + ", but unknown literal found", true);
		return false;
	}

	return true;
}

bool JsonParser::extractNull(JsonNode & node)
{
	if(!extractAndCompareLiteral("null"))
		return false;

	node.clear();
	return true;
}

bool JsonParser::extractTrue(JsonNode & node)
{
	if(!extractAndCompareLiteral("true"))
		return false;

	node.Bool() = true;
	return true;
}

bool JsonParser::extractFalse(JsonNode & node)
{
	if(!extractAndCompareLiteral("false"))
		return false;

	node.Bool() = false;
	return true;
}

bool JsonParser::extractStruct(JsonNode & node)
{
	node.setType(JsonNode::JsonType::DATA_STRUCT);

	if(currentDepth > settings.maxDepth)
		error("Maximum allowed depth of json structure has been reached", true);

	pos++;
	currentDepth++;
	auto guard = vstd::makeScopeGuard([this]()
	{
		currentDepth--;
	});

	if(!extractWhitespace())
		return false;

	//Empty struct found
	if(input[pos] == '}')
	{
		pos++;
		return true;
	}

	while(true)
	{
		if(!extractWhitespace())
			return false;

		bool overrideFlag = false;
		std::string key;

		if(settings.mode < JsonParsingSettings::JsonFormatMode::JSON5)
		{
			if(!extractString(key))
				return false;
		}
		else
		{
			if(input[pos] == '\'' || input[pos] == '\"')
			{
				if(!extractString(key))
					return false;
			}
			else
			{
				if(!extractLiteral(key))
					return false;
			}
		}

		if(key.find('#') != std::string::npos)
		{
			// split key string into actual key and meta-flags
			std::vector<std::string> keyAndFlags;
			boost::split(keyAndFlags, key, boost::is_any_of("#"));

			key = keyAndFlags[0];

			for(int i = 1; i < keyAndFlags.size(); i++)
			{
				if(keyAndFlags[i] == "override")
					overrideFlag = true;
				else
					error("Encountered unknown flag #" + keyAndFlags[i], true);
			}
		}

		if(node.Struct().find(key) != node.Struct().end())
			error("Duplicate element encountered!", true);

		if(!extractSeparator())
			return false;

		if(!extractElement(node.Struct()[key], '}'))
			return false;

		node.Struct()[key].setOverrideFlag(overrideFlag);

		if(input[pos] == '}')
		{
			pos++;
			return true;
		}
	}
}

bool JsonParser::extractArray(JsonNode & node)
{
	if(currentDepth > settings.maxDepth)
		error("Maximum allowed depth of json structure has been reached", true);

	currentDepth++;
	auto guard = vstd::makeScopeGuard([this]()
	{
		currentDepth--;
	});

	pos++;
	node.setType(JsonNode::JsonType::DATA_VECTOR);

	if(!extractWhitespace())
		return false;

	//Empty array found
	if(input[pos] == ']')
	{
		pos++;
		return true;
	}

	while(true)
	{
		//NOTE: currently 50% of time is this vector resizing.
		//May be useful to use list during parsing and then swap() all items to vector
		node.Vector().resize(node.Vector().size() + 1);

		if(!extractElement(node.Vector().back(), ']'))
			return false;

		if(input[pos] == ']')
		{
			pos++;
			return true;
		}
	}
}

bool JsonParser::extractElement(JsonNode & node, char terminator)
{
	if(!extractValue(node))
		return false;

	if(!extractWhitespace())
		return false;

	bool comma = (input[pos] == ',');
	if(comma)
	{
		pos++;
		if(!extractWhitespace())
			return false;
	}

	if(input[pos] == terminator)
	{
		if(comma && settings.mode < JsonParsingSettings::JsonFormatMode::JSON5)
			error("Extra comma found!", true);

		return true;
	}

	if(!comma)
		error("Comma expected!", true);

	return true;
}

bool JsonParser::extractFloat(JsonNode & node)
{
	//TODO: JSON5 - hexacedimal support
	//TODO: JSON5 - Numbers may be IEEE 754 positive infinity, negative infinity, and NaN (why?)

	assert(input[pos] == '-' || (input[pos] >= '0' && input[pos] <= '9'));
	bool negative = false;
	double result = 0;
	si64 integerPart = 0;
	bool isFloat = false;

	if(input[pos] == '+')
	{
		if(settings.mode < JsonParsingSettings::JsonFormatMode::JSON5)
			error("Positive numbers should not have plus sign!", true);
		pos++;
	}
	else if(input[pos] == '-')
	{
		pos++;
		negative = true;
	}

	if(input[pos] < '0' || input[pos] > '9')
	{
		if(input[pos] != '.' && settings.mode < JsonParsingSettings::JsonFormatMode::JSON5)
			return error("Number expected!");
	}

	//Extract integer part
	while(input[pos] >= '0' && input[pos] <= '9')
	{
		integerPart = integerPart * 10 + (input[pos] - '0');
		pos++;
	}

	result = static_cast<double>(integerPart);

	if(input[pos] == '.')
	{
		//extract fractional part
		isFloat = true;
		pos++;
		double fractMult = 0.1;

		if(settings.mode < JsonParsingSettings::JsonFormatMode::JSON5 && (input[pos] < '0' || input[pos] > '9'))
			return error("Decimal part expected!");

		while(input[pos] >= '0' && input[pos] <= '9')
		{
			result = result + fractMult * (input[pos] - '0');
			fractMult /= 10;
			pos++;
		}
	}

	if(input[pos] == 'e')
	{
		//extract exponential part
		pos++;
		isFloat = true;
		bool powerNegative = false;
		double power = 0;

		if(input[pos] == '-')
		{
			pos++;
			powerNegative = true;
		}
		else if(input[pos] == '+')
		{
			pos++;
		}

		if(input[pos] < '0' || input[pos] > '9')
			return error("Exponential part expected!");

		while(input[pos] >= '0' && input[pos] <= '9')
		{
			power = power * 10 + (input[pos] - '0');
			pos++;
		}

		if(powerNegative)
			power = -power;

		result *= std::pow(10, power);
	}

	if(isFloat)
	{
		if(negative)
			result = -result;

		node.setType(JsonNode::JsonType::DATA_FLOAT);
		node.Float() = result;
	}
	else
	{
		if(negative)
			integerPart = -integerPart;

		node.setType(JsonNode::JsonType::DATA_INTEGER);
		node.Integer() = integerPart;
	}

	return true;
}

bool JsonParser::error(const std::string & message, bool warning)
{
	if(settings.strict)
		throw JsonFormatException(message);

	std::ostringstream stream;
	std::string type(warning ? " warning: " : " error: ");

	if(!errors.empty())
	{
		// only add the line breaks between error messages so we don't have a trailing line break
		stream << "\n";
	}
	stream << "At line " << lineCount << ", position " << pos - lineStart << type << message;
	errors += stream.str();

	return warning;
}

VCMI_LIB_NAMESPACE_END
