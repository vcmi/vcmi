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

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace ascii = spirit::ascii;
namespace phoenix = boost::phoenix;


//Greenspun's Tenth Rule of Programming:
//Any sufficiently complicated C or Fortran program contains an ad hoc, informally-specified,
//bug-ridden, slow implementation of half of Common Lisp.
//actually these macros help in dealing with boost::variant


#define BEGIN_TYPE_CASE(LinePrinterVisitor) struct LinePrinterVisitor : boost::static_visitor<> \
{

#define FOR_TYPE(TYPE, VAR) void operator()(TYPE const& VAR) const

#define DO_TYPE_CASE(LinePrinterVisitor, VAR) } ___UN; boost::apply_visitor(___UN, VAR);



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
	if(std::string(header) != "ZVSE" && std::string(header) != "VERM")
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
				repairEncoding(lineBuf, ARRAY_COUNT(lineBuf));
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
				repairEncoding(wholeLine);
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
	struct TStringConstant
	{
		std::string str;
	};
	struct TMacroUsage
	{
		std::string macro;
	};

	//macro with '?', for write only
	struct TQMacroUsage
	{
		std::string qmacro;
	};

	//definition of a macro
	struct TMacroDef
	{
		std::string macro;
	};
	typedef std::string TCmdName;
	
	struct TVarExpNotMacro
	{
		typedef boost::optional<int> Tval;
		boost::optional<char> questionMark;
		std::string varsym;
		Tval val;
	};

	typedef boost::variant<TVarExpNotMacro, TMacroUsage> TVarExp;

	//write-only variable expression
	struct TVarpExp
	{
		typedef boost::variant<TVarExpNotMacro, TQMacroUsage> Tvartype;
		Tvartype var;
	};

	//i-expression (identifier expression) - an integral constant, variable symbol or array symbol
	typedef boost::variant<TVarExp, int> TIexp;

	struct TArithmeticOp
	{
		TIexp lhs, rhs;
		char opcode;
	};

	struct TVRLogic
	{
		char opcode;
		TIexp var;
	};

	struct TVRArithmetic
	{
		char opcode;
		TIexp rhs;
	};

	struct TSemiCompare
	{
		std::string compSign;
		TIexp rhs;
	};

	struct TCurriedString
	{
		TIexp iexp;
		TStringConstant string;
	};

	struct TVarConcatString 
	{
		TVarExp var;
		TStringConstant string;
	};

	typedef boost::variant<TVarConcatString, TStringConstant, TCurriedString, TSemiCompare, TMacroUsage, TMacroDef, TIexp, TVarpExp, qi::unused_type> TBodyOptionItem;

	typedef std::vector<TBodyOptionItem> TNormalBodyOptionList;

	struct TNormalBodyOption
	{
		char optionCode;
		TNormalBodyOptionList params;
	};
	typedef boost::variant<TVRLogic, TVRArithmetic, TNormalBodyOption> TBodyOption;

	typedef boost::variant<TIexp, TArithmeticOp > TIdentifierInternal;
	typedef std::vector< TIdentifierInternal > Tidentifier;

	struct TComparison
	{
		std::string compSign;
		TIexp lhs, rhs;
	};

	struct Tcondition;
	typedef
		boost::optional<
		boost::recursive_wrapper<Tcondition>
		>
		TconditionNode;


	struct Tcondition
	{
		typedef boost::variant<
			TComparison,
			int>
			Tcond; //comparison or condition flag
		char ctype;
		Tcond cond;
		TconditionNode rhs;
	};

	struct Ttrigger
	{
		TCmdName name;
		boost::optional<Tidentifier> identifier;
		boost::optional<Tcondition> condition;
	};

	//a dirty workaround for preprocessor magic that prevents the use types with comma in it in BOOST_FUSION_ADAPT_STRUCT
	//see http://comments.gmane.org/gmane.comp.lib.boost.user/62501 for some info
	//
	//moreover, I encountered a quite serious bug in boost: http://boost.2283326.n4.nabble.com/container-hpp-111-error-C2039-value-type-is-not-a-member-of-td3352328.html
	//not sure how serious it is...

	//typedef boost::variant<char, TStringConstant, TMacroUsage, TMacroDef> bodyItem;
 	typedef std::vector<TBodyOption> Tbody;

	struct Tinstruction
	{
		TCmdName name;
		boost::optional<Tidentifier> identifier;
		boost::optional<Tcondition> condition;
		Tbody body;
	};

	struct Treceiver
	{
		TCmdName name;
		boost::optional<Tidentifier> identifier;
		boost::optional<Tcondition> condition;
		boost::optional<Tbody> body;
	};

	struct TPostTrigger
	{
		TCmdName name;
		boost::optional<Tidentifier> identifier;
		boost::optional<Tcondition> condition;
	};

	struct Tcommand
	{
		typedef	boost::variant<
			Ttrigger,
			Tinstruction,
			Treceiver,
			TPostTrigger
		>
		Tcmd;
		Tcmd cmd;
		std::string comment;
	};

	//vector expression


	typedef boost::variant<Tcommand, std::string, qi::unused_type> TERMline;

	typedef std::string TVModifier; //'`', ',', ',@', '#''

	struct TSymbol
	{
		boost::optional<TVModifier> symModifier;
		std::string sym;
	};

	//for #'symbol expression

	struct TVExp;
	typedef boost::variant<boost::recursive_wrapper<TVExp>, TSymbol, char, double, int, Tcommand, TStringConstant > TVOption; //options in v-expression
	//v-expression
	struct TVExp
	{
		boost::optional<TVModifier> modifier;
		std::vector<TVOption> children;
	};

	//script line
	typedef boost::variant<TVExp, TERMline> TLine;

	//console printer

	struct VarPrinterVisitor : boost::static_visitor<>
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
			tlog2 << "$" << val.macro << "&";
		}
	};

	void varPrinter(const TVarExp & var)
	{
		boost::apply_visitor(VarPrinterVisitor(), var);
	}

	struct IExpPrinterVisitor : boost::static_visitor<>
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


	void iexpPrinter(const TIexp & exp)
	{
		boost::apply_visitor(IExpPrinterVisitor(), exp);
	}

	struct IdentifierPrinterVisitor : boost::static_visitor<>
	{
		void operator()(TIexp const& iexp) const
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

	void identifierPrinter(const boost::optional<Tidentifier> & id)
	{
		if(id.is_initialized())
		{
			tlog2 << "identifier: ";
			BOOST_FOREACH(TIdentifierInternal x, id.get())
			{
				tlog2 << "#";
				boost::apply_visitor(IdentifierPrinterVisitor(), x);
			}
		}
	}

	struct ConditionCondPrinterVisitor : boost::static_visitor<>
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

	void conditionPrinter(const boost::optional<Tcondition> & cond)
	{
		if(cond.is_initialized())
		{
			Tcondition condp = cond.get();
			tlog2 << " condition: ";
			boost::apply_visitor(ConditionCondPrinterVisitor(), condp.cond);
			tlog2 << " cond type: " << condp.ctype;
			
			//recursive call
			if(condp.rhs.is_initialized())
			{
				tlog2 << "rhs: ";
				boost::optional<Tcondition> rhsc = condp.rhs.get().get();
				conditionPrinter(rhsc);
			}
			else
			{
				tlog2 << "no rhs; ";
			}
		}
	}

	struct BodyVarpPrinterVisitor : boost::static_visitor<>
	{
		void operator()(TVarExpNotMacro const& cmp) const
		{
			if(cmp.questionMark.is_initialized())
			{
				tlog2 << cmp.questionMark.get();
			}
			if(cmp.val.is_initialized())
			{
				tlog2 << "val:" << cmp.val.get();
			}
			tlog2 << "varsym: |" << cmp.varsym << "|";
		}
		void operator()(TQMacroUsage const& cmp) const
		{
			tlog2 << "???$$" << cmp.qmacro << "$$";
		}
	};

	struct BodyOptionItemPrinterVisitor : boost::static_visitor<>
	{
		void operator()(TVarConcatString const& cmp) const
		{
			tlog2 << "+concat\"";
			varPrinter(cmp.var);
			tlog2 << " with " << cmp.string.str;
		}
		void operator()(TStringConstant const& cmp) const
		{
			tlog2 << " \"" << cmp.str << "\" ";
		}
		void operator()(TCurriedString const& cmp) const
		{
			tlog2 << "cs: ";
			iexpPrinter(cmp.iexp);
			tlog2 << " '" << cmp.string.str << "' ";
		}
		void operator()(TSemiCompare const& cmp) const
		{
			tlog2 << cmp.compSign << "; rhs: ";
			iexpPrinter(cmp.rhs);
		}
		void operator()(TMacroUsage const& cmp) const
		{
			tlog2 << "$$" << cmp.macro << "$$";
		}
		void operator()(TMacroDef const& cmp) const
		{
			tlog2 << "@@" << cmp.macro << "@@";
		}
		void operator()(TIexp const& cmp) const
		{
			iexpPrinter(cmp);
		}
		void operator()(TVarpExp const& cmp) const
		{
			tlog2 << "varp";
			boost::apply_visitor(BodyVarpPrinterVisitor(), cmp.var);
		}
		void operator()(qi::unused_type const& cmp) const
		{
			tlog2 << "nothing";
		}
	};

	struct BodyOptionVisitor : boost::static_visitor<>
	{
		void operator()(TVRLogic const& cmp) const
		{
			tlog2 << cmp.opcode << " ";
			iexpPrinter(cmp.var);
		}
		void operator()(TVRArithmetic const& cmp) const
		{
			tlog2 << cmp.opcode << " ";
			iexpPrinter(cmp.rhs);
		}
		void operator()(TNormalBodyOption const& cmp) const
		{
			tlog2 << cmp.optionCode << "~";
			BOOST_FOREACH(TBodyOptionItem optList, cmp.params)
			{
				boost::apply_visitor(BodyOptionItemPrinterVisitor(), optList);
			}
		}
	};

	void bodyPrinter(const Tbody & body)
	{
		tlog2 << " body items: ";
		BOOST_FOREACH(TBodyOption bi, body)
		{
			tlog2 << " (";
			apply_visitor(BodyOptionVisitor(), bi);
			tlog2 << ") ";
		}
	}

	struct CommandPrinterVisitor : boost::static_visitor<>
	{
		void operator()(Ttrigger const& trig) const
		{
			tlog2 << "trigger: " << trig.name << " ";
			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);
		}
		void operator()(Tinstruction const& trig) const
		{
			tlog2 << "instruction: " << trig.name << " ";
			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);
			bodyPrinter(trig.body);
			
		}
		void operator()(Treceiver const& trig) const
		{
			tlog2 << "receiver: " << trig.name << " ";

			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);
			if(trig.body.is_initialized())
				bodyPrinter(trig.body.get());
		}
		void operator()(TPostTrigger const& trig) const
		{
			tlog2 << "post trigger: " << trig.name << " ";
			identifierPrinter(trig.identifier);
			conditionPrinter(trig.condition);
		}
	};

	struct LinePrinterVisitor : boost::static_visitor<>
	{
		void operator()(Tcommand const& cmd) const
		{
			CommandPrinterVisitor un;
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

	void printERM(const TERMline & ast)
	{
		tlog2 << "";
		
		boost::apply_visitor(LinePrinterVisitor(), ast);
	}

	void printTVExp(const TVExp & exp);

	struct VOptionPrinterVisitor : boost::static_visitor<>
	{
		void operator()(TVExp const& cmd) const
		{
			printTVExp(cmd);
		}
		void operator()(TSymbol const& cmd) const
		{
			if(cmd.symModifier.is_initialized())
			{
				tlog2 << cmd.symModifier.get();
			}
			tlog2 << cmd.sym;
		}
		void operator()(char const& cmd) const
		{
			tlog2 << "'" << cmd << "'";
		}
		void operator()(int const& cmd) const
		{
			tlog2 << cmd;
		}
		void operator()(double const& cmd) const
		{
			tlog2 << cmd;
		}
		void operator()(TERMline const& cmd) const
		{
			printERM(cmd);
		}
		void operator()(TStringConstant const& cmd) const
		{
			tlog2 << "^" << cmd.str << "^";
		}
	};

	void printTVExp(const TVExp & exp)
	{
		if(exp.modifier.is_initialized())
		{
			tlog2 << exp.modifier.get();
		}
		tlog2 << "[ ";
		BOOST_FOREACH(TVOption opt, exp.children)
		{
			boost::apply_visitor(VOptionPrinterVisitor(), opt);
			tlog2 << " ";
		}
		tlog2 << "]";
	}

	struct TLPrinterVisitor : boost::static_visitor<>
	{
		void operator()(TVExp const& cmd) const
		{
			printTVExp(cmd);
		}
		void operator()(TERMline const& cmd) const
		{
			printERM(cmd);
		}
	};

	void printAST(const TLine & ast)
	{
		boost::apply_visitor(TLPrinterVisitor(), ast);
	}
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
	(boost::optional<ERM::TVModifier>, modifier)
	(std::vector<ERM::TVOption>, children)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::TSymbol,
	(boost::optional<ERM::TVModifier>, symModifier)
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
			macroUsage %= qi::lexeme[qi::lit('$') >> *(qi::char_ - '$') >> qi::lit('$')];
			macroDef %= qi::lexeme[qi::lit('@') >> *(qi::char_ - '@') >> qi::lit('@')];
			varExpNotMacro %= -qi::char_("?") >> (+(qi::char_("a-z") - 'u')) >> -qi::int_;
			qMacroUsage %= qi::lexeme[qi::lit("?$") >> *(qi::char_ - '$') >> qi::lit('$')];
			varExp %= varExpNotMacro | macroUsage;
			iexp %= varExp | qi::int_;
			varp %=/* qi::lit("?") >> */(varExpNotMacro | qMacroUsage);
 			comment %= *qi::char_;
			commentLine %= (~qi::char_("!@") >> comment | (qi::char_('!') >> (~qi::char_("?!$#")) >> comment ));
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
			bodyOptionItem %= varConcatString | curStr | string | semiCompare | macroUsage | macroDef | varp | iexp | qi::eps;
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

			vmod %= qi::string("`") | qi::string(",") | qi::string(",@") | qi::string("#'");
			vsym %= -vmod >> qi::lexeme[+qi::char_("+*/$%&_=<>~a-zA-Z0-9-")];

			qi::real_parser<double, qi::strict_real_policies<double> > strict_double;
			vopt %= qi::lexeme[(qi::lit("!") >> qi::char_ >> qi::lit("!"))] | qi::lexeme[strict_double] | qi::lexeme[qi::int_] | command | vexp | string | vsym;
			vexp %= -vmod >> qi::lit("[") >> *(vopt) >> qi::lit("]");

			vline %= (( qi::lit("@") >>vexp) | rline ) > spirit::eoi;

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

		qi::rule<Iterator, TMacroUsage(), ascii::space_type> macroUsage;
		qi::rule<Iterator, TQMacroUsage(), ascii::space_type> qMacroUsage;
		qi::rule<Iterator, TMacroDef(), ascii::space_type> macroDef;
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

void ERMParser::parseLine( const std::string & line )
{
	std::string::const_iterator beg = line.begin(),
		end = line.end();

	ERM::ERM_grammar<std::string::const_iterator> ERMgrammar;
	ERM::TLine AST;

// 	bool r = qi::phrase_parse(beg, end, ERMgrammar, ascii::space, AST);
// 	if(!r || beg != end)
// 	{
// 		tlog1 << "Parse error for line (" << parsedLine << ") : " << line << std::endl;
// 		tlog1 << "\tCannot parse: " << std::string(beg, end) << std::endl;
// 	}
// 	else
// 	{
// 		//parsing succeeded
// 		tlog2 << line << std::endl;
// 		ERM::printAST(AST);
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
void ERMParser::parseFile(){}

#endif
