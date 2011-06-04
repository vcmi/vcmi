#define VCMI_DLL
#include "ERMInterpreter.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>

#include "CObjectHandler.h"
#include "CHeroHandler.h"
#include "CCreatureHandler.h"

/*
 * ERMInterpreter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#define DBG_PRINT(X) tlog0 << X << std::endl;

namespace spirit = boost::spirit;
using namespace VERMInterpreter;
using namespace boost::assign;
typedef int TUnusedType;
using namespace boost::assign;

ERMInterpreter *erm;

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
		void operator()(TMacroUsage const& cmp) const
		{
			tlog2 << "???$$" << cmp.macro << "$$";
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

				std::vector<LineInfo> buf = ep.parseFile();
				finfo->length = buf.size();
				files.push_back(finfo);

				for(int g=0; g<buf.size(); ++g)
				{
					scripts[LinePointer(finfo, g, buf[g].realLineNum)] = buf[g].tl;
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
	erm	 = this;
	curFunc = NULL;
	curTrigger = NULL;
	globalEnv = new Environment();
}

void ERMInterpreter::executeTrigger( VERMInterpreter::Trigger & trig, int funNum /*= -1*/, std::vector<int> funParams/*=std::vector<int>()*/ )
{
	//function-related logic
	if(funNum != -1)
	{
		curFunc = getFuncVars(funNum);
		for(int g=1; g<=FunctionLocalVars::NUM_PARAMETERS; ++g)
		{
			curFunc->getParam(g) = g-1 < funParams.size() ? funParams[g-1] : 0;
		}
	}
	else
		curFunc = getFuncVars(0);

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

	curFunc = NULL;
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
			TERMline ermline = boost::get<TERMline>(line);
			switch(ermline.which())
			{
			case 0: //tcmd
				return isCMDATrigger( boost::get<ERM::Tcommand>(ermline) );
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

ERM::TLine &ERMInterpreter::retrieveLine( LinePointer linePtr )
{
	return scripts.find(linePtr)->second;
}

/////////
//code execution

template<typename OwnerType>
struct StandardBodyOptionItemVisitor : boost::static_visitor<>
{
	typedef OwnerType TReceiverType;
	OwnerType & owner;
	explicit StandardBodyOptionItemVisitor(OwnerType & _owner) : owner(_owner)
	{}
	virtual void operator()(TVarConcatString const& cmp) const
	{
		throw EScriptExecError("String concatenation not allowed in this receiver");
	}
	virtual void operator()(TStringConstant const& cmp) const
	{
		throw EScriptExecError("String constant not allowed in this receiver");
	}
	virtual void operator()(TCurriedString const& cmp) const
	{
		throw EScriptExecError("Curried string not allowed in this receiver");
	}
	virtual void operator()(TSemiCompare const& cmp) const
	{
		throw EScriptExecError("Semi comparison not allowed in this receiver");
	}
// 	virtual void operator()(TMacroUsage const& cmp) const
// 	{
// 		throw EScriptExecError("Macro usage not allowed in this receiver");
// 	}
	virtual void operator()(TMacroDef const& cmp) const
	{
		throw EScriptExecError("Macro definition not allowed in this receiver");
	}
	virtual void operator()(TIexp const& cmp) const
	{
		throw EScriptExecError("i-expression not allowed in this receiver");
	}
	virtual void operator()(TVarpExp const& cmp) const
	{
		throw EScriptExecError("Varp expression not allowed in this receiver");
	}
	virtual void operator()(spirit::unused_type const& cmp) const
	{
		throw EScriptExecError("\'Nothing\' not allowed in this receiver");
	}
};

template<typename T>
struct StandardReceiverVisitor : boost::static_visitor<>
{
	ERMInterpreter * interp;
	T identifier;
	StandardReceiverVisitor(ERMInterpreter * _interpr, T ident) : interp(_interpr), identifier(ident)
	{}

	virtual void operator()(TVRLogic const& trig) const
	{
		throw EScriptExecError("VR logic not allowed in this receiver!");
	}
	virtual void operator()(TVRArithmetic const& trig) const
	{
		throw EScriptExecError("VR arithmetic not allowed in this receiver!");
	}
	virtual void operator()(TNormalBodyOption const& trig) const = 0;

	template<typename OptionPerformer>
	void performOptionTakingOneParamter(const ERM::TNormalBodyOptionList & params) const
	{
		if(params.size() == 1)
		{
			ERM::TBodyOptionItem boi = params[0];
			boost::apply_visitor(
				OptionPerformer(*const_cast<typename OptionPerformer::TReceiverType*>(static_cast<const typename OptionPerformer::TReceiverType*>(this))), boi);
		}
		else
			throw EScriptExecError("This receiver option takes exactly 1 parameter!");
	}

	template<template <int opcode> class OptionPerformer>
	void performOptionTakingOneParamterWithIntDispatcher(const ERM::TNormalBodyOptionList & params) const
	{
		if(params.size() == 2)
		{
			int optNum = erm->getIexp(params[0]).getInt();

			ERM::TBodyOptionItem boi = params[1];
			switch(optNum)
			{
			case 0:
				boost::apply_visitor(
					OptionPerformer<0>(*const_cast<typename OptionPerformer<0>::TReceiverType*>(static_cast<const typename OptionPerformer<0>::TReceiverType*>(this))), boi);
				break;
			default:
				throw EScriptExecError("Wrong number of option code!");
				break;
			}
		}
		else
			throw EScriptExecError("This receiver option takes exactly 2 parameters!");
	}
};
////HE
struct HEPerformer;

template<int opcode>
struct HE_BPerformer : StandardBodyOptionItemVisitor<HEPerformer>
{
	explicit HE_BPerformer(HEPerformer & _owner) : StandardBodyOptionItemVisitor<HEPerformer>(_owner)
	{}
	using StandardBodyOptionItemVisitor<HEPerformer>::operator();

	void operator()(TIexp const& cmp) const OVERRIDE;
	void operator()(TVarpExp const& cmp) const OVERRIDE;
};

template<int opcode>
void HE_BPerformer<opcode>::operator()( TIexp const& cmp ) const
{
	throw EScriptExecError("Setting hero name is not implemented!");
}

template<int opcode>
struct HE_CPerformer : StandardBodyOptionItemVisitor<HEPerformer>
{
	explicit HE_CPerformer(HEPerformer & _owner) : StandardBodyOptionItemVisitor<HEPerformer>(_owner)
	{}
	using StandardBodyOptionItemVisitor<HEPerformer>::operator();

	void operator()(TIexp const& cmp) const OVERRIDE;
	void operator()(TVarpExp const& cmp) const OVERRIDE;
};

template<int opcode>
void HE_CPerformer<opcode>::operator()( TIexp const& cmp ) const
{
	throw EScriptExecError("Setting hero army is not implemented!");
}

struct HEPerformer : StandardReceiverVisitor<const CGHeroInstance *>
{
	HEPerformer(ERMInterpreter * _interpr, const CGHeroInstance * hero) : StandardReceiverVisitor<const CGHeroInstance *>(_interpr, hero)
	{}
	using StandardReceiverVisitor<const CGHeroInstance *>::operator();

	void operator()(TNormalBodyOption const& trig) const OVERRIDE
	{
		switch(trig.optionCode)
		{
		case 'B':
			{
				performOptionTakingOneParamterWithIntDispatcher<HE_BPerformer>(trig.params);
			}
			break;
		case 'C':
			{
				const ERM::TNormalBodyOptionList & params = trig.params;
				if(params.size() == 4)
				{
					if(erm->getIexp(params[0]).getInt() == 0)
					{
						int slot = erm->getIexp(params[1]).getInt();
						if(params[2].which() == 6) //varp
						{
							erm->getIexp(boost::get<ERM::TVarpExp>(params[2])).setTo(identifier->getCreature(slot)->idNumber);
						}
						else
							throw EScriptExecError("Setting stack creature type is not implemented!");

						if(params[3].which() == 6) //varp
						{
							erm->getIexp(boost::get<ERM::TVarpExp>(params[3])).setTo(identifier->getStackCount(slot));
						}
						else
							throw EScriptExecError("Setting stack count is not implemented!");
					}
					else 
						throw EScriptExecError("Slot number must be an evaluable i-exp");
				}
				//todo else if(14 params)
				else
					throw EScriptExecError("Slot number must be an evaluable i-exp");
			}
			break;
		case 'E':
			break;
		case 'N':
			break;
		default:
			break;
		}
	}

};

template<int opcode>
void HE_BPerformer<opcode>::operator()( TVarpExp const& cmp ) const
{
	erm->getIexp(cmp).setTo(owner.identifier->name);
}

template<int opcode>
void HE_CPerformer<opcode>::operator()( TVarpExp const& cmp ) const
{
	erm->getIexp(cmp).setTo(owner.identifier->name);
}

////MA
struct MAPerformer;
struct MA_PPerformer : StandardBodyOptionItemVisitor<MAPerformer>
{
	explicit MA_PPerformer(MAPerformer & _owner);
	using StandardBodyOptionItemVisitor<MAPerformer>::operator();

	void operator()(TIexp const& cmp) const OVERRIDE;
	void operator()(TVarpExp const& cmp) const OVERRIDE;
};

struct MAPerformer : StandardReceiverVisitor<TUnusedType>
{
	MAPerformer(ERMInterpreter * _interpr) : StandardReceiverVisitor<TUnusedType>(_interpr, 0)
	{}
	using StandardReceiverVisitor<TUnusedType>::operator();

	void operator()(TNormalBodyOption const& trig) const OVERRIDE
	{
		switch(trig.optionCode)
		{
		case 'A': //sgc monster attack
			break;
		case 'B': //spell?
			break;
		case 'P': //hit points
			{
				//TODO
			}
			break;
		default:
			break;
		}
	}
		
};

void MA_PPerformer::operator()( TIexp const& cmp ) const
{

}

void MA_PPerformer::operator()( TVarpExp const& cmp ) const
{

}

////MO

struct MOPerformer;
struct MO_GPerformer : StandardBodyOptionItemVisitor<MOPerformer>
{
	explicit MO_GPerformer(MOPerformer & _owner) : StandardBodyOptionItemVisitor<MOPerformer>(_owner)
	{}
	using StandardBodyOptionItemVisitor<MOPerformer>::operator();

	void operator()(TVarpExp const& cmp) const OVERRIDE;
	void operator()(TIexp const& cmp) const OVERRIDE;
};

struct MOPerformer: StandardReceiverVisitor<int3>
{
	MOPerformer(ERMInterpreter * _interpr, int3 pos) : StandardReceiverVisitor<int3>(_interpr, pos)
	{}
	using StandardReceiverVisitor<int3>::operator();

	void operator()(TNormalBodyOption const& trig) const OVERRIDE
	{
		switch(trig.optionCode)
		{
		case 'G':
			{
				performOptionTakingOneParamter<MO_GPerformer>(trig.params);
			}
			break;
		default:
			break;
		}
	}
};

void MO_GPerformer::operator()( TIexp const& cmp ) const
{
	throw EScriptExecError("Setting monster count is not implemented yet!");
}

void MO_GPerformer::operator()( TVarpExp const& cmp ) const
{
	const CGCreature *cre = erm->getObjFromAs<CGCreature>(owner.identifier);
	erm->getIexp(cmp).setTo(cre->getStackCount(0));
}


struct ConditionDisemboweler;
//OB
struct OBPerformer;
struct OB_UPerformer : StandardBodyOptionItemVisitor<OBPerformer>
{
	explicit OB_UPerformer(OBPerformer & owner) : StandardBodyOptionItemVisitor<OBPerformer>(owner)
	{}
	using StandardBodyOptionItemVisitor<OBPerformer>::operator();

	virtual void operator()(TIexp const& cmp) const;
	virtual void operator()(TVarpExp const& cmp) const;
};

struct OBPerformer : StandardReceiverVisitor<int3>
{
	OBPerformer(ERMInterpreter * _interpr, int3 objPos) : StandardReceiverVisitor<int3>(_interpr, objPos)
	{}
	using StandardReceiverVisitor<int3>::operator(); //it removes compilation error... not sure why it *must* be here
	void operator()(TNormalBodyOption const& trig) const
	{
		switch(trig.optionCode)
		{
		case 'B': //removes description hint
			{
				//TODO
			}
			break;
		case 'C': //sgc of control word of object
			{
				//TODO
			}
			break;
		case 'D': //disable gamer to use object
			{
				//TODO
			}
			break;
		case 'E': //enable gamer to use object
			{
				//TODO
			}
			break;
		case 'H': //replace hint for object
			{
				//TODO
			}
			break;
		case 'M': //disabling messages and questions
			{
				//TODO
			}
			break;
		case 'R': //enable all gamers to use object
			{
				//TODO
			}
			break;
		case 'S': //disable all gamers to use object
			{
				//TODO
			}
			break;
		case 'T': //sgc of obj type
			{
				//TODO
			}
			break;
		case 'U': //sgc of obj subtype
			{
				performOptionTakingOneParamter<OB_UPerformer>(trig.params);
			}
			break;
		default:
			throw EScriptExecError("Wrong OB receiver option!");
			break;
		}
	}
};

void OB_UPerformer::operator()( TIexp const& cmp ) const
{
	IexpValStr val = owner.interp->getIexp(cmp);
	throw EScriptExecError("Setting subID is not implemented yet!");
}

void OB_UPerformer::operator()( TVarpExp const& cmp ) const
{
	IexpValStr val = owner.interp->getIexp(cmp);
	val.setTo(erm->getObjFrom(owner.identifier)->subID);
}

/////VR
struct VRPerformer;
struct VR_SPerformer : StandardBodyOptionItemVisitor<VRPerformer>
{
	explicit VR_SPerformer(VRPerformer & _owner);
	using StandardBodyOptionItemVisitor<VRPerformer>::operator();

	void operator()(TStringConstant const& cmp) const OVERRIDE;
	void operator()(TIexp const& cmp) const OVERRIDE;
};

struct VRPerformer : StandardReceiverVisitor<IexpValStr>
{
	VRPerformer(ERMInterpreter * _interpr, IexpValStr ident) : StandardReceiverVisitor<IexpValStr>(_interpr, ident)
	{}

	void operator()(TVRLogic const& trig) const OVERRIDE
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
	void operator()(TVRArithmetic const& trig) const OVERRIDE
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
	void operator()(TNormalBodyOption const& trig) const OVERRIDE
	{
		switch(trig.optionCode)
		{
		case 'C': //setting/checking v vars
			{
				//TODO
			}
			break;
		case 'H': //checking if string is empty
			{
				//TODO
			}
			break;
		case 'M': //string operations
			{
				//TODO
			}
			break;
		case 'R': //random variables
			{
				//TODO
			}
			break;
		case 'S': //setting variable
			{
				performOptionTakingOneParamter<VR_SPerformer>(trig.params);				
			}
			break;
		case 'T': //random variables
			{
				//TODO
			}
			break;
		case 'U': //search for a substring
			{
				//TODO
			}
			break;
		case 'V': //convert string to value
			{
				//TODO
			}
			break;
		default:
			throw EScriptExecError("Wrong VR receiver option!");
			break;
		}
	}
};


VR_SPerformer::VR_SPerformer(VRPerformer & _owner) : StandardBodyOptionItemVisitor<VRPerformer>(_owner)
{}

void VR_SPerformer::operator()(ERM::TIexp const& trig) const
{
	owner.identifier.setTo(owner.interp->getIexp(trig));
}
void VR_SPerformer::operator()(TStringConstant const& cmp) const
{
	owner.identifier.setTo(cmp.str);
}

/////

struct ERMExpDispatch : boost::static_visitor<>
{
	ERMInterpreter * owner;
	ERMExpDispatch(ERMInterpreter * _owner) : owner(_owner)
	{}

	struct HLP
	{
		ERMInterpreter * ei;
		HLP(ERMInterpreter * interp) : ei(interp)
		{}

		int3 getPosFromIdentifier(ERM::Tidentifier tid, bool allowDummyFourth)
		{
			switch(tid.size())
			{
			case 1:
				{
					int num = ei->getIexp(tid[0]).getInt();
					return int3(ei->ermGlobalEnv->getStandardVar(num),
						ei->ermGlobalEnv->getStandardVar(num+1),
						ei->ermGlobalEnv->getStandardVar(num+2));
				}
				break;
			case 3:
			case 4:
				if(tid.size() == 4 && !allowDummyFourth)
					throw EScriptExecError("4 items in identifier are not allowed for this receiver!");

				return int3(ei->getIexp(tid[0]).getInt(),
					ei->getIexp(tid[1]).getInt(),
					ei->getIexp(tid[2]).getInt());
				break;
			default:
				throw EScriptExecError("This receiver takes 1 or 3 items in identifier!");
				break;
			}
		}
		template <typename Visitor>
		void performBody(const boost::optional<ERM::Tbody> & body, const Visitor& visitor)
		{
			if(body.is_initialized())
			{
				ERM::Tbody bo = body.get();
				for(int g=0; g<bo.size(); ++g)
				{
					boost::apply_visitor(visitor, bo[g]);
				}
			}
		}
	};

	void operator()(Ttrigger const& trig) const
	{
		throw EInterpreterError("Triggers cannot be executed!");
	}
	void operator()(Tinstruction const& trig) const
	{
	}
	void operator()(Treceiver const& trig) const
	{
		HLP helper(owner);
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
					helper.performBody(trig.body, VRPerformer(owner, ievs));
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
					std::vector<int> params(FunctionLocalVars::NUM_PARAMETERS, 0);
					params.back() = it;
					//owner->getFuncVars(funNum)->getParam(16) = it;
					ERMInterpreter::TIDPattern tip;
					std::vector<int> v1;
					v1 += funNum;
					insert(tip) (v1.size(), v1);
					owner->executeTriggerType(TriggerType("FU"), true, tip, params);
					it = owner->getFuncVars(funNum)->getParam(16);
				}
			}
		}
		else if(trig.name == "MA")
		{
			if(trig.identifier.is_initialized())
			{
				throw EScriptExecError("MA receiver doesn't take the identifier!");
			}
			helper.performBody(trig.body, MAPerformer(owner));
		}
		else if(trig.name == "MO")
		{
			int3 objPos;
			if(trig.identifier.is_initialized())
			{
				ERM::Tidentifier tid = trig.identifier.get();
				objPos = HLP(owner).getPosFromIdentifier(tid, true);

				helper.performBody(trig.body, MOPerformer(owner, objPos));
			}
			else
				throw EScriptExecError("MO receiver must have an identifier!");
		}
		else if(trig.name == "OB")
		{
			int3 objPos;
			if(trig.identifier.is_initialized())
			{
				ERM::Tidentifier tid = trig.identifier.get();
				objPos = HLP(owner).getPosFromIdentifier(tid, false);

				helper.performBody(trig.body, OBPerformer(owner, objPos));
			}
			else
				throw EScriptExecError("OB receiver must have an identifier!");
		}
		else if(trig.name == "HE")
		{
			const CGHeroInstance * hero = NULL;
			if(trig.identifier.is_initialized())
			{
				ERM::Tidentifier tid = trig.identifier.get();
				switch(tid.size())
				{
				case 1:
					{
						int heroNum = erm->getIexp(tid[0]).getInt();
						if(heroNum == -1)
							hero = icb->getSelectedHero();
						else
							hero = icb->getHeroWithSubid(heroNum);

					}
					break;
				case 3:
					{
						int3 pos = helper.getPosFromIdentifier(tid, false);
						hero = erm->getObjFromAs<CGHeroInstance>(pos);
					}
					break;
				default:
					throw EScriptExecError("HE receiver takes 1 or 3 items in identifier");
					break;
				}
				helper.performBody(trig.body, HEPerformer(owner, hero));
			}
			else
				throw EScriptExecError("HE receiver must have an identifier!");
		}
		else
		{
			//not supported or invalid trigger
		}
	}
	void operator()(TPostTrigger const& trig) const
	{
		throw EInterpreterError("Post-triggers cannot be executed!");
	}
};

