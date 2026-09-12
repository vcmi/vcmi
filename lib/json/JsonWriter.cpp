/*
 * JsonWriter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonWriter.h"

#include <limits>

template<typename Iterator>
void JsonWriter::writeContainer(Iterator begin, Iterator end)
{
	if(begin == end)
		return;

	prefix += '\t';

	writeEntry(begin++);

	while(begin != end)
	{
		out << (compactMode ? ", " : ",\n");
		writeEntry(begin++);
	}

	out << (compactMode ? "" : "\n");
	prefix.resize(prefix.size() - 1);
}

void JsonWriter::writeEntry(JsonMap::const_iterator entry)
{
	if(!compactMode)
	{
		if(!entry->second.getModScope().empty())
			out << prefix << " // " << entry->second.getModScope() << "\n";
		out << prefix;
	}
	writeString(entry->first);
	out << " : ";
	writeNode(entry->second);
}

void JsonWriter::writeEntry(JsonVector::const_iterator entry)
{
	if(!compactMode)
	{
		if(!entry->getModScope().empty())
			out << prefix << " // " << entry->getModScope() << "\n";
		out << prefix;
	}
	writeNode(*entry);
}

void JsonWriter::writeString(const std::string & string)
{
	static const std::string escaped = "\"\\\b\f\n\r\t";
	static const std::array escapedCode = {'\"', '\\', 'b', 'f', 'n', 'r', 't'};

	out << '\"';
	size_t pos = 0;
	size_t start = 0;
	for(; pos < string.size(); pos++)
	{
		//we need to check if special character was been already escaped
		if((string[pos] == '\\') && (pos + 1 < string.size()) && (std::find(escapedCode.begin(), escapedCode.end(), string[pos + 1]) != escapedCode.end()))
		{
			pos++; //write unchanged, next simbol also checked
		}
		else
		{
			size_t escapedPos = escaped.find(string[pos]);

			if(escapedPos != std::string::npos)
			{
				out.write(string.data() + start, pos - start);
				out << '\\' << escapedCode[escapedPos];
				start = pos + 1;
			}
		}
	}
	out.write(string.data() + start, pos - start);
	out << '\"';
}

namespace
{

/// Repeats the arithmetic that JsonParser::extractFloat performs on unsigned number text. The parser
/// does not hand numbers to strtod: it gathers the digits into an integer mantissa, stops once a
/// double can no longer hold them and scales what it has by a power of ten. Whether a number that
/// has been written comes back unchanged depends on this and on nothing else.
double readNumberBack(const std::string & text)
{
	// largest integer a double still holds exactly, as in JsonParser::extractFloat
	static constexpr si64 maxExactMantissa = 1LL << 53;

	si64 mantissa = 0;
	int fractionDigits = 0;
	size_t pos = 0;

	while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
	{
		mantissa = mantissa * 10 + (text[pos] - '0');
		pos++;
	}

	if(pos < text.size() && text[pos] == '.')
	{
		pos++;

		while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
		{
			// once the mantissa is full the parser drops every digit that follows
			if(mantissa <= (maxExactMantissa - 9) / 10)
			{
				mantissa = mantissa * 10 + (text[pos] - '0');
				fractionDigits++;
			}
			pos++;
		}
	}

	double result = static_cast<double>(mantissa) / std::pow(10, fractionDigits);

	if(pos < text.size() && text[pos] == 'e')
	{
		pos++;

		bool powerNegative = pos < text.size() && text[pos] == '-';

		if(pos < text.size() && (text[pos] == '-' || text[pos] == '+'))
			pos++;

		double power = 0;

		while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
		{
			power = power * 10 + (text[pos] - '0');
			pos++;
		}

		if(powerNegative)
			power = -power;

		result *= std::pow(10, power);
	}

	return result;
}

/// Writes magnitude without an exponent and with the given number of digits after the decimal point
std::string formatFixed(double magnitude, int fractionDigits)
{
	std::ostringstream formatted;
	formatted << std::fixed << std::setprecision(fractionDigits) << magnitude;
	return formatted.str();
}

/// Rounds magnitude to the given number of significant digits and hands back those digits along with
/// the exponent that belongs to them: 1234.5678 to three digits gives "123" and 3
void roundToDigits(double magnitude, int digitCount, std::string & digits, int & exponent)
{
	std::ostringstream formatted;
	formatted << std::scientific << std::setprecision(digitCount - 1) << magnitude;
	std::string text = formatted.str();

	size_t exponentAt = text.find('e');
	exponent = std::stoi(text.substr(exponentAt + 1));

	digits.clear();
	for(size_t i = 0; i < exponentAt; ++i)
	{
		if(text[i] >= '0' && text[i] <= '9')
			digits += text[i];
	}
}

/// Writes the digits with the decimal point after integerDigits of them and the power of ten that
/// puts the number back where it belongs: "123", 3, 1 gives "1.23e2"
std::string formatDigits(const std::string & digits, int exponent, int integerDigits)
{
	int digitCount = static_cast<int>(digits.size());
	std::string text;

	if(integerDigits >= digitCount)
		text = digits + std::string(integerDigits - digitCount, '0');
	else if(integerDigits > 0)
		text = digits.substr(0, integerDigits) + "." + digits.substr(integerDigits);
	else
		text = "0." + std::string(-integerDigits, '0') + digits;

	return text + "e" + std::to_string(exponent + 1 - integerDigits);
}

}

void JsonWriter::writeFloat(double value)
{
	// JsonParser::extractFloat does not hand numbers to strtod: it gathers the digits into an integer
	// mantissa and scales that by a power of ten once, so writing a value at some chosen precision
	// says nothing about the double that reading the file back returns. Reverse the parser instead:
	// take the digits of the value from short to long, run the parser's own arithmetic over every
	// candidate and write the first one that gives this exact double again. Numbers without an
	// exponent are tried first, so ordinary values still read as 0.5 or 1234.5678.
	if(!std::isfinite(value))
	{
		out << value;
		return;
	}

	// digits after the decimal point that std::pow(10, fractionDigits) still scales away exactly
	static constexpr int maxFractionDigits = 22;
	// digits that a double carries
	static constexpr int maxSignificantDigits = 17;
	// above this the integer part no longer fits the si64 that the parser gathers it in
	static constexpr double largestPlainValue = 9e18;

	std::string sign = std::signbit(value) ? "-" : "";
	double magnitude = std::abs(value);
	std::string nearest;
	double nearestDistance = std::numeric_limits<double>::infinity();

	// Writes the candidate if the parser reads it back as this exact value. A value the parser cannot
	// hold at all still has to be written somehow, so the candidates that read as an ordinary number
	// also offer themselves as the fallback for that case.
	auto writeIfExact = [&](const std::string & candidate, bool canBeFallback)
	{
		double readBack = readNumberBack(candidate);

		if(readBack == magnitude)
		{
			out << sign << candidate;
			return true;
		}

		double distance = std::abs(readBack - magnitude);

		if(canBeFallback && distance < nearestDistance)
		{
			nearestDistance = distance;
			nearest = candidate;
		}

		return false;
	};

	if(magnitude < largestPlainValue)
	{
		for(int fractionDigits = 1; fractionDigits <= maxFractionDigits; ++fractionDigits)
		{
			if(writeIfExact(formatFixed(magnitude, fractionDigits), true))
				return;
		}
	}

	std::string digits;
	int exponent = 0;

	for(int digitCount = 1; digitCount <= maxSignificantDigits; ++digitCount)
	{
		roundToDigits(magnitude, digitCount, digits, exponent);

		if(writeIfExact(formatDigits(digits, exponent, 1), true))
			return;
	}

	// The parser rounds a number that carries an exponent twice, once for the digits and once for the
	// power of ten, and there are values that only survive both roundings if the decimal point sits
	// somewhere less usual.
	for(int digitCount = 1; digitCount <= maxSignificantDigits; ++digitCount)
	{
		roundToDigits(magnitude, digitCount, digits, exponent);

		// the integer part has to stay inside the si64 that the parser gathers it in
		int lastPlace = std::min(digitCount + 2, 18);

		for(int integerDigits = -1; integerDigits <= lastPlace; ++integerDigits)
		{
			if(integerDigits == 1)
				continue; // written the usual way above

			if(writeIfExact(formatDigits(digits, exponent, integerDigits), false))
				return;
		}
	}

	// The parser keeps only the digits that a double holds exactly, so there are doubles that no
	// number in a json file can produce. Write the one it comes closest to.
	out << sign << nearest;
}

void JsonWriter::writeNode(const JsonNode & node)
{
	bool originalMode = compactMode;
	if(compact && !compactMode && node.isCompact())
		compactMode = true;

	switch(node.getType())
	{
		case JsonNode::JsonType::DATA_NULL:
			out << "null";
			break;

		case JsonNode::JsonType::DATA_BOOL:
			if(node.Bool())
				out << "true";
			else
				out << "false";
			break;

		case JsonNode::JsonType::DATA_FLOAT:
			writeFloat(node.Float());
			break;

		case JsonNode::JsonType::DATA_STRING:
			writeString(node.String());
			break;

		case JsonNode::JsonType::DATA_VECTOR:
			out << "[" << (compactMode ? " " : "\n");
			writeContainer(node.Vector().begin(), node.Vector().end());
			out << (compactMode ? " " : prefix) << "]";
			break;

		case JsonNode::JsonType::DATA_STRUCT:
			out << "{" << (compactMode ? " " : "\n");
			writeContainer(node.Struct().begin(), node.Struct().end());
			out << (compactMode ? " " : prefix) << "}";
			break;

		case JsonNode::JsonType::DATA_INTEGER:
			out << node.Integer();
			break;
	}

	compactMode = originalMode;
}

JsonWriter::JsonWriter(std::ostream & output, bool compact)
	: out(output)
	, compact(compact)
{
}
