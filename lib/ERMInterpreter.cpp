#include "ERMInterpreter.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

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
	FunctionLocalVars * funcVars;
	ERMExecEnvironment(ERMEnvironment * erm, Trigger * trig = NULL, FunctionLocalVars * funvars = NULL)
		: ermGlobalEnv(erm), trigEnv(trig), funcVars(funvars)
	{}

	template<typename T>
	T* getVar(std::string toFollow, boost::optional<int> initVal)
	{
		int initV;
		bool hasInit = false;
		if(initVal.is_initialized())
		{
			initV = initVal.get();
			hasInit = true;
		}

		int endNum = 0;
		if(toFollow[0] == 'd')
		{
			endNum = 1;
			//TODO: support
		}
		T* ret;
		for(int b=toFollow.size()-1; b>=endNum; --b)
		{
			bool retIt = b == endNum+1; //if we should return the value are currently at

			char cr = toFollow[toFollow.size() - 1];
			if(cr == 'c')//write number of current day
			{
				//TODO
			}
			else if(cr == 'd') //info for external env - add i/o set
			{
				throw EIexpProblem("d inside i-expression not allowed!");
			}
			else if(cr == 'e')
			{
				if(hasInit)
				{
					if(retIt)
					{
						//these C-style cast is here just to shut up compiler errors
						if(initV > 0 && initV <= FunctionLocalVars::NUM_FLOATINGS)
						{
							if(funcVars)
								ret = (T*)funcVars->floats + initV - 1;
							else
								throw EIexpProblem("Function context not available!");
						}
						else if(initV < 0 && initV >= -TriggerLocalVars::EVAR_NUM)
						{
							if(trigEnv)
								ret = (T*)trigEnv->ermLocalVars.evar - initV + 1; //minus is important!
							else
								throw EIexpProblem("No trigger context available!");
						}
						else
							throw EIexpProblem("index " + boost::lexical_cast<std::string>(initV) + " not allowed for e array");
					}
					else
						throw EIexpProblem("e variables cannot appear in this context");
				}
				else
					throw EIexpProblem("e variables cannot appear in this context");
			}
			else if(cr >= 'f' && cr <= 't')
			{
				if(retIt)
					ret = &ermGlobalEnv->getQuickVar(cr);
				else
				{
					if(hasInit)
						throw EIexpProblem("quick variables cannot be used in this context");
					else
					{
						initV = ermGlobalEnv->getQuickVar(cr);
						hasInit = true;
					}
				}
			}
			else if(cr == 'v') //standard variables
			{
				if(hasInit)
				{
					if(initV > 0 && initV <= ERMEnvironment::NUM_STANDARDS)
					{
						if(retIt)
							ret = ermGlobalEnv->standardVars + initV - 1;
						else
							initV = ermGlobalEnv->standardVars[initV-1];
					}
					else
						throw EIexpProblem("standard variable index out of range");
				}
				else
					throw EIexpProblem("standard variable cannot be used in this context!");
			}
			else if(cr == 'w') //local hero variables
			{
				//TODO
			}
			else if(cr == 'x') //function parameters
			{
				if(hasInit)
				{
					if(initV > 0 && initV <= FunctionLocalVars::NUM_PARAMETERS)
					{
						if(funcVars)
						{
							if(retIt)
								ret = funcVars->params + initV-1;
							else
								initV = funcVars->params[initV-1];
						}
						else throw EIexpProblem("Function parameters cannot be used outside a function!");
					}
					else
						throw EIexpProblem("Parameter number out of range");
				}
				else
					throw EIexpProblem("Specify which function parameter should be used");
			}
			else if(cr == 'y')
			{
				if(hasInit)
				{
					if(initV > 0 && initV <= FunctionLocalVars::NUM_LOCALS)
					{
						if(funcVars)
						{
							if(retIt)
								ret = funcVars->locals + initV-1;
							else
								initV = funcVars->params[initV - 1];
						}
						else
							throw EIexpProblem("Function local variables cannot be used outside a function!");
					}
					else if(initV < 0 && initV >= -TriggerLocalVars::YVAR_NUM)
					{
						if(trigEnv)
						{
							if(retIt)
								ret = trigEnv->ermLocalVars.yvar - initV + 1;
							else
								initV = trigEnv->ermLocalVars.yvar[-initV + 1];
						}
						else
							throw EIexpProblem("Trigger local variables cannot be used outside triggers!");
					}
					else
						throw EIexpProblem("Wrong argument for function local variable!");
				}
				else
					throw EIexpProblem("y variable cannot be used in this context!");
			}
			else if(cr == 'z')
			{
				if(hasInit)
				{
					if(retIt)
					{
						//these C-style casts are here just to shut up compiler errors
						if(initV > 0 && initV <= ermGlobalEnv->NUM_STRINGS)
						{
							ret = (T*)ermGlobalEnv->strings + initV - 1;
						}
						else if(initV < 0 && initV >= -FunctionLocalVars::NUM_STRINGS)
						{
							if(funcVars)
							{
								ret = (T*)funcVars->strings + initV - 1;
							}
							else
								throw EIexpProblem("Function local string variables cannot be used outside functions!");
						}
						else
							throw EIexpProblem("Wrong parameter for string variable!");
					}
					else
						throw EIexpProblem("String variables can only be returned!");
				}
				else
					throw EIexpProblem("String variables cannot be used in this context!");
			}
			else
			{
				throw EIexpProblem(std::string("Symbol ") + cr + " is not allowed in this context!");
			}
			
		}
		return ret;
	}
};

