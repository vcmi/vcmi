#include "ERMParser.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <fstream>

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = spirit::ascii;
namespace phoenix = boost::phoenix;


//Greenspun's Tenth Rule of Programming:
//Any sufficiently complicated C or Fortran program contains an ad hoc, informally-specified,
//bug-ridden, slow implementation of half of Common Lisp.
//actually these macros help in dealing with boost::variant


#define BEGIN_TYPE_CASE(UN) struct UN : boost::static_visitor<> \
{

#define FOR_TYPE(TYPE, VAR) void operator()(TYPE const& VAR) const

#define DO_TYPE_CASE(UN, VAR) } ___UN; boost::apply_visitor(___UN, VAR);



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
	parsedLine = 1;
	std::string wholeLine; //used for buffering multiline lines
	bool inString = false;
	
	while(file.good())
	{
		//reading line
		file.getline(lineBuf, ARRAY_COUNT(lineBuf));
		if(file.gcount() == ARRAY_COUNT(lineBuf))
		{
			tlog1 << "Encountered a problem during parsing " << srcFile << " too long line (" << parsedLine << ")\n";
		}

		switch(classifyLine(lineBuf, inString))
		{
		case ERMParser::COMMAND_FULL:
		case ERMParser::COMMENT:
			{
				parseLine(lineBuf);
			}
			break;
		case ERMParser::UNFINISHED_STRING:
			{
				if(!inString)
					wholeLine = "";
				inString = true;
				wholeLine += lineBuf;
			}
			break;
		case ERMParser::END_OF_STRING:
			{
				inString = false;
				wholeLine += lineBuf;
				parseLine(wholeLine);
			}
			break;
		}

		//loop end
		++parsedLine;
	}
}

void callme(char const& i)
{
	std::cout << "fd";
}



namespace ERM
{
	typedef std::string TStringConstant;
	typedef std::string TMacroUsage;
	typedef std::string TMacroDef;
	typedef std::string TCmdName;

	
	struct TVarExpNotMacro
	{
		typedef boost::optional<int> valT;
		std::string varsym;
		valT val;
	};
	typedef boost::variant<TVarExpNotMacro, TMacroUsage> TVarExp;

	//write-only variable expression
	typedef TVarExp TVarpExp;

	//i-expression (identifier expression) - an integral constant, variable symbol or array symbol
	typedef boost::variant<TVarExp, int> iexpT;

	struct TArithmeticOp
	{
		iexpT lhs, rhs;
		char opcode;
	};

	struct TVRLogic
	{
		char opcode;
		iexpT var;
	};

	struct TVRArithmetic
	{
		char opcode;
		iexpT rhs;
	};

	struct TSemiCompare
	{
		std::string compSign;
		iexpT rhs;
	};

	struct TCurriedString
	{
		iexpT iexp;
		TStringConstant string;
	};

	struct TVarConcatString 
	{
		TVarExp var;
		TStringConstant string;
	};

	typedef boost::variant<TVarConcatString, TStringConstant, TCurriedString, TSemiCompare, TMacroUsage, TMacroDef, iexpT, TVarpExp, qi::unused_type> TBodyOptionItem;

	typedef std::vector<TBodyOptionItem> TNormalBodyOptionList;

	struct TNormalBodyOption
	{
		char optionCode;
		TNormalBodyOptionList params;
	};
	typedef boost::variant<TVRLogic, TVRArithmetic, TNormalBodyOption> TBodyOption;

	typedef boost::variant<iexpT, TArithmeticOp > TIdentifierInternal;
	typedef std::vector< TIdentifierInternal > identifierT;

	struct TComparison
	{
		std::string compSign;
		iexpT lhs, rhs;
	};

	struct conditionT;
	typedef
		boost::optional<
		boost::recursive_wrapper<conditionT>
		>
		conditionNodeT;


	struct conditionT
	{
		typedef boost::variant<
			TComparison,
			int>
			Tcond; //comparison or condition flag
		char ctype;
		Tcond cond;
		conditionNodeT rhs;
	};

	struct triggerT
	{
		TCmdName name;
		boost::optional<identifierT> identifier;
		boost::optional<conditionT> condition;
	};