struct CommandExec : boost::static_visitor<>
{
	void operator()(Tcommand const& cmd) const
	{
		boost::apply_visitor(ERMExpDispatch(erm), cmd.cmd);
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
		VNode line(cmd);
		erm->eval(line);
	}
	void operator()(TERMline const& cmd) const
	{
		boost::apply_visitor(CommandExec(), cmd);
	}
};

/////////

void ERMInterpreter::executeLine( const LinePointer & lp )
{
	tlog0 << "Executing line nr " << getRealLine(lp.lineNum) << " (internal " << lp.lineNum << ") from " << lp.file->filename << std::endl;
	boost::apply_visitor(LineExec(), scripts[lp]);
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
		bool retIt = b == endNum/*+1*/; //if we should return the value are currently at

		char cr = toFollow[b];
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
					int &valPtr = curFunc ? curFunc->getLocal(initV) : const_cast<ERMInterpreter&>(*this).getFuncVars(0)->getLocal(initV); //retreive local var if in function or use global set otherwise
					if(retIt)
						ret = IexpValStr(&valPtr);
					else
						initV = curFunc->getLocal(initV);
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

	ret.name = toFollow;
	if(initVal.is_initialized())
	{
		ret.name += boost::lexical_cast<std::string>(initVal.get());
	}
	return ret;
}

