#pragma once
#include "../global.h"
#include <fstream>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/home/support/unused.hpp>

/*
 * ERMParser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


class CERMPreprocessor
{
	std::string fname;
	std::ifstream file;
	int lineNo;
	enum {INVALID, ERM, VERM} version;

	void getline(std::string &ret);

public:
	CERMPreprocessor(const std::string &Fname);
	std::string retreiveCommandLine();
	int getCurLineNo() const
	{
		return lineNo;
	}
};

//various classes that represent ERM/VERM AST
namespace ERM
{
	struct TStringConstant
	{
		std::string str;
	};
	struct TMacroUsage
	{
		std::string macro;
	};

	//macro with '?', for write only
	struct TQMacroUsage
	{
		std::string qmacro;
	};

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

	typedef boost::variant<TVarExpNotMacro, TMacroUsage> TVarExp;

	//write-only variable expression
	struct TVarpExp
	{
		typedef boost::variant<TVarExpNotMacro, TQMacroUsage> Tvartype;
		Tvartype var;
	};

	//i-expression (identifier expression) - an integral constant, variable symbol or array symbol
	typedef boost::variant<TVarExp, int> TIexp;

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

	typedef boost::variant<TVarConcatString, TStringConstant, TCurriedString, TSemiCompare, TMacroUsage, TMacroDef, TIexp, TVarpExp, boost::spirit::unused_type> TBodyOptionItem;

	typedef std::vector<TBodyOptionItem> TNormalBodyOptionList;

	struct TNormalBodyOption
	{
		char optionCode;
		TNormalBodyOptionList params;
	};
	typedef boost::variant<TVRLogic, TVRArithmetic, TNormalBodyOption> TBodyOption;

	typedef boost::variant<TIexp, TArithmeticOp > TIdentifierInternal;
	typedef std::vector< TIdentifierInternal > Tidentifier;

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
		typedef boost::variant<
			TComparison,
			int>
			Tcond; //comparison or condition flag
		char ctype;
		Tcond cond;
		TconditionNode rhs;
	};

	struct Ttrigger
	{
		TCmdName name;
		boost::optional<Tidentifier> identifier;
		boost::optional<Tcondition> condition;
	};

	//a dirty workaround for preprocessor magic that prevents the use types with comma in it in BOOST_FUSION_ADAPT_STRUCT
	//see http://comments.gmane.org/gmane.comp.lib.boost.user/62501 for some info
	//
	//moreover, I encountered a quite serious bug in boost: http://boost.2283326.n4.nabble.com/container-hpp-111-error-C2039-value-type-is-not-a-member-of-td3352328.html
	//not sure how serious it is...

	//typedef boost::variant<char, TStringConstant, TMacroUsage, TMacroDef> bodyItem;
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

	struct TPostTrigger
	{
		TCmdName name;
		boost::optional<Tidentifier> identifier;
		boost::optional<Tcondition> condition;
	};

	struct Tcommand
	{
		typedef	boost::variant<
			Ttrigger,
			Tinstruction,
			Treceiver,
			TPostTrigger
		>
		Tcmd;
		Tcmd cmd;
		std::string comment;
	};

	//vector expression


	typedef boost::variant<Tcommand, std::string, boost::spirit::unused_type> TERMline;

	typedef std::string TVModifier; //'`', ',', ',@', '#''

	struct TSymbol
	{
		std::vector<TVModifier> symModifier;
		std::string sym;
	};

	//for #'symbol expression

	enum EVOtions{VEXP, SYMBOL, CHAR, DOUBLE, INT, TCMD, STRINGC};
	struct TVExp;
	typedef boost::variant<boost::recursive_wrapper<TVExp>, TSymbol, char, double, int, Tcommand, TStringConstant > TVOption; //options in v-expression
	//v-expression
	struct TVExp
	{
		std::vector<TVModifier> modifier;
		std::vector<TVOption> children;
	};

	//script line
	typedef boost::variant<TVExp, TERMline> TLine;
}

struct LineInfo
{
	ERM::TLine tl;
	int realLineNum;
};

class ERMParser
{
private:
	std::string srcFile;
	void repairEncoding(char * str, int len) const; //removes nonstandard ascii characters from string
	void repairEncoding(std::string & str) const; //removes nonstandard ascii characters from string
	enum ELineType{COMMAND_FULL, COMMENT, UNFINISHED, END_OF};
	int countHatsBeforeSemicolon(const std::string & line) const;
	ELineType classifyLine(const std::string & line, bool inString) const;
	ERM::TLine parseLine(const std::string & line, int realLineNo);


public:
	ERMParser(std::string file);
	std::vector<LineInfo> parseFile();
};