	//a dirty workaround for preprocessor magic that prevents the use types with comma in it in BOOST_FUSION_ADAPT_STRUCT
	//see http://comments.gmane.org/gmane.comp.lib.boost.user/62501 for some info
	//
	//moreover, I encountered a quite serious bug in boost: http://boost.2283326.n4.nabble.com/container-hpp-111-error-C2039-value-type-is-not-a-member-of-td3352328.html
	//not sure how serious it is...

	//typedef boost::variant<char, TStringConstant, TMacroUsage, TMacroDef> bodyItem;
 	typedef std::vector<TBodyOption> bodyTbody;

	struct instructionT
	{
		TCmdName name;
		boost::optional<identifierT> identifier;
		boost::optional<conditionT> condition;
		bodyTbody body;
	};

	struct receiverT
	{
		TCmdName name;
		boost::optional<identifierT> identifier;
		boost::optional<conditionT> condition;
		bodyTbody body;
	};

	struct postOBtriggerT
	{
		boost::optional<identifierT> identifier;
		boost::optional<conditionT> condition;
	};

	typedef	boost::variant<
		triggerT,
		instructionT,
		receiverT,
		postOBtriggerT
	>
	commandTcmd;

	struct commandT
	{
		commandTcmd cmd;
		std::string comment;
	};

	typedef boost::variant<commandT, std::string, qi::unused_type> lineT;


	//console printer

	struct VarPrinter : boost::static_visitor<>
	{
		void operator()(TVarExpNotMacro const& val) const
		{
			tlog2 << val.varsym;
			if(val.val.is_initialized())
			{
				tlog2 << val.val.get();
			}
		}
		void operator()(TMacroUsage const& val) const
		{
			tlog2 << "$" << val << "&";
		}
	};

	void varPrinter(const TVarExp & var)
	{
		boost::apply_visitor(VarPrinter(), var);
	}

	struct _IEP : boost::static_visitor<>
	{
		void operator()(int const & constant) const
		{
			tlog2 << constant;
		}
		void operator()(TVarExp const & var) const
		{
			varPrinter(var);
		}
	};


	void iexpPrinter(const iexpT & exp)
	{
		boost::apply_visitor(_IEP(), exp);
	}

	struct IdentifierVisitor : boost::static_visitor<>
	{
		void operator()(iexpT const& iexp) const
		{
			iexpPrinter(iexp);
		}
		void operator()(TArithmeticOp const& arop) const
		{
			iexpPrinter(arop.lhs);
			tlog2 << " " << arop.opcode << " ";
			iexpPrinter(arop.rhs);
		}
	};

	void identifierPrinter(const boost::optional<identifierT> & id)
	{
		if(id.is_initialized())
		{
			tlog2 << "identifier: ";
			BOOST_FOREACH(TIdentifierInternal x, id.get())
			{
				tlog2 << "\\";
				boost::apply_visitor(IdentifierVisitor(), x);
			}
		}
	}

	struct ConditionCondPrinter : boost::static_visitor<>
	{
		void operator()(TComparison const& cmp) const
		{
			iexpPrinter(cmp.lhs);
			tlog2 << " " << cmp.compSign << " ";
			iexpPrinter(cmp.rhs);
		}
		void operator()(int const& flag) const
		{
			tlog2 << "condflag " << flag;
		}
	};

	void conditionPrinter(const boost::optional<conditionT> & cond)
	{
		if(cond.is_initialized())
		{
			conditionT condp = cond.get();
			tlog2 << " condition: ";
			boost::apply_visitor(ConditionCondPrinter(), condp.cond);
			tlog2 << " cond type: " << condp.ctype << " rhs:";
			
			//recursive call
			if(condp.rhs.is_initialized())
			{
				boost::optional<conditionT> rhsc = condp.rhs.get().get();
				conditionPrinter(rhsc);
			}
		}
	}

	struct UN2 : boost::static_visitor<>
	{
		void operator()(triggerT const& trig) const
		{
			tlog2 << "trigger: " << trig.name;
			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);
		}
		void operator()(instructionT const& trig) const
		{
			tlog2 << "instruction: " << trig.name;
			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);