namespace IexpDisemboweler
{
	enum EDir{GET, SET};
}

struct LVL2IexpDisemboweler : boost::static_visitor<IexpValStr>
{
	/*const*/ ERMInterpreter * env;
	IexpDisemboweler::EDir dir;

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
	/*const*/ ERMInterpreter * env;
	IexpDisemboweler::EDir dir;

	LVL1IexpDisemboweler(/*const*/ ERMInterpreter * _env, IexpDisemboweler::EDir _dir)
		: env(_env), dir(_dir) //writes value to given var
	{}
	IexpValStr operator()(int const & constant) const
	{
		if(dir == IexpDisemboweler::GET)
		{
			return IexpValStr(constant);
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
		return getIexp(boost::get<ERM::TIexp>(tid));
	}
	else
		throw EScriptExecError("Identifier must be a valid i-expression to perform this operation!");
}

IexpValStr ERMInterpreter::getIexp( const ERM::TVarpExp & tid ) const
{
	return boost::apply_visitor(LVL2IexpDisemboweler(const_cast<ERMInterpreter*>(this), IexpDisemboweler::GET), tid.var);
}

struct LVL3BodyOptionItemVisitor : StandardBodyOptionItemVisitor<IexpValStr>
{
	explicit LVL3BodyOptionItemVisitor(IexpValStr & _owner) : StandardBodyOptionItemVisitor<IexpValStr>(_owner)
	{}
	using StandardBodyOptionItemVisitor<IexpValStr>::operator();

