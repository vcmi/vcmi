#define VCMI_DLL
#include "ERMParser.h"
#include <boost/version.hpp>
//To make compilation with older boost versions possible
//Don't know exact version - 1.46 works while 1.42 not
#if BOOST_VERSION >= 104600

#include <boost/spirit/include/qi.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = spirit::ascii;
namespace phoenix = boost::phoenix;

/*
 * ERMParser.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


//Greenspun's Tenth Rule of Programming:
//Any sufficiently complicated C or Fortran program contains an ad hoc, informally-specified,
//bug-ridden, slow implementation of half of Common Lisp.
//actually these macros help in dealing with boost::variant


#define BEGIN_TYPE_CASE(LinePrinterVisitor) struct LinePrinterVisitor : boost::static_visitor<> \
{

#define FOR_TYPE(TYPE, VAR) void operator()(TYPE const& VAR) const

#define DO_TYPE_CASE(LinePrinterVisitor, VAR) } ___UN; boost::apply_visitor(___UN, VAR);

CERMPreprocessor::CERMPreprocessor(const std::string &Fname) : fname(Fname), file(Fname.c_str()), lineNo(0), version(INVALID)
{
	if(!file.is_open())
	{
		tlog1 << "File " << Fname << " not found or unable to open\n";
		return;
	}

	//check header
	std::string header;
	getline(header);

	if(header == "ZVSE")
		version = ERM;
	else if(header == "VERM")
		version = VERM;
	else
	{
		tlog1 << "File " << fname << " has wrong header\n";
		return;
	}
}

class ParseErrorException : public std::exception
{

};

std::string CERMPreprocessor::retreiveCommandLine()
{
	std::string wholeCommand;

	//parse file
	bool verm = false;
	bool openedString = false;
	int openedBraces = 0;


	while(file.good())
	{

		std::string line ;
		getline(line); //reading line


		int dash = line.find_first_of('^');
		bool inTheMiddle = openedBraces || openedString;

		if(!inTheMiddle)
		{
			if(line.size() < 2)
				continue;
			if(line[0] != '!' ) //command lines must begin with ! -> otherwise treat as comment
				continue;
			verm = line[1] == '[';
		}

		if(openedString)
		{
			wholeCommand += "\\n";
			if(dash != std::string::npos)
			{
				wholeCommand += line.substr(0, dash);
				line.erase(0,dash);
			}
			else //no closing marker -> the whole line is further part of string
			{
				wholeCommand += line;
				continue;
			}
		}

		int i = 0;
		for(; i < line.length(); i++)
		{
			char c = line[i];
			if(!openedString)
			{
				if(c == '[')
					openedBraces++;
				else if(c == ']')
				{
					openedBraces--;
					if(!openedBraces) //the last brace has been matched -> stop "parsing", everything else in the line is comment
					{
						i++;
						break;
					}
				}
				else if(c == '^')
					openedString = true;
				else if(c == ';') // a ';' that is in command line (and not in string) ends the command -> throw away rest
				{
					line.erase(i+!verm, line.length() - i - !verm); //leave ';' at the end only at ERM commands
					break;
				}
			}
			else if(c == '^')
				openedString = false;
		}

		if(verm && !openedBraces && i < line.length())
		{
			line.erase(i, line.length() - i);
		}

		if(wholeCommand.size()) //separate lines with a space
			wholeCommand += " ";

		wholeCommand += line;
		if(!openedBraces && !openedString)
			return wholeCommand;

		//loop end
	}

	if(openedBraces || openedString)
		tlog1 << "Ill-formed file: " << fname << std::endl;
	return "";
}

void CERMPreprocessor::getline(std::string &ret)
{
	lineNo++;
	std::getline(file, ret);
	boost::trim(ret); //get rid of wspace
}

ERMParser::ERMParser(std::string file)
	:srcFile(file)
{}

std::vector<ERM::TLine> ERMParser::parseFile()
{
	CERMPreprocessor preproc(srcFile);
	std::vector<ERM::TLine> ret;
	try
	{
		while(1)
		{
			std::string command = preproc.retreiveCommandLine();
			if(command.length() == 0)
				break;

			repairEncoding(command);
			ret.push_back(parseLine(command));
		}
	}
	catch (ParseErrorException & e)
	{
		tlog1 << "stopped parsing file" << std::endl;
	}
	return ret;
}

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TStringConstant,
	(std::string, str)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TMacroUsage,
	(std::string, macro)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TQMacroUsage,
	(std::string, qmacro)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TMacroDef,
	(std::string, macro)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVarExpNotMacro,
	(boost::optional<char>, questionMark)
	(std::string, varsym)
	(ERM::TVarExpNotMacro::Tval, val)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TArithmeticOp,
	(ERM::TIexp, lhs)
	(char, opcode)
	(ERM::TIexp, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVarpExp,
	(ERM::TVarpExp::Tvartype, var)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVRLogic,
	(char, opcode)
	(ERM::TIexp, var)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVRArithmetic,
	(char, opcode)
	(ERM::TIexp, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TNormalBodyOption,
	(char, optionCode)
	(ERM::TNormalBodyOptionList, params)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::Ttrigger,
	(ERM::TCmdName, name)
	(boost::optional<ERM::Tidentifier>, identifier)
	(boost::optional<ERM::Tcondition>, condition)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TComparison,
	(ERM::TIexp, lhs)
	(std::string, compSign)
	(ERM::TIexp, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TSemiCompare,
	(std::string, compSign)
	(ERM::TIexp, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TCurriedString,
	(ERM::TIexp, iexp)
	(ERM::TStringConstant, string)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVarConcatString,
	(ERM::TVarExp, var)
	(ERM::TStringConstant, string)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::Tcondition,
	(char, ctype)
	(ERM::Tcondition::Tcond, cond)
	(ERM::TconditionNode, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::Tinstruction,
	(ERM::TCmdName, name)
	(boost::optional<ERM::Tidentifier>, identifier)
	(boost::optional<ERM::Tcondition>, condition)
	(ERM::Tbody, body)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::Treceiver,
	(ERM::TCmdName, name)
	(boost::optional<ERM::Tidentifier>, identifier)
	(boost::optional<ERM::Tcondition>, condition)
	(boost::optional<ERM::Tbody>, body)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TPostTrigger,
	(ERM::TCmdName, name)
	(boost::optional<ERM::Tidentifier>, identifier)
	(boost::optional<ERM::Tcondition>, condition)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::Tcommand,
	(ERM::Tcommand::Tcmd, cmd)
	(std::string, comment)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVExp,
	(std::vector<ERM::TVModifier>, modifier)
	(std::vector<ERM::TVOption>, children)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TSymbol,
	(std::vector<ERM::TVModifier>, symModifier)
	(std::string, sym)
	)

namespace ERM
{
	template<typename Iterator>
	struct ERM_grammar : qi::grammar<Iterator, TLine(), ascii::space_type>
	{
		ERM_grammar() : ERM_grammar::base_type(vline, "VERM script line")
		{
			//do not build too complicated expressions, e.g. (a >> b) | c, qi has problems with them
			ERMmacroUsage %= qi::lexeme[qi::lit('$') >> *(qi::char_ - '$') >> qi::lit('$')];
			ERMmacroDef %= qi::lexeme[qi::lit('@') >> *(qi::char_ - '@') >> qi::lit('@')];
			varExpNotMacro %= -qi::char_("?") >> (+(qi::char_("a-z") - 'u')) >> -qi::int_;
			qERMMacroUsage %= qi::lexeme[qi::lit("?$") >> *(qi::char_ - '$') >> qi::lit('$')];
			varExp %= varExpNotMacro | ERMmacroUsage;
			iexp %= varExp | qi::int_;
			varp %=/* qi::lit("?") >> */(varExpNotMacro | qERMMacroUsage);
 			comment %= *qi::char_;
			commentLine %= (~qi::char_("!") >> comment | (qi::char_('!') >> (~qi::char_("?!$#[")) >> comment ));
 			cmdName %= qi::lexeme[qi::repeat(2)[qi::char_]];
			arithmeticOp %= iexp >> qi::char_ >> iexp;
			//identifier is usually a vector of i-expressions but VR receiver performs arithmetic operations on it
			identifier %= (iexp | arithmeticOp) % qi::lit('/');
			comparison %= iexp >> (*qi::char_("<=>")) >> iexp;
			condition %= qi::char_("&|X/") >> (comparison | qi::int_) >> -condition;

			trigger %= cmdName >> -identifier >> -condition > qi::lit(";"); /////
			string %= qi::lexeme['^' >> *(qi::char_ - '^') >> '^'];
			
			VRLogic %= qi::char_("&|X") >> iexp;
			VRarithmetic %= qi::char_("+*:/%-") >> iexp;
			semiCompare %= *qi::char_("<=>") >> iexp;
			curStr %= iexp >> string;
			varConcatString %= varExp >> qi::lit("+") >> string;
			bodyOptionItem %= varConcatString | curStr | string | semiCompare | ERMmacroUsage | ERMmacroDef | varp | iexp | qi::eps;
			exactBodyOptionList %= (bodyOptionItem % qi::lit("/"));
			normalBodyOption = qi::char_("A-Z+") > exactBodyOptionList;
			bodyOption %= VRLogic | VRarithmetic | normalBodyOption;
			body %= qi::lit(":") >> +(bodyOption) > qi::lit(";");

			instruction %= cmdName >> -identifier >> -condition >> body;
			receiver %= cmdName >> -identifier >> -condition >> -body; //receiver without body exists... change needed
			postTrigger %= cmdName >> -identifier >> -condition > qi::lit(";");

			command %= (qi::lit("!") >>
					(
						(qi::lit("?") >> trigger) |
						(qi::lit("!") >> receiver) |
						(qi::lit("#") >> instruction) | 
						(qi::lit("$") >> postTrigger)
					) >> comment
				);

			rline %=
				(
					command | commentLine | spirit::eps
				);

			vmod %= qi::string("`") | qi::string(",!") | qi::string(",") | qi::string("#'") | qi::string("'");
			vsym %= *vmod >> qi::lexeme[+qi::char_("+*/$%&_=<>~a-zA-Z0-9-")];

			qi::real_parser<double, qi::strict_real_policies<double> > strict_double;
			vopt %= qi::lexeme[(qi::lit("!") >> qi::char_ >> qi::lit("!"))] | qi::lexeme[strict_double] | qi::lexeme[qi::int_] | command | vexp | string | vsym;
			vexp %= *vmod >> qi::lit("[") >> *(vopt) >> qi::lit("]");

			vline %= (( qi::lit("!") >>vexp) | rline ) > spirit::eoi;

			//error handling

			string.name("string constant");
			ERMmacroUsage.name("macro usage");
			qERMMacroUsage.name("macro usage with ?");
			ERMmacroDef.name("macro definition");
			varExpNotMacro.name("variable expression (not macro)");
			varExp.name("variable expression");
			iexp.name("i-expression");
			comment.name("comment");
			commentLine.name("comment line");
			cmdName.name("name of a command");
			identifier.name("identifier");
			condition.name("condition");
			trigger.name("trigger");
			body.name("body");
			instruction.name("instruction");
			receiver.name("receiver");
			postTrigger.name("post trigger");
			command.name("command");
			rline.name("ERM script line");
			vsym.name("V symbol");
			vopt.name("V option");
			vexp.name("V expression");
			vline.name("VERM line");

			qi::on_error<qi::fail>
				(
				vline
				, std::cout //or phoenix::ref(std::count), is there any difference?
				<< phoenix::val("Error! Expecting ")
				<< qi::_4                               // what failed?
				<< phoenix::val(" here: \"")
				<< phoenix::construct<std::string>(qi::_3, qi::_2)   // iterators to error-pos, end
				<< phoenix::val("\"")
				<< std::endl
				);

		}

		qi::rule<Iterator, TStringConstant(), ascii::space_type> string;

		qi::rule<Iterator, TMacroUsage(), ascii::space_type> ERMmacroUsage;
		qi::rule<Iterator, TQMacroUsage(), ascii::space_type> qERMMacroUsage;
		qi::rule<Iterator, TMacroDef(), ascii::space_type> ERMmacroDef;
		qi::rule<Iterator, TVarExpNotMacro(), ascii::space_type> varExpNotMacro;
		qi::rule<Iterator, TVarExp(), ascii::space_type> varExp;
		qi::rule<Iterator, TIexp(), ascii::space_type> iexp;
		qi::rule<Iterator, TVarpExp(), ascii::space_type> varp;
		qi::rule<Iterator, TArithmeticOp(), ascii::space_type> arithmeticOp;
		qi::rule<Iterator, std::string(), ascii::space_type> comment;
		qi::rule<Iterator, std::string(), ascii::space_type> commentLine;
		qi::rule<Iterator, TCmdName(), ascii::space_type> cmdName;
		qi::rule<Iterator, Tidentifier(), ascii::space_type> identifier;
		qi::rule<Iterator, TComparison(), ascii::space_type> comparison;
		qi::rule<Iterator, Tcondition(), ascii::space_type> condition;
		qi::rule<Iterator, TVRLogic(), ascii::space_type> VRLogic;
		qi::rule<Iterator, TVRArithmetic(), ascii::space_type> VRarithmetic;
		qi::rule<Iterator, TSemiCompare(), ascii::space_type> semiCompare;
		qi::rule<Iterator, TCurriedString(), ascii::space_type> curStr;
		qi::rule<Iterator, TVarConcatString(), ascii::space_type> varConcatString;
		qi::rule<Iterator, TBodyOptionItem(), ascii::space_type> bodyOptionItem;
		qi::rule<Iterator, TNormalBodyOptionList(), ascii::space_type> exactBodyOptionList;
		qi::rule<Iterator, TNormalBodyOption(), ascii::space_type> normalBodyOption;
		qi::rule<Iterator, TBodyOption(), ascii::space_type> bodyOption;
		qi::rule<Iterator, Ttrigger(), ascii::space_type> trigger;
		qi::rule<Iterator, Tbody(), ascii::space_type> body;
		qi::rule<Iterator, Tinstruction(), ascii::space_type> instruction;
		qi::rule<Iterator, Treceiver(), ascii::space_type> receiver;
		qi::rule<Iterator, TPostTrigger(), ascii::space_type> postTrigger;
		qi::rule<Iterator, Tcommand(), ascii::space_type> command;
		qi::rule<Iterator, TERMline(), ascii::space_type> rline;
		qi::rule<Iterator, TSymbol(), ascii::space_type> vsym;
		qi::rule<Iterator, TVModifier(), ascii::space_type> vmod;
		qi::rule<Iterator, TVOption(), ascii::space_type> vopt;
		qi::rule<Iterator, TVExp(), ascii::space_type> vexp;
		qi::rule<Iterator, TLine(), ascii::space_type> vline;
	};
};

