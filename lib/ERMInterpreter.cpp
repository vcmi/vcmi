#include "ERMInterpreter.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

/*
 * ERMInterpreter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

namespace spirit = boost::spirit;

namespace VERMInterpreter
{
	using namespace ERM;

	//different exceptions that can be thrown during interpreting
	class EInterpreterProblem : public std::exception
	{};

	class ESymbolNotFound : public EInterpreterProblem
	{
		std::string problem;
	public:
		ESymbolNotFound(const std::string & sym) : problem(std::string("Symbol ") + sym + std::string(" not found!"))
		{}

		const char * what() const throw() OVERRIDE
		{
			return problem.c_str();
		}
	};


	///main environment class, manages symbols
	class Environment
	{
	private:
		std::map<std::string, TVOption> symbols;
		Environment * lexicalParent;

	public:
		bool isBound(const std::string & name, bool globalOnly) const
		{
			std::map<std::string, TVOption>::const_iterator it = symbols.find(name);
			if(globalOnly && lexicalParent)
			{
				return lexicalParent->isBound(name, globalOnly);
			}

			//we have it; if globalOnly is true, lexical parent is false here so we are global env
			if(it != symbols.end())
				return true;

			//here, we don;t have it; but parent can have
			if(lexicalParent)
				return lexicalParent->isBound(name, globalOnly);

			return false;
		}

		TVOption retrieveValue(const std::string & name) const
		{
			std::map<std::string, TVOption>::const_iterator it = symbols.find(name);
			if(it == symbols.end())
			{
				if(lexicalParent)
				{
					return lexicalParent->retrieveValue(name);
				}

				throw ESymbolNotFound(name);
			}
			return it->second;
		}
		
		///returns true if symbols was really unbound
		enum EUnbindMode{LOCAL, RECURSIVE_UNTIL_HIT, FULLY_RECURSIVE};
		bool unbind(const std::string & name, EUnbindMode mode)
		{
			if(isBound(name, false))
			{
				if(symbols.find(name) != symbols.end()) //result of isBound could be from higher lexical env
					symbols.erase(symbols.find(name));

				if(mode == FULLY_RECURSIVE && lexicalParent)
					lexicalParent->unbind(name, mode);

				return true;
			}
			if(lexicalParent && (mode == RECURSIVE_UNTIL_HIT || mode == FULLY_RECURSIVE))
				return lexicalParent->unbind(name, mode);

			//neither bound nor have lexical parent
			return false;
		}
	};

//		All numeric variables are integer variables and have a range of -2147483647...+2147483647
//		c			stores game active day number			//indirect variable
//		d			current value							//not an actual variable but a modifier
// 		e1..e100 	Function floating point variables		//local
// 		e-1..e-100	Trigger local floating point variables	//local
// 		'f'..'t'	Standard variables ('quick variables')	//global
// 		v1..v1000	Standard variables						//global
// 		w1..w100	Hero variables
// 		w101..w200	Hero variables
// 		x1..x16		Function parameters						//local
// 		y1..y100	Function local variables				//local
// 		y-1..y-100	Trigger-based local integer variables	//local
// 		z1..z1000	String variables						//global
// 		z-1..z-10	Function local string variables			//local

	struct TriggerLocalVars
	{
		static const int EVAR_NUM = 100; //number of evar locals
		double evar[EVAR_NUM]; //negative indices
		
		static const int YVAR_NUM = 100; //number of yvar locals
		int yvar[YVAR_NUM];
	};

	struct FunctionLocalVars
	{
		static const int NUM_PARAMETERS = 16; //number of function parameters
		int params[NUM_PARAMETERS]; //x-vars

		static const int NUM_LOCALS = 100;
		int locals[NUM_LOCALS]; //y-vars

		static const int NUM_STRINGS = 10;
		std::string strings[NUM_STRINGS]; //z-vars (negative indices)

		static const int NUM_FLOATINGS = 100;
		double floats[NUM_FLOATINGS]; //e-vars (positive indices)
	};

	struct ERMEnvironment
	{
		static const int NUM_QUICKS = 't' - 'f' + 1; //it should be 15
		int quickVars[NUM_QUICKS]; //referenced by letter ('f' to 't' inclusive)

		static const int NUM_STANDARDS = 1000;
		int standardVars[NUM_STANDARDS]; //v-vars

		static const int NUM_STRINGS = 1000;
		std::string strings[NUM_STRINGS]; //z-vars (positive indices)
	};

	//call stack
	class Stack
	{
		std::vector<int> entryPoints; //defines how to pass to current location
		Environment * env; //most nested VERM environment
	};
}



namespace ERMPrinter
{
	//console printer
	using namespace ERM;

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
		void operator()(spirit::unused_type const& cmp) const
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
		void operator()(spirit::unused_type const& nothing) const
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
			BOOST_FOREACH(TVModifier mod, cmd.symModifier)
			{
				tlog2 << mod << " ";
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
		BOOST_FOREACH(TVModifier mod, exp.modifier)
		{
			tlog2 << mod << " ";
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
		tlog2 << std::endl;
	}
}

void ERMInterpreter::scanForScripts()
{
	using namespace boost::filesystem;
	//parser checking
	if(!exists(DATA_DIR "/Data/s/"))
	{
		tlog3 << "Warning: Folder " DATA_DIR "/Data/s/ doesn't exist!\n";
		return;
	}
	directory_iterator enddir;
	for (directory_iterator dir(DATA_DIR "/Data/s"); dir!=enddir; dir++)
	{
		if(is_regular(dir->status()))
		{
			std::string name = dir->path().leaf();
			if( boost::algorithm::ends_with(name, ".erm") ||
				boost::algorithm::ends_with(name, ".verm") )
			{
				ERMParser ep(dir->path().string());
				scripts[name] = ep.parseFile();
			}
		}
	}
}

void ERMInterpreter::printScripts( EPrintMode mode /*= EPrintMode::ALL*/ )
{
	for(std::map<std::string, std::vector<ERM::TLine> >::const_iterator it = scripts.begin(); it != scripts.end(); ++it)
	{
		tlog2 << "----------------- script " << it->first << " ------------------\n";
		for(int i=0; i<it->second.size(); ++i)
		{
			ERMPrinter::printAST(it->second[i]);
		}
	}
}
