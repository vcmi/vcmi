#define VCMI_DLL
#include "ERMInterpreter.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

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
using namespace boost::assign;

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
	curFunc = NULL;
	curTrigger = NULL;
	globalEnv = new Environment();
}

void ERMInterpreter::executeTrigger( Trigger & trig )
{
	//skip the first line
	LinePointer lp = trig.line;
	++lp;
	for(; lp.isValid(); ++lp)
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

struct VRPerformer;
struct VR_SPerformer : boost::static_visitor<>
{
	VRPerformer & owner;
	explicit VR_SPerformer(VRPerformer & _owner);

	void operator()(TVarConcatString const& cmp) const;
	void operator()(TStringConstant const& cmp) const;
	void operator()(TCurriedString const& cmp) const;
	void operator()(TSemiCompare const& cmp) const;
	void operator()(TMacroUsage const& cmp) const;
	void operator()(TMacroDef const& cmp) const;
	void operator()(TIexp const& cmp) const;
	void operator()(TVarpExp const& cmp) const;
	void operator()(spirit::unused_type const& cmp) const;
};


struct VRPerformer : boost::static_visitor<>
{
	ERMInterpreter * interp;
	IexpValStr identifier;
	VRPerformer(ERMInterpreter * _interpr, IexpValStr ident) : interp(_interpr), identifier(ident)
	{}

	void operator()(TVRLogic const& trig) const
	{
		int valr = interp->getIexp(trig.var).getInt();
		switch (trig.opcode)
		{
		case '&':
			const_cast<VRPerformer*>(this)->identifier.setTo(identifier.getInt() & valr);
			break;
		case '|':
			const_cast<VRPerformer*>(this)->identifier.setTo(identifier.getInt() | valr);
			break;
		case 'X':
			const_cast<VRPerformer*>(this)->identifier.setTo(identifier.getInt() ^ valr);
			break;
		default:
			throw EInterpreterError("Wrong opcode in VR logic expression!");
			break;
		}
	}
	void operator()(TVRArithmetic const& trig) const
	{
		IexpValStr rhs = interp->getIexp(trig.rhs);
		switch (trig.opcode)
		{
		case '+':
			const_cast<VRPerformer*>(this)->identifier += rhs;
			break;
		case '-':
			const_cast<VRPerformer*>(this)->identifier -= rhs;
			break;
		case '*':
			const_cast<VRPerformer*>(this)->identifier *= rhs;
			break;
		case ':':
			const_cast<VRPerformer*>(this)->identifier /= rhs;
			break;
		case '%':
			const_cast<VRPerformer*>(this)->identifier %= rhs;
			break;
		default:
			throw EInterpreterError("Wrong opcode in VR arithmetic!");
			break;
		}
	}
	void operator()(TNormalBodyOption const& trig) const
	{
		switch(trig.optionCode)
		{
		case 'S': //setting variable
			{
				if(trig.params.size() == 1)
				{
					ERM::TBodyOptionItem boi = trig.params[0];
					boost::apply_visitor(VR_SPerformer(*const_cast<VRPerformer*>(this)), boi);
				}
				else
					throw EScriptExecError("VR receiver S option takes exactly 1 parameter!");
			}
			break;
		default:
			//not supported or not allowed
			break;
		}
	}
};


VR_SPerformer::VR_SPerformer(VRPerformer & _owner) : owner(_owner)
{}

void VR_SPerformer::operator()(ERM::TIexp const& trig) const
{
	owner.identifier.setTo(owner.interp->getIexp(trig));
}

void VR_SPerformer::operator()(TVarConcatString const& cmp) const
{
	throw EScriptExecError("String concatenation not allowed in VR S");
}
void VR_SPerformer::operator()(TStringConstant const& cmp) const
{
	owner.identifier.setTo(cmp.str);
}
void VR_SPerformer::operator()(TCurriedString const& cmp) const
{
	throw EScriptExecError("Curried string not allowed in VR S");
}
void VR_SPerformer::operator()(TSemiCompare const& cmp) const
{
	throw EScriptExecError("Incomplete comparison not allowed in VR S");
}
void VR_SPerformer::operator()(TMacroUsage const& cmp) const
{
	owner.identifier.setTo(owner.interp->getIexp(cmp));
}
void VR_SPerformer::operator()(TMacroDef const& cmp) const
{
	throw EScriptExecError("Macro definition not allowed in VR S");
}
void VR_SPerformer::operator()(TVarpExp const& cmp) const
{
	throw EScriptExecError("Write-only variable expression not allowed in VR S");
}
void VR_SPerformer::operator()(spirit::unused_type const& cmp) const
{
	throw EScriptExecError("Expression not allowed in VR S");
}

struct ConditionDisemboweler;


struct ERMExpDispatch : boost::static_visitor<>
{
	ERMInterpreter * owner;
	ERMExpDispatch(ERMInterpreter * _owner) : owner(_owner)
	{}

	void operator()(Ttrigger const& trig) const
	{
		throw EInterpreterError("Triggers cannot be executed!");
	}
	void operator()(Tinstruction const& trig) const
	{
	}
	void operator()(Treceiver const& trig) const
	{
		if(trig.name == "VR")
		{
			//check condition
			if(trig.condition.is_initialized())
			{
				if( !owner->checkCondition(trig.condition.get()) )
					return;
			}

			//perform operations
			if(trig.identifier.is_initialized())
			{
				ERM::Tidentifier ident = trig.identifier.get();
				if(ident.size() == 1)
				{
					IexpValStr ievs = owner->getIexp(ident[0]);

					//see body
					if(trig.body.is_initialized())
					{
						ERM::Tbody bo = trig.body.get();
						for(int g=0; g<bo.size(); ++g)
						{
							boost::apply_visitor(VRPerformer(owner, ievs), bo[g]);
						}
					}
				}
				else
					throw EScriptExecError("VR receiver must be used with exactly one identifier item!");
			}
			else
				throw EScriptExecError("VR receiver must be used with an identifier!");
		}
		else if(trig.name == "DO")
		{
			//perform operations
			if(trig.identifier.is_initialized())
			{
				ERM::Tidentifier tid = trig.identifier.get();
				if(tid.size() != 4)
				{
					throw EScriptExecError("DO receiver takes exactly 4 arguments");
				}
				int funNum = owner->getIexp(tid[0]).getInt(),
					startVal = owner->getIexp(tid[1]).getInt(),
					stopVal = owner->getIexp(tid[2]).getInt(),
					increment = owner->getIexp(tid[3]).getInt();

				for(int it = startVal; it < stopVal; it += increment)
				{
					owner->getFuncVars(funNum)->getParam(16) = it;
					ERMInterpreter::TIDPattern tip;
					std::vector<int> v1;
					v1 += funNum;
					insert(tip) (v1.size(), v1);
					owner->executeTriggerType(TriggerType("FU"), true, tip);
					it = owner->getFuncVars(funNum)->getParam(16);
				}
			}
		}
		else
		{
			//unsupported or invalid trigger
		}
	}
	void operator()(TPostTrigger const& trig) const
	{
		throw EInterpreterError("Post-triggers cannot be executed!");
	}
};

struct CommandExec : boost::static_visitor<>
{
	ERMInterpreter * owner;
	CommandExec(ERMInterpreter * _owner) : owner(_owner)
	{}

	void operator()(Tcommand const& cmd) const
	{
		boost::apply_visitor(ERMExpDispatch(owner), cmd.cmd);
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
	ERMInterpreter * owner;
	LineExec(ERMInterpreter * _owner) : owner(_owner)
	{}

	void operator()(TVExp const& cmd) const
	{
		//printTVExp(cmd);
	}
	void operator()(TERMline const& cmd) const
	{
		boost::apply_visitor(CommandExec(owner), cmd);
	}
};

/////////

void ERMInterpreter::executeLine( const LinePointer & lp )
{
	boost::apply_visitor(LineExec(this), scripts[lp]);
}

void ERMInterpreter::init()
{
	ermGlobalEnv = new ERMEnvironment();
	globalEnv = new Environment();
	//TODO: reset?
}

IexpValStr ERMInterpreter::getVar(std::string toFollow, boost::optional<int> initVal) const
{
	IexpValStr ret;
	ret.type = IexpValStr::WRONGVAL;

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
	if(toFollow.size() == 0)
	{
		if(hasInit)
			ret = IexpValStr(initV);
		else
			throw EIexpProblem("No input to getVar!");

		return ret;
	}
	//now we have at least one element in toFollow
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
						if(curFunc)
							ret = IexpValStr(&curFunc->getFloat(initV));
						else
							throw EIexpProblem("Function context not available!");
					}
					else if(initV < 0 && initV >= -TriggerLocalVars::EVAR_NUM)
					{
						if(curTrigger)
							ret = IexpValStr(&curTrigger->ermLocalVars.getEvar(initV));
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
				ret = IexpValStr(&ermGlobalEnv->getQuickVar(cr));
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
				if(retIt)
					ret = IexpValStr(&ermGlobalEnv->getStandardVar(initV));
				else
					initV = ermGlobalEnv->getStandardVar(initV);
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
				if(curFunc)
				{
					if(retIt)
						ret = IexpValStr(&curFunc->getParam(initV));
					else
						initV = curFunc->getParam(initV);
				}
				else throw EIexpProblem("Function parameters cannot be used outside a function!");
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
					if(curFunc)
					{
						if(retIt)
							ret = IexpValStr(&curFunc->getLocal(initV));
						else
							initV = curFunc->getLocal(initV);
					}
					else
						throw EIexpProblem("Function local variables cannot be used outside a function!");
				}
				else if(initV < 0 && initV >= -TriggerLocalVars::YVAR_NUM)
				{
					if(curTrigger)
					{
						if(retIt)
							ret = IexpValStr(&curTrigger->ermLocalVars.getYvar(initV));
						else
							initV = curTrigger->ermLocalVars.getYvar(initV);
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
					if(initV > 0 )
						ret = IexpValStr(&ermGlobalEnv->getZVar(initV));
					else if(initV < 0)
					{
						if(curFunc)
							ret = IexpValStr(&curFunc->getString(initV));
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

namespace IexpDisemboweler
{
	enum EDir{GET, SET};
}

struct LVL2IexpDisemboweler : boost::static_visitor<IexpValStr>
{
	IexpDisemboweler::EDir dir;
	/*const*/ ERMInterpreter * env;

	LVL2IexpDisemboweler(/*const*/ ERMInterpreter * _env, IexpDisemboweler::EDir _dir)
		: env(_env), dir(_dir) //writes value to given var
	{}

	IexpValStr processNotMacro(const TVarExpNotMacro & val) const
	{
		if(val.questionMark.is_initialized())
			throw EIexpProblem("Question marks ('?') are not allowed in getter i-expressions");

		//const-cast just to do some code-reuse...
		return env->getVar(val.varsym, val.val);

	}

	IexpValStr operator()(TVarExpNotMacro const& val) const
	{
		return processNotMacro(val);
	}
	IexpValStr operator()(TMacroUsage const& val) const
	{
		return env->getIexp(val);
	}
};

struct LVL1IexpDisemboweler : boost::static_visitor<IexpValStr>
{
	IexpDisemboweler::EDir dir;
	/*const*/ ERMInterpreter * env;

	LVL1IexpDisemboweler(/*const*/ ERMInterpreter * _env, IexpDisemboweler::EDir _dir)
		: env(_env), dir(_dir) //writes value to given var
	{}
	IexpValStr operator()(int const & constant) const
	{
		if(dir == IexpDisemboweler::GET)
		{
			IexpValStr ret = IexpValStr(constant);
		}
		else
		{
			throw EIexpProblem("Cannot set a constant!");
		}
	}
	IexpValStr operator()(TVarExp const & var) const
	{
		return boost::apply_visitor(LVL2IexpDisemboweler(env, dir), var);
	}
};

IexpValStr ERMInterpreter::getIexp( const ERM::TIexp & iexp ) const
{
	return boost::apply_visitor(LVL1IexpDisemboweler(const_cast<ERMInterpreter*>(this), IexpDisemboweler::GET), iexp);
}

IexpValStr ERMInterpreter::getIexp( const ERM::TMacroUsage & macro ) const
{
	std::map<std::string, ERM::TVarExpNotMacro>::const_iterator it =
		ermGlobalEnv->macroBindings.find(macro.macro);
	if(it == ermGlobalEnv->macroBindings.end())
		throw EUsageOfUndefinedMacro(macro.macro);

	return getVar(it->second.varsym, it->second.val);
}

IexpValStr ERMInterpreter::getIexp( const ERM::TIdentifierInternal & tid ) const
{
	if(tid.which() == 0)
	{
		IexpValStr ievs = getIexp(boost::get<ERM::TIexp>(tid));
	}
	else
		throw EScriptExecError("Identifier must be a valid i-expression to perform this operation!");
}

void ERMInterpreter::executeTriggerType( VERMInterpreter::TriggerType tt, bool pre, const TIDPattern & identifier )
{
	TtriggerListType & triggerList = pre ? triggers : postTriggers;

	TriggerIdentifierMatch tim;
	tim.allowNoIdetifier = false;
	tim.ermEnv = this;
	tim.matchToIt = identifier;
	std::vector<Trigger> & triggersToTry = triggerList[tt];
	for(int g=0; g<triggersToTry.size(); ++g)
	{
		if(tim.tryMatch(&triggersToTry[g]))
		{
			curTrigger = &triggersToTry[g];
			executeTrigger(triggersToTry[g]);
		}
	}
}

void ERMInterpreter::executeTriggerType(const char *trigger, int id)
{
	TIDPattern tip;
	tip[0] = std::vector<int>(1, id);
	executeTriggerType(VERMInterpreter::TriggerType(trigger), true, tip);
}

void ERMInterpreter::executeTriggerType(const char *trigger)
{
	executeTriggerType(VERMInterpreter::TriggerType(trigger), true, TIDPattern());
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

template<typename T>
bool compareExp(const T & lhs, const T & rhs, std::string op)
{
	if(op == "<")
	{
		return lhs < rhs;
	}
	else if(op == ">")
	{
		return lhs > rhs;
	}
	else if(op == ">=" || op == "=>")
	{
		return lhs >= rhs;
	}
	else if(op == "<=" || op == "=<")
	{
		return lhs <= rhs;
	}
	else if(op == "==")
	{
		return lhs == rhs;
	}
	else if(op == "<>" || op == "><")
	{
		return lhs != rhs;
	}
	else
		throw EScriptExecError(std::string("Wrong comparison sign: ") + op);
}

struct ConditionDisemboweler : boost::static_visitor<bool>
{
	ConditionDisemboweler(ERMInterpreter * _ei) : ei(_ei)
	{}

	bool operator()(TComparison const & cmp) const
	{
		IexpValStr lhs = ei->getIexp(cmp.lhs),
			rhs = ei->getIexp(cmp.rhs);
		switch (lhs.type)
		{
		case IexpValStr::FLOATVAR:
			switch (rhs.type)
			{
			case IexpValStr::FLOATVAR:
				return compareExp(lhs.getFloat(), rhs.getFloat(), cmp.compSign);
				break;
			default:
				throw EScriptExecError("Incompatible types for comparison");
			}
			break;
		case IexpValStr::INT:
		case IexpValStr::INTVAR:
			switch (rhs.type)
			{
			case IexpValStr::INT:
			case IexpValStr::INTVAR:
				return compareExp(lhs.getInt(), rhs.getInt(), cmp.compSign);
				break;
			default:
				throw EScriptExecError("Incompatible types for comparison");
			}
			break;
		case IexpValStr::STRINGVAR:
			switch (rhs.type)
			{
			case IexpValStr::STRINGVAR:
				return compareExp(lhs.getString(), rhs.getString(), cmp.compSign);
				break;
			default:
				throw EScriptExecError("Incompatible types for comparison");
			}
			break;
		default:
			throw EScriptExecError("Wrong type of left iexp!");
		}
		//we should never reach this place
	}
	bool operator()(int const & flag) const
	{
		return ei->ermGlobalEnv->getFlag(flag);
	}
private:
	ERMInterpreter * ei;
};

bool ERMInterpreter::checkCondition( ERM::Tcondition cond )
{
	bool ret = boost::apply_visitor(ConditionDisemboweler(this), cond.cond);
	if(cond.rhs.is_initialized())
	{ //taking care of rhs expression
		bool rhs = checkCondition(cond.rhs.get().get());
		switch (cond.ctype)
		{
		case '&':
			ret &= rhs;
			break;
		case '|':
			ret |= rhs;
			break;
		case 'X':
			ret ^= rhs;
			break;
		default:
			throw EInterpreterProblem(std::string("Strange - wrong condition connection (") + cond.ctype + ") !");
			break;
		}
	}

	return ret;
}

FunctionLocalVars * ERMInterpreter::getFuncVars( int funNum )
{
	return funcVars + funNum - 1;
}

void ERMInterpreter::executeInstructions()
{
	//TODO implement me
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
		IexpValStr val = interpreter->getIexp(iexp);
		switch(val.type)
		{
		case IexpValStr::INT:
		case IexpValStr::INTVAR:
			ret = val.getInt();
			break;
		default:
			throw EScriptExecError("Incompatible i-exp type!");
			break;
		}
	}
	void operator()(TArithmeticOp const& arop) const
	{
		//error?!?
	}
};

bool TriggerIdentifierMatch::tryMatch( Trigger * interptrig ) const
{
	bool ret = true;

	const ERM::Ttrigger & trig = ERMInterpreter::retrieveTrigger(ermEnv->retrieveLine(interptrig->line));
	if(trig.identifier.is_initialized())
	{

		ERM::Tidentifier tid = trig.identifier.get();
		std::map< int, std::vector<int> >::const_iterator it = matchToIt.find(tid.size());
		if(it == matchToIt.end())
			ret = false;
		else
		{
			const std::vector<int> & pattern = it->second;
			for(int g=0; g<pattern.size(); ++g)
			{
				int val = -1;
				boost::apply_visitor(TriggerIdMatchHelper(val, ermEnv, interptrig), tid[g]);
				if(pattern[g] != val)
				{
					ret = false;
				}
			}
			ret = true;
		}
	}
	else
	{
		ret = allowNoIdetifier;
	}

	//check condition
	if(ret)
	{
		if(trig.condition.is_initialized())
		{
			return ermEnv->checkCondition(trig.condition.get());
		}
		else //no condition
			return true;
	}
	else
		return false;
}

VERMInterpreter::ERMEnvironment::ERMEnvironment()
{
	for(int g=0; g<NUM_QUICKS; ++g)
		quickVars[g] = 0;
	for(int g=0; g<NUM_STANDARDS; ++g)
		standardVars[g] = 0;
	//string should be automatically initialized to ""
	for(int g=0; g<NUM_FLAGS; ++g)
		flags[g] = false;
}

int & VERMInterpreter::ERMEnvironment::getQuickVar( const char letter )
{
	assert(letter >= 'f' && letter <= 't'); //it should be check by another function, just making sure here
	return quickVars[letter - 'f'];
}

int & VERMInterpreter::ERMEnvironment::getStandardVar( int num )
{
	if(num < 1 || num > NUM_STANDARDS)
		throw EScriptExecError("Number of standard variable out of bounds");

	return standardVars[num-1];
}

std::string & VERMInterpreter::ERMEnvironment::getZVar( int num )
{
	if(num < 1 || num > NUM_STRINGS)
		throw EScriptExecError("Number of string variable out of bounds");

	return strings[num-1];
}

bool & VERMInterpreter::ERMEnvironment::getFlag( int num )
{
	if(num < 1 || num > NUM_FLAGS)
		throw EScriptExecError("Number of flag out of bounds");

	return flags[num-1];
}

VERMInterpreter::TriggerLocalVars::TriggerLocalVars()
{
	for(int g=0; g<EVAR_NUM; ++g)
		evar[g] = 0.0;
	for(int g=0; g<YVAR_NUM; ++g)
		yvar[g] = 0;
}

double & VERMInterpreter::TriggerLocalVars::getEvar( int num )
{
	num = -num;
	if(num < 1 || num > EVAR_NUM)
		throw EScriptExecError("Number of trigger local floating point variable out of bounds");

	return evar[num-1];
}

int & VERMInterpreter::TriggerLocalVars::getYvar( int num )
{
	if(num < 1 || num > YVAR_NUM)
		throw EScriptExecError("Number of trigger local variable out of bounds");

	return yvar[num-1];
}

bool VERMInterpreter::Environment::isBound( const std::string & name, bool globalOnly ) const
{
	std::map<std::string, TVOption>::const_iterator it = symbols.find(name);
	if(globalOnly && parent)
	{
		return parent->isBound(name, globalOnly);
	}

	//we have it; if globalOnly is true, lexical parent is false here so we are global env
	if(it != symbols.end())
		return true;

	//here, we don;t have it; but parent can have
	if(parent)
		return parent->isBound(name, globalOnly);

	return false;
}

ERM::TVOption VERMInterpreter::Environment::retrieveValue( const std::string & name ) const
{
	std::map<std::string, TVOption>::const_iterator it = symbols.find(name);
	if(it == symbols.end())
	{
		if(parent)
		{
			return parent->retrieveValue(name);
		}

		throw ESymbolNotFound(name);
	}
	return it->second;
}

bool VERMInterpreter::Environment::unbind( const std::string & name, EUnbindMode mode )
{
	if(isBound(name, false))
	{
		if(symbols.find(name) != symbols.end()) //result of isBound could be from higher lexical env
			symbols.erase(symbols.find(name));

		if(mode == FULLY_RECURSIVE && parent)
			parent->unbind(name, mode);

		return true;
	}
	if(parent && (mode == RECURSIVE_UNTIL_HIT || mode == FULLY_RECURSIVE))
		return parent->unbind(name, mode);

	//neither bound nor have lexical parent
	return false;
}

int & VERMInterpreter::FunctionLocalVars::getParam( int num )
{
	if(num < 1 || num > NUM_PARAMETERS)
		throw EScriptExecError("Number of parameter out of bounds");

	return params[num-1];
}

int & VERMInterpreter::FunctionLocalVars::getLocal( int num )
{
	if(num < 1 || num > NUM_LOCALS)
		throw EScriptExecError("Number of local variable out of bounds");

	return locals[num-1];
}

std::string & VERMInterpreter::FunctionLocalVars::getString( int num )
{
	num = -num; //we deal with negative indices
	if(num < 1 || num > NUM_PARAMETERS)
		throw EScriptExecError("Number of function local string variable out of bounds");

	return strings[num-1];
}

double & VERMInterpreter::FunctionLocalVars::getFloat( int num )
{
	if(num < 1 || num > NUM_FLOATINGS)
		throw EScriptExecError("Number of float var out of bounds");

	return floats[num-1];
}

void IexpValStr::setTo( const IexpValStr & second )
{
	switch(type)
	{
	case IexpValStr::FLOATVAR:
		*val.flvar = second.getFloat();
		break;
	case IexpValStr::INT:
		throw EScriptExecError("VR S: value not assignable!");
		break;
	case IexpValStr::INTVAR:
		*val.integervar = second.getInt();
		break;
	case IexpValStr::STRINGVAR:
		*val.stringvar = second.getString();
		break;
	default:
		throw EScriptExecError("Wrong type of identifier iexp!");
	}
}

void IexpValStr::setTo( int val )
{
	switch(type)
	{
	case INTVAR:
		*this->val.integervar = val;
		break;
	default:
		throw EIexpProblem("Incompatible type!");
		break;
	}
}

void IexpValStr::setTo( float val )
{
	switch(type)
	{
	case FLOATVAR:
		*this->val.flvar = val;
		break;
	default:
		throw EIexpProblem("Incompatible type!");
		break;
	}
}

void IexpValStr::setTo( const std::string & val )
{
	switch(type)
	{
	case STRINGVAR:
		*this->val.stringvar = val;
		break;
	default:
		throw EIexpProblem("Incompatible type!");
		break;
	}
}

int IexpValStr::getInt() const
{
	switch(type)
	{
	case IexpValStr::INT:
		return val.val;
		break;
	case IexpValStr::INTVAR:
		return *val.integervar;
		break;
	default:
		throw EIexpProblem("Cannot get iexp as int!");
		break;
	}
}

float IexpValStr::getFloat() const
{
	switch(type)
	{
	case IexpValStr::FLOATVAR:
		return *val.flvar;
		break;
	default:
		throw EIexpProblem("Cannot get iexp as float!");
		break;
	}
}

std::string IexpValStr::getString() const
{
	switch(type)
	{
	case IexpValStr::STRINGVAR:
		return *val.stringvar;
		break;
	default:
		throw EScriptExecError("Cannot get iexp as string!");
		break;
	}
}
