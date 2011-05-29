#pragma once
#include "../global.h"
#include "ERMParser.h"
#include "IGameEventsReceiver.h"
#include "ERMScriptModule.h"


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

	struct EExecutionError : public EInterpreterProblem
	{
		EExecutionError(const std::string & desc) :
			EInterpreterProblem(desc)
		{}
	};

	//internal interpreter error related to execution
	struct EInterpreterError : public EExecutionError
	{
		EInterpreterError(const std::string & desc) :
			EExecutionError(desc)
		{}
	};

	//wrong script
	struct EScriptExecError : public EExecutionError
	{
		EScriptExecError(const std::string & desc) :
			EExecutionError(desc)
		{}
	};


	///main environment class, manages symbols
	class Environment
	{
	private:
		std::map<std::string, TVOption> symbols;
		Environment * parent;

	public:
		bool isBound(const std::string & name, bool globalOnly) const;

		TVOption retrieveValue(const std::string & name) const;

		enum EUnbindMode{LOCAL, RECURSIVE_UNTIL_HIT, FULLY_RECURSIVE};
		///returns true if symbol was really unbound
		bool unbind(const std::string & name, EUnbindMode mode);
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

		static const int YVAR_NUM = 100; //number of yvar locals
		TriggerLocalVars();

		double & getEvar(int num);
		int & getYvar(int num);
	private:
		double evar[EVAR_NUM]; //negative indices
		int yvar[YVAR_NUM];

	};

	struct FunctionLocalVars
	{
		static const int NUM_PARAMETERS = 16; //number of function parameters
		static const int NUM_LOCALS = 100;
		static const int NUM_STRINGS = 10;
		static const int NUM_FLOATINGS = 100;

		int & getParam(int num);
		int & getLocal(int num);
		std::string & getString(int num);
		double & getFloat(int num);
		void reset();
	private:
		int params[NUM_PARAMETERS]; //x-vars
		int locals[NUM_LOCALS]; //y-vars
		std::string strings[NUM_STRINGS]; //z-vars (negative indices)
		double floats[NUM_FLOATINGS]; //e-vars (positive indices)
	};

	struct ERMEnvironment
	{
		ERMEnvironment();
		static const int NUM_QUICKS = 't' - 'f' + 1; //it should be 15
		int & getQuickVar(const char letter); //'f' - 't' variables
		int & getStandardVar(int num); //get v-variable
		std::string & getZVar(int num);
		bool & getFlag(int num);

		static const int NUM_STANDARDS = 10000;

		static const int NUM_STRINGS = 1000;

		std::map<std::string, ERM::TVarExpNotMacro> macroBindings;

		static const int NUM_FLAGS = 1000;
	private:
		int quickVars[NUM_QUICKS]; //referenced by letter ('f' to 't' inclusive)
		int standardVars[NUM_STANDARDS]; //v-vars
		std::string strings[NUM_STRINGS]; //z-vars (positive indices)
		bool flags[NUM_FLAGS];
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

		int realLineNum;

		LinePointer() : file(NULL)
		{}

		LinePointer(const FileInfo * finfo, int line, int _realLineNum) : file(finfo), lineNum(line),
			realLineNum(_realLineNum)
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


	//verm goodies
	struct VSymbol
	{
		std::string text;
		VSymbol(const std::string & txt) : text(txt)
		{}
	};
	typedef boost::variant<char, double, int, std::string> TLiteral;

	struct VNIL
	{};

	struct VNode;
	struct VOptionList;

	typedef boost::variant<VNIL, boost::recursive_wrapper<VNode>, VSymbol, TLiteral, ERM::Tcommand> VOption; //options in v-expression, VNIl should be the default


	struct VermTreeIterator
	{
	private:
		friend struct VOptionList;
		VOptionList & parent;
		enum Estate {CAR, CDR} state;
	public:
		VermTreeIterator(VOptionList & _parent) : parent(_parent)
		{}

		VermTreeIterator & operator=(const VOption & opt);
		VermTreeIterator & operator=(const std::vector<VOption> & opt);
		VermTreeIterator & operator=(const VOptionList & opt);
	};

	struct VOptionList
	{
		std::vector<VOption> elements;
		VermTreeIterator car()
		{
			VermTreeIterator ret(*this);
			ret.state = VermTreeIterator::CAR;
			return ret;
		}
		VermTreeIterator cdr()
		{
			VermTreeIterator ret(*this);
			ret.state = VermTreeIterator::CDR;
			return ret;
		}
		bool isNil() const
		{
			return elements.size() == 0;
		}
	};

	struct OptionConverterVisitor : boost::static_visitor<VOption>
	{
		VOption operator()(ERM::TVExp const& cmd) const;
		VOption operator()(ERM::TSymbol const& cmd) const;
		VOption operator()(char const& cmd) const;
		VOption operator()(double const& cmd) const;
		VOption operator()(int const& cmd) const;
		VOption operator()(ERM::Tcommand const& cmd) const;
		VOption operator()(ERM::TStringConstant const& cmd) const;
	};

	struct VNode
	{
	private:
		void processModifierList(const std::vector<TVModifier> & modifierList);
	public:
		VOptionList children;
		VNode( const ERM::TVExp & exp);
		VNode( const VOptionList & cdren );
		VNode( const ERM::TSymbol & sym ); //only in case sym has modifiers!
		VNode( const VOption & first, const VOptionList & rest); //merges given arguments into [a, rest];
		void setVnode( const VOption & first, const VOptionList & rest);
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

struct IexpValStr
{
private:
	union
	{
		int val;
		int * integervar;
		double * flvar;
		std::string * stringvar;
	} val;
public:
	std::string name;
	std::string getName() const;

	enum {WRONGVAL, INT, INTVAR, FLOATVAR, STRINGVAR} type;
	void setTo(const IexpValStr & second);
	void setTo(int val);
	void setTo(float val);
	void setTo(const std::string & val);
	int getInt() const;
	float getFloat() const;
	std::string getString() const;

	IexpValStr() : type(WRONGVAL)
	{}
	IexpValStr(int _val) : type(INT)
	{
		val.val = _val;
	}
	IexpValStr(int* _val) : type(INTVAR)
	{
		val.integervar = _val;
	}
	IexpValStr(double * _val) : type(FLOATVAR)
	{
		val.flvar = _val;
	}
	IexpValStr(std::string * _val) : type(STRINGVAR)
	{
		val.stringvar = _val;
	}

#define OPERATOR_DEFINITION_FULL(OPSIGN) \
	template<typename T> \
	IexpValStr operator OPSIGN(const T & sec) const \
	{ \
		IexpValStr ret = *this; \
		switch (type) \
		{ \
		case INT: \
		case INTVAR: \
			ret.setTo(ret.getInt() OPSIGN sec); \
			break; \
		case FLOATVAR: \
			ret.setTo(ret.getFloat() OPSIGN sec); \
			break; \
		case STRINGVAR: \
			ret.setTo(ret.getString() OPSIGN sec); \
			break; \
		} \
		return ret; \
	} \
	IexpValStr operator OPSIGN(const IexpValStr & sec) const \
	{ \
		IexpValStr ret = *this; \
		switch (type) \
		{ \
		case INT: \
		case INTVAR: \
			ret.setTo(ret.getInt() OPSIGN sec.getInt()); \
			break; \
		case FLOATVAR: \
			ret.setTo(ret.getFloat() OPSIGN sec.getFloat()); \
			break; \
		case STRINGVAR: \
			ret.setTo(ret.getString() OPSIGN sec.getString()); \
			break; \
		} \
		return ret; \
	} \
	template<typename T> \
	IexpValStr & operator OPSIGN ## = (const T & sec) \
	{ \
		*this = *this OPSIGN sec; \
		return *this; \
	}

#define OPERATOR_DEFINITION(OPSIGN) \
	template<typename T> \
	IexpValStr operator OPSIGN(const T & sec) const \
	{ \
		IexpValStr ret = *this; \
		switch (type) \
		{ \
		case INT: \
		case INTVAR: \
			ret.setTo(ret.getInt() OPSIGN sec); \
			break; \
		case FLOATVAR: \
			ret.setTo(ret.getFloat() OPSIGN sec); \
			break; \
		} \
		return ret; \
	} \
	IexpValStr operator OPSIGN(const IexpValStr & sec) const \
	{ \
		IexpValStr ret = *this; \
		switch (type) \
		{ \
		case INT: \
		case INTVAR: \
			ret.setTo(ret.getInt() OPSIGN sec.getInt()); \
			break; \
		case FLOATVAR: \
			ret.setTo(ret.getFloat() OPSIGN sec.getFloat()); \
			break; \
		} \
		return ret; \
	} \
	template<typename T> \
	IexpValStr & operator OPSIGN ## = (const T & sec) \
	{ \
		*this = *this OPSIGN sec; \
		return *this; \
	}

#define OPERATOR_DEFINITION_INTEGER(OPSIGN) \
	template<typename T> \
	IexpValStr operator OPSIGN(const T & sec) const \
	{ \
		IexpValStr ret = *this; \
		switch (type) \
		{ \
		case INT: \
		case INTVAR: \
			ret.setTo(ret.getInt() OPSIGN sec); \
			break; \
		} \
		return ret; \
	} \
	IexpValStr operator OPSIGN(const IexpValStr & sec) const \
	{ \
		IexpValStr ret = *this; \
		switch (type) \
		{ \
		case INT: \
		case INTVAR: \
			ret.setTo(ret.getInt() OPSIGN sec.getInt()); \
			break; \
		} \
		return ret; \
	} \
	template<typename T> \
	IexpValStr & operator OPSIGN ## = (const T & sec) \
	{ \
		*this = *this OPSIGN sec; \
		return *this; \
	}

	OPERATOR_DEFINITION_FULL(+)
	OPERATOR_DEFINITION(-)
	OPERATOR_DEFINITION(*)
	OPERATOR_DEFINITION(/)
	OPERATOR_DEFINITION_INTEGER(%)
};

class ERMInterpreter : public CScriptingModule
{
/*not so*/ public:
// 	friend class ScriptScanner;
// 	friend class TriggerIdMatchHelper;
// 	friend class TriggerIdentifierMatch;
// 	friend class ConditionDisemboweler;
// 	friend struct LVL2IexpDisemboweler;
// 	friend struct VR_SPerformer;
// 	friend struct ERMExpDispatch;
// 	friend struct VRPerformer;

	std::vector<VERMInterpreter::FileInfo*> files;
	std::vector< VERMInterpreter::FileInfo* > fileInfos;
	std::map<VERMInterpreter::LinePointer, ERM::TLine> scripts;
	std::map<VERMInterpreter::LexicalPtr, VERMInterpreter::Environment> lexicalEnvs;
	ERM::TLine &retrieveLine(VERMInterpreter::LinePointer linePtr);
	static ERM::TTriggerBase & retrieveTrigger(ERM::TLine &line);

	VERMInterpreter::Environment * globalEnv;
	VERMInterpreter::ERMEnvironment * ermGlobalEnv;
	typedef std::map<VERMInterpreter::TriggerType, std::vector<VERMInterpreter::Trigger> > TtriggerListType;
	TtriggerListType triggers, postTriggers;
	VERMInterpreter::Trigger * curTrigger;
	VERMInterpreter::FunctionLocalVars * curFunc;
	static const int TRIG_FUNC_NUM = 30000;
	VERMInterpreter::FunctionLocalVars funcVars[TRIG_FUNC_NUM+1]; //+1 because we use [0] as a global set of y-vars
	VERMInterpreter::FunctionLocalVars * getFuncVars(int funNum); //0 is a global func-like set

	IexpValStr getIexp(const ERM::TIexp & iexp) const;
	IexpValStr getIexp(const ERM::TMacroUsage & macro) const;
	IexpValStr getIexp(const ERM::TIdentifierInternal & tid) const;
	IexpValStr getIexp(const ERM::TVarpExp & tid) const;
	IexpValStr getIexp(const ERM::TBodyOptionItem & opit) const;

	static const std::string triggerSymbol, postTriggerSymbol, defunSymbol;

	void executeLine(const VERMInterpreter::LinePointer & lp);
	void executeTrigger(VERMInterpreter::Trigger & trig, int funNum = -1, std::vector<int> funParams=std::vector<int>());
	static bool isCMDATrigger(const ERM::Tcommand & cmd);
	static bool isATrigger(const ERM::TLine & line);
	static ERM::EVOtions getExpType(const ERM::TVOption & opt);
	IexpValStr getVar(std::string toFollow, boost::optional<int> initVal) const;

	std::string processERMString(std::string ermstring);

public:
	typedef std::map< int, std::vector<int> > TIDPattern;
	void executeInstructions(); //called when starting a new game, before most of the map settings are done
	void executeTriggerType(VERMInterpreter::TriggerType tt, bool pre, const TIDPattern & identifier, const std::vector<int> &funParams=std::vector<int>()); //use this to run triggers
	void executeTriggerType(const char *trigger, int id); //convenience version of above, for pre-trigger when there is only one argument
	void executeTriggerType(const char *trigger); //convenience version of above, for pre-trigger when there are no args
	void setCurrentlyVisitedObj(int3 pos); //sets v998 - v1000 to given value
	void scanForScripts();

	enum EPrintMode{ALL, ERM_ONLY, VERM_ONLY};
	void printScripts(EPrintMode mode = ALL);
	void scanScripts(); //scans for functions, triggers etc.

	ERMInterpreter();
	bool checkCondition( ERM::Tcondition cond );
	int getRealLine(int lineNum);

	//overload CScriptingModule
	virtual void heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start) OVERRIDE;
	virtual void init() OVERRIDE;//sets up environment etc.
	virtual void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) OVERRIDE;

	const CGObjectInstance *getObjFrom(int3 pos);
	template <typename T>
	const T *getObjFromAs(int3 pos)
	{
		const T* obj =  dynamic_cast<const T*>(getObjFrom(pos));
		if(obj)
			return obj;
		else
			throw VERMInterpreter::EScriptExecError("Wrong cast attempted, object is not of a desired type!");
	}
};