			tlog2 << " body items: ";
// 			BOOST_FOREACH(bodyItem bi, trig.body)
// 			{
// 				tlog2 << " " << bi;
// 			}
		}
		void operator()(receiverT const& trig) const
		{
			tlog2 << "receiver: " << trig.name;

			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);
		}
		void operator()(postOBtriggerT const& trig) const
		{
			tlog2 << "post OB trigger; ";
			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);
		}
	};

	struct UN : boost::static_visitor<>
	{
		void operator()(commandT const& cmd) const
		{
			UN2 un;
			boost::apply_visitor(un, cmd.cmd);
			std::cout << "Line comment: " << cmd.comment << std::endl;
		}
		void operator()(std::string const& comment) const
		{
		}
		void operator()(qi::unused_type const& nothing) const
		{
		}
	};

	void printLineAST(const lineT & ast)
	{
		tlog2 << "";

		UN zm = UN();
		
		boost::apply_visitor(zm, ast);
	}
}

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVarExpNotMacro,
	(std::string, varsym)
	(ERM::TVarExpNotMacro::valT, val)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TArithmeticOp,
	(ERM::iexpT, lhs)
	(char, opcode)
	(ERM::iexpT, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVRLogic,
	(char, opcode)
	(ERM::iexpT, var)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVRArithmetic,
	(char, opcode)
	(ERM::iexpT, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TNormalBodyOption,
	(char, optionCode)
	(ERM::TNormalBodyOptionList, params)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::triggerT,
	(ERM::TCmdName, name)
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<ERM::conditionT>, condition)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TComparison,
	(ERM::iexpT, lhs)
	(std::string, compSign)
	(ERM::iexpT, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TSemiCompare,
	(std::string, compSign)
	(ERM::iexpT, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TCurriedString,
	(ERM::iexpT, iexp)
	(ERM::TStringConstant, string)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TVarConcatString,
	(ERM::TVarExp, var)
	(ERM::TStringConstant, string)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::conditionT,
	(char, ctype)
	(ERM::conditionT::Tcond, cond)
	(ERM::conditionNodeT, rhs)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::instructionT,
	(ERM::TCmdName, name)
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<ERM::conditionT>, condition)
	(ERM::bodyTbody, body)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::receiverT,
	(ERM::TCmdName, name)
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<ERM::conditionT>, condition)
	(ERM::bodyTbody, body)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::postOBtriggerT,
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<ERM::conditionT>, condition)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::commandT,
	(ERM::commandTcmd, cmd)
	(std::string, comment)
	)

namespace ERM
{
	template<typename Iterator>
	struct ERM_grammar : qi::grammar<Iterator, lineT(), ascii::space_type>
	{
		ERM_grammar() : ERM_grammar::base_type(rline, "ERM script line")
		{
			//do not build too complicated expressions, e.g. (a >> b) | c, qi has problems with them
			macroUsage %= qi::lexeme[qi::lit('$') >> *(qi::char_ - '$') >> qi::lit('$')];
			macroDef %= qi::lexeme[qi::lit('@') >> *(qi::char_ - '@') >> qi::lit('@')];
			varExpNotMacro %= (+(qi::char_("?a-z") - 'u')) >> -qi::int_;
			varExp %= varExpNotMacro | macroUsage;
			iexp %= varExp | qi::int_;
			varp %= qi::char_("?") > varExp;
 			comment %= *(qi::char_);
			commentLine %= (~qi::char_('!') >> comment | (qi::char_('!') >> (~qi::char_("?!#")) >> comment ));
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
			bodyOptionItem %= varConcatString | curStr | string | semiCompare | macroUsage | macroDef | iexp | varp | qi::eps;
			exactBodyOptionList %= (bodyOptionItem % qi::lit("/"));
			normalBodyOption = qi::char_("A-Z") > exactBodyOptionList;
			bodyOption %= VRLogic | VRarithmetic | normalBodyOption;
			body %= qi::lit(":") >> +(bodyOption) > qi::lit(";");

			instruction %= cmdName >> -identifier >> -condition >> body;
			receiver %= cmdName >> -identifier >> -condition >> body; //receiver without body exists... change needed
			postOBtrigger %= qi::lit("$OB") >> -identifier >> -condition > qi::lit(";");
			command %= (qi::lit("!") >>
					(
						(qi::lit("?") >> trigger) |
						(qi::lit("!") >> receiver) |
						(qi::lit("#") >> instruction) | 
						postOBtrigger
					) >> comment
				);

			rline %=
				(
					command | commentLine | spirit::eps
				) > spirit::eoi;

			//error handling

			string.name("string constant");
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
			postOBtrigger.name("post OB trigger");
			command.name("command");
			rline.name("script line");

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

		}