namespace IexpDisemboweler
{
	enum EDir{GET, SET};
}

template<typename T>
struct LVL2IexpDisemboweler : boost::static_visitor<>
{
	T * inout;
	IexpDisemboweler::EDir dir;
	/*const*/ ERMExecEnvironment * env;

	LVL2IexpDisemboweler(T * in_out, /*const*/ ERMExecEnvironment * _env, IexpDisemboweler::EDir _dir)
		: inout(in_out), env(_env), dir(_dir) //writes value to given var
	{}

	void processNotMacro(const TVarExpNotMacro & val) const
	{
		if(val.questionMark.is_initialized())
			throw EIexpProblem("Question marks ('?') are not allowed in getter i-expressions");

		//const-cast just to do some code-reuse...
		*inout = *const_cast<ERMExecEnvironment*>(env)->getVar<T>(val.varsym, val.val);

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
struct LVL1IexpDisemboweler : boost::static_visitor<>
{
	T * inout;
	IexpDisemboweler::EDir dir;
	/*const*/ ERMExecEnvironment * env;

	LVL1IexpDisemboweler(T * in_out, /*const*/ ERMExecEnvironment * _env, IexpDisemboweler::EDir _dir)
		: inout(in_out), env(_env), dir(_dir) //writes value to given var
	{}
	void operator()(int const & constant) const
	{
		if(dir == IexpDisemboweler::GET)
		{
			*inout = constant;
		}
		else
		{
			throw EIexpProblem("Cannot set a constant!");
		}
	}
	void operator()(TVarExp const & var) const
	{
		boost::apply_visitor(LVL2IexpDisemboweler<T>(inout, env, dir), var);
	}
};

template<typename T>
T ERMInterpreter::getIexp( const ERM::TIexp & iexp, /*const*/ Trigger * trig /*= NULL*/, /*const*/ FunctionLocalVars * fun /*= NULL*/) const
{
	T ret;
	ERMExecEnvironment env(ermGlobalEnv, trig, fun);
	boost::apply_visitor(LVL1IexpDisemboweler<T>(&ret, &env, IexpDisemboweler::GET), iexp);
	return ret;
}

void ERMInterpreter::executeTriggerType( VERMInterpreter::TriggerType tt, bool pre, const std::map< int, std::vector<int> > & identifier )
{
	TtriggerListType & triggerList = pre ? triggers : postTriggers;

	TriggerIdentifierMatch tim;
	tim.allowNoIdetifier = false;
	tim.ermEnv = this;
	tim.matchToIt = identifier;
	std::vector<Trigger> triggersToTry = triggerList[tt];
	for(int g=0; g<triggersToTry.size(); ++g)
	{
		if(tim.tryMatch(&triggersToTry[g]))
		{
			executeTrigger(triggersToTry[g]);
		}
	}
}

ERM::Ttrigger ERMInterpreter::retrieveTrigger( ERM::TLine line )
{
	if(line.which() == 1)
	{
		ERM::TERMline tl = boost::get<ERM::TERMline>(line);
		if(tl.which() == 0)
		{
			ERM::Tcommand tcm = boost::get<ERM::Tcommand>(line);
			if(tcm.cmd.which() == 0)
			{
				return boost::get<ERM::Ttrigger>(tcm.cmd);
			}
		}
	}
	throw ELineProblem("Given line is not an ERM trigger!");
}

const std::string ERMInterpreter::triggerSymbol = "trigger";
const std::string ERMInterpreter::postTriggerSymbol = "postTrigger";
const std::string ERMInterpreter::defunSymbol = "defun";


struct TriggerIdMatchHelper : boost::static_visitor<>
{
	int & ret;
	ERMInterpreter * interpreter;
	Trigger * trig;

	TriggerIdMatchHelper(int & b, ERMInterpreter * ermint, Trigger * _trig)
	: ret(b), interpreter(ermint), trig(_trig)
	{}

	void operator()(TIexp const& iexp) const
	{
		ret = interpreter->getIexp<int>(iexp, trig);
	}
	void operator()(TArithmeticOp const& arop) const
	{
		//error?!?
	}
};

bool TriggerIdentifierMatch::tryMatch( Trigger * interptrig ) const
{
	const ERM::Ttrigger & trig = ERMInterpreter::retrieveTrigger(ermEnv->retrieveLine(interptrig->line));
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
				boost::apply_visitor(TriggerIdMatchHelper(val, ermEnv, interptrig), tid[g]);
				if(pattern[g] != val)
				{
					return false;
				}
			}
			return true;
		}
	}
	else
	{
		if(allowNoIdetifier)
			return true;
	}
}
