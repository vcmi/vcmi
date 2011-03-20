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



namespace ERM
{

	typedef std::vector<int> identifierT;

	struct triggerT
	{
		std::string name;
		boost::optional<identifierT> identifier;
		boost::optional<std::string> condition;
	};

	//a dirty workaround for preprocessor magic that prevents the use types with comma in it in BOOST_FUSION_ADAPT_STRUCT
	//see http://comments.gmane.org/gmane.comp.lib.boost.user/62501 for some info
	//
	//moreover, I encountered a quite serious bug in boost: http://boost.2283326.n4.nabble.com/container-hpp-111-error-C2039-value-type-is-not-a-member-of-td3352328.html
	//not sure how serious it is...

	typedef boost::variant<char, std::string> bodyItem;
 	typedef std::vector<bodyItem> bodyTbody;

	struct instructionT
	{
		std::string name;
		boost::optional<identifierT> identifier;
		boost::optional<std::string> condition;
		bodyTbody body;
	};

	struct receiverT
	{
		std::string name;
		boost::optional<identifierT> identifier;
		boost::optional<std::string> condition;
		bodyTbody body;
	};

	struct postOBtriggerT
	{
		boost::optional<identifierT> identifier;
		boost::optional<std::string> condition;
	};

	typedef	boost::variant<
			triggerT,
			instructionT,
			receiverT,
			postOBtriggerT,
			qi::unused_type
			>
			commandT;

	typedef boost::variant<commandT, qi::unused_type> lineT;


	//console printer

	void identifierPrinter(const identifierT & id)
	{
		BOOST_FOREACH(int x, id)
		{
			tlog2 << "\\" << x;
		}
	}

	struct CommandPrinter : boost::static_visitor<>
	{
		void operator()(triggerT const& trig) const
		{
			tlog2 << "trigger: " << trig.name << " identifier: ";
			identifierPrinter(*trig.identifier);
			tlog2 << " condition: " << *trig.condition;

		}

		void operator()(instructionT const& trig) const
		{
			tlog2 << "instruction: " << trig.name << " identifier: ";
			identifierPrinter(*trig.identifier);
			tlog2 << " condition: " << *trig.condition;
			tlog2 << " body items:";
			BOOST_FOREACH(bodyItem bi, trig.body)
			{
				tlog2 << " " << bi;
			}
		}

		void operator()(receiverT const& trig) const
		{
			tlog2 << "receiver: " << trig.name << " identifier: ";
			identifierPrinter(*trig.identifier);
			tlog2 << " condition: " << *trig.condition;

		}

		void operator()(postOBtriggerT const& trig) const
		{
			tlog2 << "post OB trigger; " << " identifier: ";
			identifierPrinter(*trig.identifier);
			tlog2 << " condition: " << *trig.condition;

		}

		void operator()(qi::unused_type const& text) const
		{
			//tlog2 << "Empty line/comment line" << std::endl;
		}

	};

	struct ERMprinter : boost::static_visitor<>
	{
		void operator()(commandT const& cmd) const
		{
			CommandPrinter()(cmd);

		}

		void operator()(qi::unused_type const& nothing) const
		{
			//tlog2 << "Empty line/comment line" << std::endl;
		}
	};

	void printLineAST(const lineT & ast)
	{
		tlog2 << "";
		ast.apply_visitor(ERMprinter());
	}
}

BOOST_FUSION_ADAPT_STRUCT(
	ERM::triggerT,
	(std::string, name)
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<std::string>, condition)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::instructionT,
	(std::string, name)
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<std::string>, condition)
	(ERM::bodyTbody, body)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::receiverT,
	(std::string, name)
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<std::string>, condition)
	(ERM::bodyTbody, body)
	)

BOOST_FUSION_ADAPT_STRUCT(
	ERM::postOBtriggerT,
	(boost::optional<ERM::identifierT>, identifier)
	(boost::optional<std::string>, condition)
	)

namespace ERM
{
	template<typename Iterator>
	struct ERM_grammar : qi::grammar<Iterator, lineT()>
	{
		ERM_grammar() : ERM_grammar::base_type(rline, "ERM script line")
		{
 			comment = *(qi::char_);
 			commentLine = ~qi::char_('!') >> comment;
 			cmdName %= +qi::char_/*qi::repeat(2)[qi::char_]*/;
			identifier %= (qi::int_ % qi::lit('/'));
			condition %= qi::lit('&') >> +qi::char_("0-9a-zA-Z&/|<>=") >> qi::lit(":;");
			trigger %= cmdName >> -identifier >> -condition; /////
			string %= qi::lexeme['^' >> +(qi::char_ - '^') >> '^'];
			body %= *( qi::char_("a-zA-Z0-9/ ") | string);
			instruction %= cmdName >> -identifier >> -condition >> body;
			receiver %= cmdName >> -identifier >> -condition >> body;
			postOBtrigger %= qi::lit("$OB") >> -identifier >> -condition;
			command %= (qi::char_('!') >>
					(
						(qi::char_('?') >> trigger) |
						(qi::char_('!') >> instruction) |
						(qi::char_('#') >> receiver) |
						postOBtrigger
					) >> comment
				);

			rline %=
				(
					command | commentLine | spirit::eoi
				);

			//error handling

			string.name("string constant");
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

		qi::rule<Iterator, std::string()> string;

		qi::rule<Iterator, void()> comment;
		qi::rule<Iterator, void()> commentLine;
		qi::rule<Iterator, std::string()> cmdName;
		qi::rule<Iterator, identifierT()> identifier;
		qi::rule<Iterator, std::string()> condition;
		qi::rule<Iterator, triggerT()> trigger;
		qi::rule<Iterator, bodyTbody()> body;
		qi::rule<Iterator, instructionT()> instruction;
		qi::rule<Iterator, receiverT()> receiver;
		qi::rule<Iterator, postOBtriggerT()> postOBtrigger;
		qi::rule<Iterator, commandT()> command;
		qi::rule<Iterator, lineT()> rline;
	};
};

void ERMParser::parseLine( std::string line )
{
	std::string::const_iterator beg = line.begin(),
		end = line.end();

	ERM::ERM_grammar<std::string::const_iterator> ERMgrammar;
	ERM::lineT AST;

	bool r = qi::parse(beg, end, ERMgrammar, AST);
	if(!r || beg != end)
	{
		tlog1 << "Parse error for line " << line << std::endl;
		tlog1 << "\tCannot parse: " << std::string(beg, end) << std::endl;
	}
	else
	{
		//parsing succeeded
		//ERM::printLineAST(AST);
	}
}