		qi::rule<Iterator, TStringConstant(), ascii::space_type> string;

		qi::rule<Iterator, TMacroUsage(), ascii::space_type> macroUsage;
		qi::rule<Iterator, TMacroDef(), ascii::space_type> macroDef;
		qi::rule<Iterator, TVarExpNotMacro(), ascii::space_type> varExpNotMacro;
		qi::rule<Iterator, TVarExp(), ascii::space_type> varExp;
		qi::rule<Iterator, iexpT(), ascii::space_type> iexp;
		qi::rule<Iterator, TVarpExp(), ascii::space_type> varp;
		qi::rule<Iterator, TArithmeticOp(), ascii::space_type> arithmeticOp;
		qi::rule<Iterator, std::string(), ascii::space_type> comment;
		qi::rule<Iterator, std::string(), ascii::space_type> commentLine;
		qi::rule<Iterator, TCmdName(), ascii::space_type> cmdName;
		qi::rule<Iterator, identifierT(), ascii::space_type> identifier;
		qi::rule<Iterator, TComparison(), ascii::space_type> comparison;
		qi::rule<Iterator, conditionT(), ascii::space_type> condition;
		qi::rule<Iterator, TVRLogic(), ascii::space_type> VRLogic;
		qi::rule<Iterator, TVRArithmetic(), ascii::space_type> VRarithmetic;
		qi::rule<Iterator, TSemiCompare(), ascii::space_type> semiCompare;
		qi::rule<Iterator, TCurriedString(), ascii::space_type> curStr;
		qi::rule<Iterator, TVarConcatString(), ascii::space_type> varConcatString;
		qi::rule<Iterator, TBodyOptionItem(), ascii::space_type> bodyOptionItem;
		qi::rule<Iterator, TNormalBodyOptionList(), ascii::space_type> exactBodyOptionList;
		qi::rule<Iterator, TNormalBodyOption(), ascii::space_type> normalBodyOption;
		qi::rule<Iterator, TBodyOption(), ascii::space_type> bodyOption;
		qi::rule<Iterator, triggerT(), ascii::space_type> trigger;
		qi::rule<Iterator, bodyTbody(), ascii::space_type> body;
		qi::rule<Iterator, instructionT(), ascii::space_type> instruction;
		qi::rule<Iterator, receiverT(), ascii::space_type> receiver;
		qi::rule<Iterator, postOBtriggerT(), ascii::space_type> postOBtrigger;
		qi::rule<Iterator, commandT(), ascii::space_type> command;
		qi::rule<Iterator, lineT(), ascii::space_type> rline;
	};
};

void ERMParser::parseLine( const std::string & line )
{
	std::string::const_iterator beg = line.begin(),
		end = line.end();

	ERM::ERM_grammar<std::string::const_iterator> ERMgrammar;
	ERM::lineT AST;

// 	bool r = qi::phrase_parse(beg, end, ERMgrammar, ascii::space, AST);
// 	if(!r || beg != end)
// 	{
// 		tlog1 << "Parse error for line (" << parsedLine << ") : " << line << std::endl;
// 		tlog1 << "\tCannot parse: " << std::string(beg, end) << std::endl;
// 	}
// 	else
// 	{
// 		//parsing succeeded
// 		//ERM::printLineAST(AST);
// 	}
}

ERMParser::ELineType ERMParser::classifyLine( const std::string & line, bool inString ) const
{
	ERMParser::ELineType ret;
	if(line[0] == '!')
	{	
		if(countHatsBeforeSemicolon(line) % 2 == 1)
		{
			ret = ERMParser::UNFINISHED_STRING;
		}
		else
		{
			ret = ERMParser::COMMAND_FULL;
		}
	}
	else
	{
		if(inString)
		{
			if(countHatsBeforeSemicolon(line) % 2 == 1)
			{
				ret = ERMParser::END_OF_STRING;
			}
			else
			{
				ret = ERMParser::UNFINISHED_STRING;
			}
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
