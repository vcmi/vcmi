/*
 * ERMParser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <boost/spirit/home/support/unused.hpp>

namespace spirit = boost::spirit;

class CERMPreprocessor
{
	std::string fname;
	std::stringstream sourceStream;
	int lineNo;


	void getline(std::string &ret);

public:
	enum class Version : ui8
	{
		INVALID,
		ERM,
		VERM
	};
	Version version;

	CERMPreprocessor(const std::string & source);
	std::string retrieveCommandLine();
	int getCurLineNo() const
	{
		return lineNo;
	}

	const std::string& getCurFileName() const
	{
		return fname;
	}
};

//various classes that represent ERM/VERM AST
namespace ERM
{
	using ValType = int; //todo: set to int64_t
	using IType = int; //todo: set to int32_t

	struct TStringConstant
	{
		std::string str;
	};
	struct TMacroUsage
	{
		std::string macro;
	};

// 	//macro with '?', for write only
// 	struct TQMacroUsage
// 	{
// 		std::string qmacro;
// 	};

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

	typedef std::variant<TVarExpNotMacro, TMacroUsage> TVarExp;

	//write-only variable expression
	struct TVarpExp
	{
		TVarExp var;
	};

	//i-expression (identifier expression) - an integral constant, variable symbol or array symbol
	typedef std::variant<TVarExp, int> TIexp;

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

	typedef std::variant<TVarConcatString, TStringConstant, TCurriedString, TSemiCompare, TMacroDef, TIexp, TVarpExp> TBodyOptionItem;

	typedef std::vector<TBodyOptionItem> TNormalBodyOptionList;

	struct TNormalBodyOption
	{
		char optionCode;
		boost::optional<TNormalBodyOptionList> params;
	};
	typedef std::variant<TVRLogic, TVRArithmetic, TNormalBodyOption> TBodyOption;

//	typedef std::variant<TIexp, TArithmeticOp > TIdentifierInternal;
	typedef std::vector< TIexp > Tidentifier;

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
		typedef std::variant<
			TComparison,
			int>
			Tcond; //comparison or condition flag
		char ctype;
		Tcond cond;
		TconditionNode rhs;
	};

	struct TTriggerBase
	{
		bool pre; //if false it's !$ post-trigger, elsewise it's !# (pre)trigger
		TCmdName name;
		boost::optional<Tidentifier> identifier;
		boost::optional<Tcondition> condition;
	};

	struct Ttrigger : TTriggerBase
	{
		Ttrigger()
		{
			pre = true;
		}
	};

	struct TPostTrigger : TTriggerBase
	{
		TPostTrigger()
		{
			pre = false;
		}
	};

	//a dirty workaround for preprocessor magic that prevents the use types with comma in it in BOOST_FUSION_ADAPT_STRUCT
	//see http://comments.gmane.org/gmane.comp.lib.boost.user/62501 for some info
	//
	//moreover, I encountered a quite serious bug in boost: http://boost.2283326.n4.nabble.com/container-hpp-111-error-C2039-value-type-is-not-a-member-of-td3352328.html
	//not sure how serious it is...

	//typedef std::variant<char, TStringConstant, TMacroUsage, TMacroDef> bodyItem;
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

	struct Tcommand
	{
		typedef	std::variant<
			Ttrigger,
			Tinstruction,
			Treceiver,
			TPostTrigger
		>
		Tcmd;
		Tcmd cmd;
		//std::string comment;
	};

	//vector expression


	typedef std::variant<Tcommand, std::string, boost::spirit::unused_type> TERMline;

	typedef std::string TVModifier; //'`', ',', ',@', '#''

	struct TSymbol
	{
		std::vector<TVModifier> symModifier;
		std::string sym;
	};

	//for #'symbol expression

	enum EVOtions{VEXP, SYMBOL, CHAR, DOUBLE, INT, TCMD, STRINGC};
	struct TVExp;
	typedef std::variant<boost::recursive_wrapper<TVExp>, TSymbol, char, double, int, Tcommand, TStringConstant > TVOption; //options in v-expression
	//v-expression
	struct TVExp
	{
		std::vector<TVModifier> modifier;
		std::vector<TVOption> children;
	};

	//script line
	typedef std::variant<TVExp, TERMline> TLine;

	template <typename T> struct ERM_grammar;
}

struct LineInfo
{
	ERM::TLine tl;
	int realLineNum;
};

class ERMParser
{
public:
	std::shared_ptr<ERM::ERM_grammar<std::string::const_iterator>> ERMgrammar;

	ERMParser();
	virtual ~ERMParser();

	std::vector<LineInfo> parseFile(CERMPreprocessor & preproc);
private:
	void repairEncoding(char * str, int len) const; //removes nonstandard ascii characters from string
	void repairEncoding(std::string & str) const; //removes nonstandard ascii characters from string
	ERM::TLine parseLine(const std::string & line, int realLineNo);
	ERM::TLine parseLine(const std::string & line);
};
