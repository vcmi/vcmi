#pragma once
#include "../global.h"
#include "ERMParser.h"

/*
 * ERMInterpreter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

namespace VERMInterpreter
{
	using namespace ERM;

	//different exceptions that can be thrown during interpreting
	class EInterpreterProblem : public std::exception
	{
		std::string problem;
	public:
		const char * what() const throw() OVERRIDE
		{
			return problem.c_str();
		}
		~EInterpreterProblem() throw()
		{}
		EInterpreterProblem(const std::string & problemDesc) : problem(problemDesc)
		{}
	};

	struct ESymbolNotFound : public EInterpreterProblem
	{
		ESymbolNotFound(const std::string & sym) :
		  EInterpreterProblem(std::string("Symbol \"") + sym + std::string("\" not found!"))
		{}
	};

	struct EInvalidTrigger : public EInterpreterProblem
	{
		EInvalidTrigger(const std::string & sym) :
		  EInterpreterProblem(std::string("Trigger \"") + sym + std::string("\" is invalid!"))
		{}
	};

	struct EUsageOfUndefinedMacro : public EInterpreterProblem
	{
		EUsageOfUndefinedMacro(const std::string & macro) :
			EInterpreterProblem(std::string("Macro ") + macro + " is undefined")
		{}
	};

	struct EIexpProblem : public EInterpreterProblem
	{
		EIexpProblem(const std::string & desc) :
			EInterpreterProblem(desc)
		{}
	};

	struct ELineProblem : public EInterpreterProblem
	{
		ELineProblem(const std::string & desc) :
			EInterpreterProblem(desc)
		{}
	};


	///main environment class, manages symbols
	class Environment
	{
	private:
		std::map<std::string, TVOption> symbols;
		Environment * parent;

	public:
		bool isBound(const std::string & name, bool globalOnly) const
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

		TVOption retrieveValue(const std::string & name) const
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

		enum EUnbindMode{LOCAL, RECURSIVE_UNTIL_HIT, FULLY_RECURSIVE};
		///returns true if symbols was really unbound
		bool unbind(const std::string & name, EUnbindMode mode)
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
		TriggerLocalVars()
		{
			for(int g=0; g<EVAR_NUM; ++g)
				evar[g] = 0.0;
			for(int g=0; g<YVAR_NUM; ++g)
				yvar[g] = 0;
		}
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
		int & getQuickVar(const char letter)
		{
			assert(letter >= 'f' && letter <= 't'); //it should be check by another function, just makign sure here
			return quickVars[letter - 'f'];
		}

		static const int NUM_STANDARDS = 1000;
		int standardVars[NUM_STANDARDS]; //v-vars

		static const int NUM_STRINGS = 1000;
		std::string strings[NUM_STRINGS]; //z-vars (positive indices)

		std::map<std::string, ERM::TVarExpNotMacro> macroBindings;
	};

	struct TriggerType
	{
		//the same order of trigger types in this enum and in validTriggers array is obligatory!
		enum ETrigType{AE, BA, BF, BG, BR, CM, CO, FU, GE, GM, HE, HL, HM, IP, LE, MF, MG, MM, MR,
			MW, OB, PI, SN, TH, TM} type;
		static ETrigType convertTrigger(const std::string & trig)
		{
			static const std::string validTriggers[] = {"AE", "BA", "BF", "BG", "BR", "CM", "CO", "FU",
				"GE", "GM", "HE", "HL", "HM", "IP", "LE", "MF", "MG", "MM", "MR", "MW", "OB", "PI", "SN",
				"TH", "TM"};

			for(int i=0; i<ARRAY_COUNT(validTriggers); ++i)
			{
				if(validTriggers[i] == trig)
					return static_cast<ETrigType>(i);
			}
			throw EInvalidTrigger(trig);
		}

		bool operator<(const TriggerType & t2) const
		{
			return type < t2.type;
		}

		TriggerType(const std::string & sym)
		{
			type = convertTrigger(sym);
		}

	};

	struct FileInfo
	{
		std::string filename;
		int length;
	};

	struct LinePointer
	{
		const FileInfo * file; //non-owning
		int lineNum;

		LinePointer() : file(NULL)
		{}

		LinePointer(const FileInfo * finfo, int line) : file(finfo), lineNum(line)
		{}

		//lexicographical order
		bool operator<(const LinePointer & rhs) const
		{
			if(file->filename != rhs.file->filename)
				return file->filename < rhs.file->filename;

			return lineNum < rhs.lineNum;
		}

		bool operator!=(const LinePointer & rhs) const
		{
			return file->filename != rhs.file->filename || lineNum != rhs.lineNum;
		}
		LinePointer & operator++()
		{
			++lineNum;
			return *this;
		}
		bool isValid() const
		{
			return file && lineNum < file->length;
		}
	};

	struct LexicalPtr
	{
		LinePointer line; //where to start
		std::vector<int> entryPoints; //defines how to pass to current location

		bool operator<(const LexicalPtr & sec) const
		{
			if(line != sec.line)
				return line < sec.line;

			if(entryPoints.size() != sec.entryPoints.size())
				return entryPoints.size() < sec.entryPoints.size();

			for(int g=0; g<entryPoints.size(); ++g)
			{
				if(entryPoints[g] < sec.entryPoints[g])
					return true;
			}

			return false;
		}
	};

	//call stack, represents dynamic range
	struct Stack
	{
		std::vector<LexicalPtr> stack;
	};

	struct Trigger
	{
		LinePointer line;
		TriggerLocalVars ermLocalVars;
		Stack * stack; //where we are stuck at execution
		Trigger() : stack(NULL)
		{}
	};

}

class ERMInterpreter;

struct TriggerIdentifierMatch
{
	bool allowNoIdetifier;
	std::map< int, std::vector<int> > matchToIt; //match subidentifiers to these numbers

	static const int MAX_SUBIDENTIFIERS = 16;
	ERMInterpreter * ermEnv;
	bool tryMatch(VERMInterpreter::Trigger * interptrig) const;
};

class ERMInterpreter
{
	friend class ScriptScanner;
	friend class TriggerIdMatchHelper;
	friend class TriggerIdentifierMatch;

	std::vector<VERMInterpreter::FileInfo*> files;
	std::vector< VERMInterpreter::FileInfo* > fileInfos;
	std::map<VERMInterpreter::LinePointer, ERM::TLine> scripts;
	std::map<VERMInterpreter::LexicalPtr, VERMInterpreter::Environment> lexicalEnvs;
	ERM::TLine retrieveLine(VERMInterpreter::LinePointer linePtr) const;
	static ERM::Ttrigger retrieveTrigger(ERM::TLine line);

	VERMInterpreter::Environment * globalEnv;
	VERMInterpreter::ERMEnvironment * ermGlobalEnv;
	typedef std::map<VERMInterpreter::TriggerType, std::vector<VERMInterpreter::Trigger> > TtriggerListType;
	TtriggerListType triggers, postTriggers;


	template<typename T> void setIexp(const ERM::TIexp & iexp, const T & val, VERMInterpreter::Trigger * trig = NULL);
	template<typename T> T getIexp(const ERM::TIexp & iexp, /*const*/ VERMInterpreter::Trigger * trig = NULL, /*const*/ VERMInterpreter::FunctionLocalVars * fun = NULL) const;

	static const std::string triggerSymbol, postTriggerSymbol, defunSymbol;

	void executeLine(const VERMInterpreter::LinePointer & lp);
	void executeTrigger(VERMInterpreter::Trigger & trig);
	static bool isCMDATrigger(const ERM::Tcommand & cmd);
	static bool isATrigger(const ERM::TLine & line);
	static ERM::EVOtions getExpType(const ERM::TVOption & opt);
public:
	void executeTriggerType(VERMInterpreter::TriggerType tt, bool pre, const std::map< int, std::vector<int> > & identifier); //use this to run triggers
	void init(); //sets up environment etc.
	void scanForScripts();

	enum EPrintMode{ALL, ERM_ONLY, VERM_ONLY};
	void printScripts(EPrintMode mode = ALL);
	void scanScripts(); //scans for functions, triggers etc.

	ERMInterpreter();
};
