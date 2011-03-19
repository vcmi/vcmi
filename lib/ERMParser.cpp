#include "ERMParser.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <fstream>

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;

ERMParser::ERMParser(std::string file)
	:srcFile(file)
{}

void ERMParser::parseFile()
{
	std::ifstream file(srcFile.c_str());
	if(!file.is_open())
	{
		tlog1 << "File " << srcFile << " not found or unable to open\n";
		return;
	}
	//check header
	char header[5];
	file.getline(header, ARRAY_COUNT(header));
	if(std::string(header) != "ZVSE")
	{
		tlog1 << "File " << srcFile << " has wrong header\n";
		return;
	}
	//parse file
	char lineBuf[1024];
	int lineNum = 1;
	while(file.good())
	{
		//reading line
		file.getline(lineBuf, ARRAY_COUNT(lineBuf));
		if(file.gcount() == ARRAY_COUNT(lineBuf))
		{
			tlog1 << "Encountered a problem during parsing " << srcFile << " too long line " << lineNum << "\n";
		}
		//parsing
		parseLine(lineBuf);


		//loop end
		++lineNum;
	}
}

void callme(char const& i)
{
	std::cout << "fd";
}

void ERMParser::parseLine( std::string line )
{
	namespace ascii = spirit::ascii;
	namespace phoenix = boost::phoenix;
	typedef std::string::iterator sit;
	qi::rule<sit> comment = *(qi::char_);
	qi::rule<sit> commentLine = ~qi::char_('!') >> comment;
	qi::rule<sit> cmdName = qi::repeat(2)[ascii::alpha];
	qi::rule<sit> identifier = qi::int_ % '/';
	qi::rule<sit> condition = qi::char_('&') >> +(qi::int_ | qi::char_("a-zA-Z&/|")) >> qi::char_(":;");
	qi::rule<sit> trigger = cmdName >> -identifier >> -condition;
	qi::rule<sit> string = qi::char_('^') >> ascii::print >> qi::char_('^');
	qi::rule<sit> body = *(qi::char_("a-zA-Z0-9/ ") | string);
	qi::rule<sit> instruction = cmdName >> -identifier >> -condition >> body;
	qi::rule<sit> receiver = cmdName >> -identifier >> -condition >> body;
	qi::rule<sit> postOBtrigger = "$OB" >> -identifier >> -condition;

	qi::rule<sit> rline =
		(
			(qi::char_('!') >>
			(
				(qi::char_('?') >> trigger) |
				(qi::char_('!') >> instruction) |
				(qi::char_('#') >> receiver) |
				postOBtrigger
			) >> comment
			)
			| commentLine | spirit::eoi
		);

	qi::on_error<qi::fail>
		(
		rline
		, std::cout //or phoenix::ref(std::count), is there any difference?
		<< phoenix::val("Error! Expecting ")
		<< qi::_4                               // what failed?
		<< phoenix::val(" here: \"")
		<< phoenix::construct<std::string>(qi::_3, qi::_2)   // iterators to error-pos, end
		<< phoenix::val("\"")
		<< std::endl
		);

	sit beg = line.begin(),
		end = line.end();
	bool r = qi::parse(beg, end, rline);
	if(!r || beg != end)
	{
		tlog1 << "Parse error for line " << line << std::endl;
		tlog1 << "\tCannot parse: " << std::string(beg, end) << std::endl;
	}
}