	void operator()(TIexp const& cmp) const OVERRIDE
	{
		owner = erm->getIexp(cmp);
	}
	void operator()(TVarpExp const& cmp) const OVERRIDE
	{
		owner = erm->getIexp(cmp);
	}
};

IexpValStr ERMInterpreter::getIexp( const ERM::TBodyOptionItem & opit ) const
{
	IexpValStr ret;
	boost::apply_visitor(LVL3BodyOptionItemVisitor(ret), opit);
	return ret;
}

void ERMInterpreter::executeTriggerType( VERMInterpreter::TriggerType tt, bool pre, const TIDPattern & identifier, const std::vector<int> &funParams/*=std::vector<int>()*/ )
{
	struct HLP
	{
		static int calcFunNum(VERMInterpreter::TriggerType tt, const TIDPattern & identifier)
		{
			if(tt.type != VERMInterpreter::TriggerType::FU)
				return -1;

			return identifier.begin()->second[0];
		}
	};
	TtriggerListType & triggerList = pre ? triggers : postTriggers;

	TriggerIdentifierMatch tim;
	tim.allowNoIdetifier = true;
	tim.ermEnv = this;
	tim.matchToIt = identifier;
	std::vector<Trigger> & triggersToTry = triggerList[tt];
	for(int g=0; g<triggersToTry.size(); ++g)
	{
		if(tim.tryMatch(&triggersToTry[g]))
		{
			curTrigger = &triggersToTry[g];
			executeTrigger(triggersToTry[g], HLP::calcFunNum(tt, identifier), funParams);
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

ERM::TTriggerBase & ERMInterpreter::retrieveTrigger( ERM::TLine &line )
{
	if(line.which() == 1)
	{
		ERM::TERMline &tl = boost::get<ERM::TERMline>(line);
		if(tl.which() == 0)
		{
			ERM::Tcommand &tcm = boost::get<ERM::Tcommand>(tl);
			if(tcm.cmd.which() == 0)
			{
				return boost::get<ERM::Ttrigger>(tcm.cmd);
			}
			else if(tcm.cmd.which() == 3)
			{
				return boost::get<ERM::TPostTrigger>(tcm.cmd);
			}
			throw ELineProblem("Given line is not a trigger!");
		}
		throw ELineProblem("Given line is not a command!");
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
	if(funNum >= ARRAY_COUNT(funcVars) || funNum < 0)
		throw EScriptExecError("Attempt of accessing variables of function with index out of boundaries!");
	return funcVars + funNum;
}

void ERMInterpreter::executeInstructions()
{
	//TODO implement me
}

int ERMInterpreter::getRealLine(int lineNum)
{
	for(std::map<VERMInterpreter::LinePointer, ERM::TLine>::const_iterator i = scripts.begin(); i != scripts.end(); i++)
		if(i->first.lineNum == lineNum)
			return i->first.realLineNum;

	return -1;
}

void ERMInterpreter::setCurrentlyVisitedObj( int3 pos )
{
	ermGlobalEnv->getStandardVar(998) = pos.x;
	ermGlobalEnv->getStandardVar(999) = pos.y;
	ermGlobalEnv->getStandardVar(1000) = pos.z;
}

struct StringProcessHLP
{
	int extractNumber(const std::string & token)
	{
		return atoi(token.substr(1).c_str());
	}
	template <typename T>
	void replaceToken(std::string ret, int tokenBegin, int tokenEnd, T val)
	{
		ret.replace(tokenBegin, tokenEnd, boost::lexical_cast<std::string>(val));
	}
};

std::string ERMInterpreter::processERMString( std::string ermstring )
{
	StringProcessHLP hlp;
	std::string ret = ermstring;
	int curPos = 0;
	while((curPos = ret.find('%', curPos)) != std::string::npos)
	{
		curPos++;
		int tokenEnd = ret.find(' ', curPos);
		std::string token = ret.substr(curPos, tokenEnd - curPos);
		if(token.size() == 0)
		{
			throw EScriptExecError("Empty token not allowed!");
		}

		switch(token[0])
		{
		case '%':
			ret.erase(curPos);
			break;
		case 'F':
			hlp.replaceToken(ret, curPos, tokenEnd, ermGlobalEnv->getFlag(hlp.extractNumber(token)));
			break;
		case 'V':
			hlp.replaceToken(ret, curPos, tokenEnd, ermGlobalEnv->getStandardVar(hlp.extractNumber(token)));
			break;
		default:
			throw EScriptExecError("Unrecognized token in string");
			break;
		}

	}
	return ret;
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

	const ERM::TTriggerBase & trig = ERMInterpreter::retrieveTrigger(ermEnv->retrieveLine(interptrig->line));
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
	num = -num; //we handle negative indices
	if(num < 1 || num > YVAR_NUM)
		throw EScriptExecError("Number of trigger local variable out of bounds");

	return yvar[num-1];
}

bool VERMInterpreter::Environment::isBound( const std::string & name, bool globalOnly ) const
{
	std::map<std::string, VOption>::const_iterator it = symbols.find(name);
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

VOption & VERMInterpreter::Environment::retrieveValue( const std::string & name )
{
	std::map<std::string, VOption>::iterator it = symbols.find(name);
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

void VERMInterpreter::Environment::localBind( std::string name, const VOption & sym )
{
	symbols[name] = sym;
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

void VERMInterpreter::FunctionLocalVars::reset()
{
	for(int g=0; g<ARRAY_COUNT(params); ++g)
		params[g] = 0;
	for(int g=0; g<ARRAY_COUNT(locals); ++g)
		locals[g] = 0;
	for(int g=0; g<ARRAY_COUNT(strings); ++g)
		strings[g] = "";
	for(int g=0; g<ARRAY_COUNT(floats); ++g)
		floats[g] = 0.0;
}

void IexpValStr::setTo( const IexpValStr & second )
{
	DBG_PRINT("setting " << getName() << " to " << second.getName());
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
	DBG_PRINT("setting " << getName() << " to " << val);
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
	DBG_PRINT("setting " << getName() << " to " << val);
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
	DBG_PRINT("setting " << getName() << " to " << val);
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

std::string IexpValStr::getName() const
{
	if(name.size())
	{
		return name;
	}
	else if(type == IexpValStr::INT)
	{
		return "Literal " + boost::lexical_cast<std::string>(getInt());
	}
	else
		return "Unknown variable";
}

void ERMInterpreter::init()
{
	ermGlobalEnv = new ERMEnvironment();
	globalEnv = new Environment();
	//TODO: reset?
	for(int g = 0; g < ARRAY_COUNT(funcVars); ++g)
		funcVars[g].reset();

	scanForScripts();
	scanScripts();
	for(std::map<VERMInterpreter::LinePointer, ERM::TLine>::iterator it = scripts.begin();
		it != scripts.end(); ++it)
	{
		tlog0 << it->first.realLineNum << '\t';
		ERMPrinter::printAST(it->second);
	}

	executeInstructions();
	executeTriggerType("PI");
}

void ERMInterpreter::heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start)
{
	if(!visitedObj)
		return;
	setCurrentlyVisitedObj(visitedObj->pos);
	TIDPattern tip;
	tip[1] = list_of(visitedObj->ID);
	tip[2] = list_of(visitedObj->ID)(visitedObj->subID);
	tip[3] = list_of(visitedObj->pos.x)(visitedObj->pos.y)(visitedObj->pos.z);
	executeTriggerType(VERMInterpreter::TriggerType("OB"), start, tip);
}

void ERMInterpreter::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	executeTriggerType("BA", 0);
	executeTriggerType("BR", -1);
	executeTriggerType("BF", 0);
	//TODO tactics or not
}

const CGObjectInstance * ERMInterpreter::getObjFrom( int3 pos )
{
	std::vector<const CGObjectInstance * > objs = icb->getVisitableObjs(pos);
	if(!objs.size())
		throw EScriptExecError("Attempt to obtain access to nonexistent object!");
	return objs.back();
}

struct VOptionPrinter : boost::static_visitor<>
{
	void operator()(VNIL const& opt) const
	{
		tlog4 << "VNIL";
	}
	void operator()(VNode const& opt) const
	{
		tlog4 << "--vnode (will be supported in future versions)--";
	}
	void operator()(VSymbol const& opt) const
	{
		tlog4 << opt.text;
	}
	void operator()(TLiteral const& opt) const
	{
		tlog4 << opt;
	}
	void operator()(ERM::Tcommand const& opt) const
	{
		tlog4 << "--erm command (will be supported in future versions)--";
	}
};


struct VNodeEvaluator : boost::static_visitor<VOption>
{
	Environment & env;
	VNode & exp;
	VNodeEvaluator(Environment & _env, VNode & _exp) : env(_env), exp(_exp)
	{}
	VOption operator()(VNIL const& opt) const
	{
		throw EVermScriptExecError("Nil does not evaluate to a function");
	}
	VOption operator()(VNode const& opt) const
	{
		//otherwise...
		return VNIL();
	}
	VOption operator()(VSymbol const& opt) const
	{
		//check keywords
		if(opt.text == "quote")
		{
			if(exp.children.size() == 2)
				return exp.children[1];
			else
				throw EVermScriptExecError("quote special form takes only one argument");
		}
		else if(opt.text == "if")
		{
			if(exp.children.size() > 4)
				throw EVermScriptExecError("if statement takes no more than three arguments");

			if( isA<VNIL>(erm->eval(exp.children[1]) ) )
			{
				if(exp.children.size() > 2)
					return erm->eval(exp.children[2]);
				else
					throw EVermScriptExecError("this if form needs at least two arguments");
			}
			else
			{
				if(exp.children.size() > 3)
					return erm->eval(exp.children[3]);
				else
					throw EVermScriptExecError("this if form needs at least three arguments");
			}
		}
		else if(opt.text == "print")
		{
			if(exp.children.size() == 2)
			{
				VOption printed = erm->eval(exp.children[1]);
				boost::apply_visitor(VOptionPrinter(), printed);
				return printed;
			}
			else
				throw EVermScriptExecError("print special form takes only one argument");
		}
		else if(opt.text == "setq")
		{
			if(exp.children.size() != 3)
				throw EVermScriptExecError("setq special form takes exactly 3 arguments");

			env.localBind( getAs<std::string>(getAs<TLiteral>(exp.children[1]) ), exp.children[2]);
		}


		return VNIL(); //temporarily...
	}
	VOption operator()(TLiteral const& opt) const
	{
		throw EVermScriptExecError("String literal does not evaluate to a function");
	}
	VOption operator()(ERM::Tcommand const& opt) const
	{
		throw EVermScriptExecError("ERM command does not evaluate to a function");
	}
};

struct VEvaluator : boost::static_visitor<VOption>
{
	Environment & env;
	VEvaluator(Environment & _env) : env(_env)
	{}
	VOption operator()(VNIL const& opt) const
	{
		return opt;
	}
	VOption operator()(VNode const& opt) const
	{
		VOption & car = const_cast<VNode&>(opt).children.car().getAsItem();
		return boost::apply_visitor(VNodeEvaluator(env, const_cast<VNode&>(opt)), car);
	}
	VOption operator()(VSymbol const& opt) const
	{
		return env.retrieveValue(opt.text);
	}
	VOption operator()(TLiteral const& opt) const
	{
		return opt;
	}
	VOption operator()(ERM::Tcommand const& opt) const
	{
		return VNIL();
	}
};

VOption ERMInterpreter::eval( VOption line, Environment * env /*= NULL*/ )
{
// 	if(line.children.isNil())
// 		return;
// 
// 	VOption & car = line.children.car().getAsItem();
	return boost::apply_visitor(VEvaluator(env ? *env : *globalEnv), line);

}

VOptionList ERMInterpreter::evalEach( VermTreeIterator list, Environment * env /*= NULL*/ )
{
	VOptionList ret;
	for(int g=0; g<list.size(); ++g)
	{
		ret.push_back(eval(list.getIth(g), env));
	}
	return ret;
}

namespace VERMInterpreter
{
	VOption convertToVOption(const ERM::TVOption & tvo)
	{
		return boost::apply_visitor(OptionConverterVisitor(), tvo);
	}

	VNode::VNode( const ERM::TVExp & exp )
	{
		for(int i=0; i<exp.children.size(); ++i)
		{
			children.push_back(convertToVOption(exp.children[i]));
		}
		processModifierList(exp.modifier);
	}

	VNode::VNode( const VOption & first, const VOptionList & rest ) /*merges given arguments into [a, rest] */
	{
		setVnode(first, rest);
	}

	VNode::VNode( const VOptionList & cdren ) : children(cdren)
	{}

	VNode::VNode( const ERM::TSymbol & sym )
	{
		children.car() = OptionConverterVisitor()(sym);
		processModifierList(sym.symModifier);
	}

	void VNode::setVnode( const VOption & first, const VOptionList & rest )
	{
		children.car() = first;
		children.cdr() = rest;
	}

	void VNode::processModifierList( const std::vector<TVModifier> & modifierList )
	{
		for(int g=0; g<modifierList.size(); ++g)
		{
			children.cdr() = VNode(children);
			if(modifierList[g] == "`")
			{
				children.car() = VSymbol("backquote");
			}
			else if(modifierList[g] == ",!")
			{
				children.car() = VSymbol("comma-unlist");
			}
			else if(modifierList[g] == ",")
			{
				children.car() = VSymbol("comma");
			}
			else if(modifierList[g] == "#'")
			{
				children.car() = VSymbol("get-func");
			}
			else if(modifierList[g] == "'")
			{
				children.car() = VSymbol("quote");
			}
			else
				throw EInterpreterError("Incorrect value of modifier!");
		}
	}

	VermTreeIterator & VermTreeIterator::operator=( const VOption & opt )
	{
		switch (state)
		{
		case CAR:
			if(parent.size() <= basePos)
				parent.push_back(opt);
			else
				parent[basePos] = opt;
			break;
		case CDR:
			parent.resize(basePos+2);
			parent[basePos+1] = opt;
			break;
		default://should never happen
			break;
		}
		return *this;
	}

	VermTreeIterator & VermTreeIterator::operator=( const std::vector<VOption> & opt )
	{
		switch (state)
		{
		case CAR:
			//TODO: implement me
			break;
		case CDR:
			parent.resize(basePos+1);
			parent.insert(parent.begin()+basePos+1, opt.begin(), opt.end());
			break;
		default://should never happen
			break;
		}
		return *this;
	}
	VermTreeIterator & VermTreeIterator::operator=( const VOptionList & opt )
	{
		return *this = opt;
	}
	VOption & VermTreeIterator::getAsItem()
	{
		if(state == CAR)
			return parent[basePos];
		else
			throw EInterpreterError("iterator is not in car state, cannot get as list");
	}
	VermTreeIterator VermTreeIterator::getAsList()
	{
		VermTreeIterator ret = *this;
		ret.basePos++;
		return ret;
	}
	VOption & VermTreeIterator::getIth( int i )
	{
		return parent[basePos + i];
	}
	size_t VermTreeIterator::size() const
	{
		return parent.size() - basePos;
	}
	VOption OptionConverterVisitor::operator()( ERM::TVExp const& cmd ) const
	{
		return VNode(cmd);
	}
	VOption OptionConverterVisitor::operator()( ERM::TSymbol const& cmd ) const
	{
		if(cmd.symModifier.size() == 0)
			return VSymbol(cmd.sym);
		else
			return VNode(cmd);
	}
	VOption OptionConverterVisitor::operator()( char const& cmd ) const
	{
		return TLiteral(cmd);
	}
	VOption OptionConverterVisitor::operator()( double const& cmd ) const
	{
		return TLiteral(cmd);
	}
	VOption OptionConverterVisitor::operator()(int const& cmd) const
	{
		return TLiteral(cmd);
	}
	VOption OptionConverterVisitor::operator()(ERM::Tcommand const& cmd) const
	{
		return cmd;
	}
	VOption OptionConverterVisitor::operator()( ERM::TStringConstant const& cmd ) const
	{
		return TLiteral(cmd.str);
	}

	bool VOptionList::isNil() const
	{
		return size() == 0;
	}

	VermTreeIterator VOptionList::cdr()
	{
		VermTreeIterator ret(*this);
		ret.state = VermTreeIterator::CDR;
		return ret;
	}

	VermTreeIterator VOptionList::car()
	{
		VermTreeIterator ret(*this);
		ret.state = VermTreeIterator::CAR;
		return ret;
	}

}