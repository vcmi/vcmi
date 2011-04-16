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
using namespace VERMInterpreter;

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
				FileInfo * finfo = new FileInfo;
				finfo->filename = dir->path().string();

				std::vector<ERM::TLine> buf = ep.parseFile();
				finfo->length = buf.size();
				files.push_back(finfo);

				for(int g=0; g<buf.size(); ++g)
				{
					scripts[LinePointer(finfo, g)] = buf[g];
				}
			}
		}
	}
}

void ERMInterpreter::printScripts( EPrintMode mode /*= EPrintMode::ALL*/ )
{
	std::map< LinePointer, ERM::TLine >::const_iterator prevIt;
	for(std::map< LinePointer, ERM::TLine >::const_iterator it = scripts.begin(); it != scripts.end(); ++it)
	{
		if(it == scripts.begin() || it->first.file != prevIt->first.file)
		{
			tlog2 << "----------------- script " << it->first.file->filename << " ------------------\n";
		}
		
		ERMPrinter::printAST(it->second);
		prevIt = it;
	}
}

struct ScriptScanner : boost::static_visitor<>
{
	ERMInterpreter * interpreter;
	LinePointer lp;

	ScriptScanner(ERMInterpreter * interpr, const LinePointer & _lp) : interpreter(interpr), lp(_lp)
	{}

	void operator()(TVExp const& cmd) const
	{
		//
	}
	void operator()(TERMline const& cmd) const
	{
		if(cmd.which() == 0) //TCommand
		{
			Tcommand tcmd = boost::get<Tcommand>(cmd);
			switch (tcmd.cmd.which())
			{
			case 0: //trigger
				{
					Trigger trig;
					trig.line = lp;
					interpreter->triggers[ TriggerType(boost::get<ERM::Ttrigger>(tcmd.cmd).name) ].push_back(trig);
				}
				break;
			case 3: //post trigger
				{
					Trigger trig;
					trig.line = lp;
					interpreter->postTriggers[ TriggerType(boost::get<ERM::TPostTrigger>(tcmd.cmd).name) ].push_back(trig);
				}
				break;
			default:

				break;
			}
		}
		
	}
};

void ERMInterpreter::scanScripts()
{
	for(std::map< LinePointer, ERM::TLine >::const_iterator it = scripts.begin(); it != scripts.end(); ++it)
	{
		boost::apply_visitor(ScriptScanner(this, it->first), it->second);
	}
}

ERMInterpreter::ERMInterpreter()
{
	globalEnv = new Environment();
}

void ERMInterpreter::executeTrigger( Trigger & trig )
{
	for(LinePointer lp = trig.line; lp.isValid(); ++lp)
	{
		ERM::TLine curLine = retrieveLine(lp);
		if(isATrigger(curLine))
			break;

		executeLine(lp);
	}
}

bool ERMInterpreter::isATrigger( const ERM::TLine & line )
{
	switch(line.which())
	{
	case 0: //v-exp
		{
			TVExp vexp = boost::get<TVExp>(line);
			if(vexp.children.size() == 0)
				return false;

			switch (getExpType(vexp.children[0]))
			{
			case SYMBOL:
				{
					//TODO: what about sym modifiers?
					//TOOD: macros?
					ERM::TSymbol sym = boost::get<ERM::TSymbol>(vexp.children[0]);
					return sym.sym == triggerSymbol || sym.sym == postTriggerSymbol;
				}
				break;
			case TCMD:
				return isCMDATrigger( boost::get<ERM::Tcommand>(vexp.children[0]) );
				break;
			default:
				return false;
				break;
			}
		}
		break;
	case 1: //erm
		{
			TERMline line = boost::get<TERMline>(line);
			switch(line.which())
			{
			case 0: //tcmd
				return isCMDATrigger( boost::get<ERM::Tcommand>(line) );
				break;
			default:
				return false;
				break;
			}
		}
		break;
	default:
		assert(0); //it should never happen
		break;
	}
	assert(0);
}

ERM::EVOtions ERMInterpreter::getExpType( const ERM::TVOption & opt )
{
	//MAINTENANCE: keep it correct!
	return static_cast<ERM::EVOtions>(opt.which());
}

bool ERMInterpreter::isCMDATrigger( const ERM::Tcommand & cmd )
{
	switch (cmd.cmd.which())
	{
	case 0: //trigger
	case 3: //post trigger
		return true;
		break;
	default:
		return false;
		break;
	}
}

ERM::TLine ERMInterpreter::retrieveLine( LinePointer linePtr ) const
{
	return *scripts.find(linePtr);
}

/////////
//code execution

