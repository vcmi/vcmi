/*
 * ERMInterpreter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ERMParser.h"
#include "ERMScriptModule.h"

class ERMInterpreter;

namespace VERMInterpreter
{
	using namespace ERM;

	//different exceptions that can be thrown during interpreting
	class EInterpreterProblem : public std::exception
	{
		std::string problem;
	public:
		const char * what() const throw() override
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

	//wrong script
	struct EVermScriptExecError : public EScriptExecError
	{
		EVermScriptExecError(const std::string & desc) :
			EScriptExecError(desc)
		{}
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


	struct TriggerType
	{
		//the same order of trigger types in this enum and in validTriggers array is obligatory!
		enum ETrigType
		{
			AE, BA, BF, BG, BR, CM, CO, FU, GE, GM, HE, HL, HM, IP, LE, MF, MG, MM, MR,	MW, OB, PI, SN, TH, TM
		};

		ETrigType type;

		static ETrigType convertTrigger(const std::string & trig)
		{
			static const std::string validTriggers[] =
			{
				"AE", "BA", "BF", "BG", "BR", "CM", "CO", "FU",
				"GE", "GM", "HE", "HL", "HM", "IP", "LE", "MF", "MG", "MM", "MR", "MW", "OB", "PI", "SN",
				"TH", "TM"
			};

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

	struct LinePointer
	{
		int lineNum;
		int realLineNum;
		int fileLength;

		LinePointer()
			: fileLength(-1)
		{}

		LinePointer(int _fileLength, int line, int _realLineNum)
			: fileLength(_fileLength),
			lineNum(line),
			realLineNum(_realLineNum)
		{}

		bool operator<(const LinePointer & rhs) const
		{
			return lineNum < rhs.lineNum;
		}

		bool operator!=(const LinePointer & rhs) const
		{
			return lineNum != rhs.lineNum;
		}
		LinePointer & operator++()
		{
			++lineNum;
			return *this;
		}
		bool isValid() const
		{
			return fileLength > 0 && lineNum < fileLength;
		}
	};

	struct Trigger
	{
		LinePointer line;
		Trigger()
		{}
	};


	//verm goodies
	struct VSymbol
	{
		std::string text;
		VSymbol(const std::string & txt) : text(txt)
		{}
	};

	struct VNode;
	struct VOptionList;

	struct VNIL
	{};


	typedef std::variant<char, double, int, std::string> TLiteral;

	typedef std::variant<VNIL, boost::recursive_wrapper<VNode>, VSymbol, TLiteral, ERM::Tcommand> VOption; //options in v-expression, VNIl should be the default

	template<typename T, typename SecType>
	T& getAs(SecType & opt)
	{
		if(opt.type() == typeid(T))
			return std::get<T>(opt);
		else
			throw EVermScriptExecError("Wrong type!");
	}

	template<typename T, typename SecType>
	bool isA(const SecType & opt)
	{
		if(opt.type() == typeid(T))
			return true;
		else
			return false;
	}

	struct VermTreeIterator
	{
	private:
		friend struct VOptionList;
		VOptionList * parent;
		enum Estate {NORM, CAR} state;
		int basePos; //car/cdr offset
	public:
		VermTreeIterator(VOptionList & _parent) : parent(&_parent), state(NORM), basePos(0)
		{}
		VermTreeIterator() : parent(nullptr), state(NORM)
		{}

		VermTreeIterator & operator=(const VOption & opt);
		VermTreeIterator & operator=(const std::vector<VOption> & opt);
		VermTreeIterator & operator=(const VOptionList & opt);
		VOption & getAsItem();
		VOptionList getAsList();
		size_t size() const;

		VermTreeIterator& operator=(const VermTreeIterator & rhs)
		{
			if(this == &rhs)
			{
				return *this;
			}
			parent = rhs.parent;
			state = rhs.state;
			basePos = rhs.basePos;

			return *this;
		}
	};

	struct VOptionList : public std::vector<VOption>
	{
	private:
		friend struct VermTreeIterator;
	public:
		VermTreeIterator car();
		VermTreeIterator cdr();
	};

	struct OptionConverterVisitor
	{
		VOption operator()(ERM const ::TVExp & cmd) const;
		VOption operator()(ERM const ::TSymbol & cmd) const;
		VOption operator()(const char & cmd) const;
		VOption operator()(const double & cmd) const;
		VOption operator()(const int & cmd) const;
		VOption operator()(ERM const ::Tcommand & cmd) const;
		VOption operator()(ERM const ::TStringConstant & cmd) const;
	};

	struct VNode
	{
	private:
		void processModifierList(const std::vector<TVModifier> & modifierList, bool asSymbol);
	public:
		VOptionList children;
		VNode( const ERM::TVExp & exp);
		VNode( const VOptionList & cdren );
		VNode( const ERM::TSymbol & sym ); //only in case sym has modifiers!
		VNode( const VOption & first, const VOptionList & rest); //merges given arguments into [a, rest];
		void setVnode( const VOption & first, const VOptionList & rest);
	};
}

class ERMInterpreter
{
/*not so*/ public:

	std::map<VERMInterpreter::LinePointer, ERM::TLine> scripts;

	typedef std::map<VERMInterpreter::TriggerType, std::vector<VERMInterpreter::Trigger> > TtriggerListType;
	TtriggerListType triggers;
	TtriggerListType postTriggers;
	std::vector<VERMInterpreter::LinePointer> instructions;

	static bool isCMDATrigger(const ERM::Tcommand & cmd);
	static bool isATrigger(const ERM::TLine & line);
	static ERM::EVOtions getExpType(const ERM::TVOption & opt);
	ERM::TLine & retrieveLine(const VERMInterpreter::LinePointer & linePtr);
	static ERM::TTriggerBase & retrieveTrigger(ERM::TLine & line);
public:
	vstd::CLoggerBase * logger;

	ERMInterpreter(vstd::CLoggerBase * logger_);
	virtual ~ERMInterpreter();

	std::string loadScript(const std::string & name, const std::string & source);
};