ERM::TLine ERMParser::parseLine( const std::string & line )
{
	std::string::const_iterator beg = line.begin(),
		end = line.end();

	ERM::ERM_grammar<std::string::const_iterator> ERMgrammar;
	ERM::TLine AST;

	bool r = qi::phrase_parse(beg, end, ERMgrammar, ascii::space, AST);
	if(!r || beg != end)
	{
		tlog1 << "Parse error in file " << srcFile << " (line " << parsedLine << ") :\n" << line << std::endl;
		tlog1 << "\tCannot parse: " << std::string(beg, end) << std::endl;
		throw ParseErrorException();
	}
	return AST;
}

ERMParser::ELineType ERMParser::classifyLine( const std::string & line, bool inString ) const
{
	ERMParser::ELineType ret;
	if(line[0] == '!')
	{
		if(countHatsBeforeSemicolon(line) % 2 == 1)
			ret = ERMParser::UNFINISHED;
		else
			ret = ERMParser::COMMAND_FULL;
	}
	else
	{
		if(inString)
		{
			if(countHatsBeforeSemicolon(line) % 2 == 1)
				ret = ERMParser::END_OF;
			else
				ret = ERMParser::UNFINISHED;
		}
		else
		{
			ret = ERMParser::COMMENT;
		}
	}

	return ret;
}

int ERMParser::countHatsBeforeSemicolon( const std::string & line ) const
{
	//CHECK: omit macros? or anything else? 
	int numOfHats = 0; //num of '^' before ';'
	//check for unmatched ^
	BOOST_FOREACH(char c, line)
	{
		if(c == ';')
			break;
		if(c == '^')
			++numOfHats;
	}
	return numOfHats;
}

void ERMParser::repairEncoding( std::string & str ) const
{
	for(int g=0; g<str.size(); ++g)
		if(str[g] & 0x80)
			str[g] = '|';
}

void ERMParser::repairEncoding( char * str, int len ) const
{
	for(int g=0; g<len; ++g)
		if(str[g] & 0x80)
			str[g] = '|';
}
#else

ERMParser::ERMParser(std::string file){}
std::vector<ERM::TLine> ERMParser::parseFile() {}

#endif