struct ERMExpDispatch : boost::static_visitor<>
{
	void operator()(Ttrigger const& trig) const
	{
		//the first executed line, check if we should proceed
	}
	void operator()(Tinstruction const& trig) const
	{
	}
	void operator()(Treceiver const& trig) const
	{
	}
	void operator()(TPostTrigger const& trig) const
	{
	}
};

struct CommandExec : boost::static_visitor<>
{
	void operator()(Tcommand const& cmd) const
	{
		boost::apply_visitor(ERMExpDispatch(), cmd.cmd);
		std::cout << "Line comment: " << cmd.comment << std::endl;
	}
	void operator()(std::string const& comment) const
	{
		//comment - do nothing
	}
	void operator()(spirit::unused_type const& nothing) const
	{
		//nothing - do nothing
	}
};

struct LineExec : boost::static_visitor<>
{
	void operator()(TVExp const& cmd) const
	{
		//printTVExp(cmd);
	}
	void operator()(TERMline const& cmd) const
	{
		boost::apply_visitor(CommandExec(), cmd);
	}
};

/////////

void ERMInterpreter::executeLine( const LinePointer & lp )
{
	boost::apply_visitor(LineExec(), scripts[lp]);
}

void ERMInterpreter::init()
{
	ermGlobalEnv = new ERMEnvironment();
	globalEnv = new Environment();
	//TODO: reset?
}

struct ERMExecEnvironment
{
	ERMEnvironment * ermGlobalEnv;
	Trigger * trigEnv;
	ERMExecEnvironment(ERMEnvironment * erm, Trigger * trig = NULL) : ermGlobalEnv(erm), trigEnv(trig)
	{}
};

template<typename T>
struct LVL2GetIexpDisemboweler : boost::static_visitor<>
{
	T & out;
	const ERMExecEnvironment * env;
	LVL2GetIexpDisemboweler(T & ret, const ERMExecEnvironment * _env) : out(ret), env(_env) //writes value to given var
	{}

	void processNotMacro(const TVarExpNotMacro & val) const
	{
		if(val.questionMark.is_initialized())
			throw EIexpGetterProblem("Question marks ('?') are not allowed in getter i-expressions");

		//TODO: finish it
	}

	void operator()(TVarExpNotMacro const& val) const
	{
		processNotMacro(val);
	}
	void operator()(TMacroUsage const& val) const
	{
		std::map<std::string, ERM::TVarExpNotMacro>::const_iterator it =
			env->ermGlobalEnv->macroBindings.find(val	.macro);
		if(it == env->ermGlobalEnv->macroBindings.end())
			throw EUsageOfUndefinedMacro(val.macro);
		else
			processNotMacro(it->second);
	}
};

template<typename T>
struct LVL1GetIexpDisemboweler : boost::static_visitor<>
{
	T & out;
	const ERMExecEnvironment * env;
	LVL1GetIexpDisemboweler(T & ret, const ERMExecEnvironment * _env) : out(ret), env(_env) //writes value to given var
	{}
	void operator()(int const & constant) const
	{
		out = constant;
	}
	void operator()(TVarExp const & var) const
	{
		
	}
};

template<typename T>
T ERMInterpreter::getIexp( const ERM::TIexp & iexp, const Trigger * trig /*= NULL*/ ) const
{
	T ret;
	boost::apply_visitor(LVL1GetIexpDisemboweler(ret, ERMExecEnvironment(ermGlobalEnv, trig)), iexp);
	return ret;
}

const std::string ERMInterpreter::triggerSymbol = "trigger";
const std::string ERMInterpreter::postTriggerSymbol = "postTrigger";
const std::string ERMInterpreter::defunSymbol = "defun";


struct TriggerIdMatchHelper : boost::static_visitor<>
{
	int & ret;
	TriggerIdMatchHelper(int & b) : ret(b)
	{}

	void operator()(TIexp const& iexp) const
	{
		
	}
	void operator()(TArithmeticOp const& arop) const
	{
		//error?!?
	}
};

bool TriggerIdentifierMatch::tryMatch( const ERM::Ttrigger & trig ) const
{
	if(trig.identifier.is_initialized())
	{
		ERM::Tidentifier tid = trig.identifier.get();
		std::map< int, std::vector<int> >::const_iterator it = matchToIt.find(tid.size());
		if(it == matchToIt.end())
			return false;
		else
		{
			const std::vector<int> & pattern = it->second;
			for(int g=0; g<pattern.size(); ++g)
			{
				int val = -1;
				boost::apply_visitor(TriggerIdMatchHelper(val), tid[g]);
				return pattern[g] == val;
			}
		}
	}
	else
	{
		if(allowNoIdetifier)
			return true;
	}
}
