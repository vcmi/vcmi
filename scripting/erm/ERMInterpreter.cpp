/*
 * ERMInterpreter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ERMInterpreter.h"

#include <cctype>

namespace spirit = boost::spirit;
using ::scripting::ContextBase;
using namespace ::VERMInterpreter;

typedef int TUnusedType;

namespace ERMConverter
{
	//console printer
	using namespace ERM;

	enum class EDir{GET, SET};

	struct LVL2Iexp : boost::static_visitor<std::string>
	{
		EDir dir;

		LVL2Iexp(EDir dir_)
			: dir(dir_)
		{}

		std::string processNotMacro(TVarExpNotMacro const & val) const
		{
			if(val.questionMark.is_initialized())
				throw EIexpProblem("Question marks ('?') are not allowed in getter i-expressions");

			//TODO:

			if(val.val.is_initialized())
			{
				return boost::to_string(boost::format("%s['%d']") % val.varsym % val.val.get());
			}
			else
			{
				return boost::to_string(boost::format("quick['%s']") % val.varsym);
			}
		}

		std::string operator()(TVarExpNotMacro const & val) const
		{
			return processNotMacro(val);
		}

		std::string operator()(TMacroUsage const & val) const
		{
			return val.macro;
		}
	};

	struct LVL1Iexp : boost::static_visitor<std::string>
	{
		EDir dir;

		LVL1Iexp(EDir dir_)
			: dir(dir_)
		{}

		LVL1Iexp()
			: dir(EDir::GET)
		{}

		std::string operator()(int const & constant) const
		{
			if(dir == EDir::GET)
			{
				return std::to_string(constant);
			}
			else
			{
				throw EIexpProblem("Cannot set a constant!");
			}
		}

		std::string operator()(TVarExp const & var) const
		{
			return boost::apply_visitor(LVL2Iexp(dir), var);
		}
	};

	struct Condition : public boost::static_visitor<std::string>
	{
		Condition()
		{}

		std::string operator()(TComparison const & cmp) const
		{
			std::string lhs = boost::apply_visitor(LVL1Iexp(), cmp.lhs);
			std::string rhs = boost::apply_visitor(LVL1Iexp(), cmp.rhs);

			static const std::map<std::string, std::string> OPERATION =
			{
				{"<", "<"},
				{">", ">"},
				{">=", ">="},
				{"=>", ">="},
				{"<=", "<="},
				{"=<", "<="},
				{"==", "=="},
				{"<>", "~="},
				{"><", "~="},
			};

			auto sign = OPERATION.find(cmp.compSign);
			if(sign == std::end(OPERATION))
				throw EScriptExecError(std::string("Wrong comparison sign: ") + cmp.compSign);

			boost::format fmt("(%s %s %s)");
			fmt % lhs % sign->second % rhs;
			return fmt.str();
		}
		std::string operator()(int const & flag) const
		{
			return boost::to_string(boost::format("ERM.flag['%d']") % flag);
		}
	};

	struct ParamIO
	{
		std::string name;
		bool isInput;
	};

	struct Converter : public boost::static_visitor<>
	{
		mutable std::ostream * out;
		Converter(std::ostream * out_)
			: out(out_)
		{}
	};

	struct GetBodyOption : public boost::static_visitor<std::string>
	{
		GetBodyOption()
		{}

		virtual std::string operator()(TVarConcatString const & cmp) const
		{
			throw EScriptExecError("String concatenation not allowed in this receiver");
		}
		virtual std::string operator()(TStringConstant const & cmp) const
		{
			throw EScriptExecError("String constant not allowed in this receiver");
		}
		virtual std::string operator()(TCurriedString const & cmp) const
		{
			throw EScriptExecError("Curried string not allowed in this receiver");
		}
		virtual std::string operator()(TSemiCompare const & cmp) const
		{
			throw EScriptExecError("Semi comparison not allowed in this receiver");
		}
	// 	virtual void operator()(TMacroUsage const& cmp) const
	// 	{
	// 		throw EScriptExecError("Macro usage not allowed in this receiver");
	// 	}
		virtual std::string operator()(TMacroDef const & cmp) const
		{
			throw EScriptExecError("Macro definition not allowed in this receiver");
		}
		virtual std::string operator()(TIexp const & cmp) const
		{
			throw EScriptExecError("i-expression not allowed in this receiver");
		}
		virtual std::string operator()(TVarpExp const & cmp) const
		{
			throw EScriptExecError("Varp expression not allowed in this receiver");
		}
		virtual std::string operator()(spirit::unused_type const & cmp) const
		{
			throw EScriptExecError("\'Nothing\' not allowed in this receiver");
		}
	};

	struct BodyOption : public boost::static_visitor<ParamIO>
	{
		ParamIO operator()(TVarConcatString const & cmp) const
		{
			throw EScriptExecError("String concatenation not allowed in this receiver");
		}

		ParamIO operator()(TStringConstant const & cmp) const
		{
			boost::format fmt("[===[%s]===]");
			fmt % cmp.str;

			ParamIO ret;
			ret.isInput = true;
			ret.name = fmt.str();
			return ret;
		}

		ParamIO operator()(TCurriedString const & cmp) const
		{
			throw EScriptExecError("Curried string not allowed in this receiver");
		}

		ParamIO operator()(TSemiCompare const & cmp) const
		{
			throw EScriptExecError("Semi comparison not allowed in this receiver");
		}

		ParamIO operator()(TMacroDef const & cmp) const
		{
			throw EScriptExecError("Macro definition not allowed in this receiver");
		}

		ParamIO operator()(TIexp const & cmp) const
		{
			ParamIO ret;
			ret.isInput = true;
			ret.name = boost::apply_visitor(LVL1Iexp(), cmp);
			return ret;
		}

		ParamIO operator()(TVarpExp const & cmp) const
		{
			ParamIO ret;
			ret.isInput = false;

			ret.name = boost::apply_visitor(LVL2Iexp(EDir::SET), cmp.var);
			return ret;
		}

		ParamIO operator()(spirit::unused_type const & cmp) const
		{
			throw EScriptExecError("\'Nothing\' not allowed in this receiver");
		}
	};

	struct VR_S : public GetBodyOption
	{
		VR_S()
		{}

		using GetBodyOption::operator();

		std::string operator()(TIexp const & cmp) const override
		{
			return boost::apply_visitor(LVL1Iexp(), cmp);
		}
		std::string operator()(TStringConstant const & cmp) const override
		{
			boost::format fmt("[===[%s]===]");
			fmt % cmp.str;
			return fmt.str();
		}
	};

	struct Receiver : public Converter
	{
		std::string name;
		std::vector<std::string> identifiers;

		Receiver(std::ostream * out_, std::string name_, std::vector<std::string> identifiers_)
			: Converter(out_),
			name(name_),
			identifiers(identifiers_)
		{}

		void operator()(TVRLogic const & trig) const
		{
			throw EInterpreterError("VR logic is not allowed in this receiver!");
		}

		void operator()(TVRArithmetic const & trig) const
		{
			throw EInterpreterError("VR arithmetic is not allowed in this receiver!");
		}

		void operator()(TNormalBodyOption const & trig) const
		{
			std::string params;
			std::string outParams;
			std::string inParams;

			for(auto iter = std::begin(identifiers); iter != std::end(identifiers); ++iter)
			{
				params += ", ";
				params += *iter;
			}

			{
				std::vector<ParamIO> optionParams;

				for(auto & p : trig.params)
					optionParams.push_back(boost::apply_visitor(BodyOption(), p));

				for(auto & p : optionParams)
				{
					if(p.isInput)
					{
						if(outParams.empty())
							outParams = "_";
						else
							outParams += ", _";

						inParams += ", ";
						inParams += p.name;
					}
					else
					{
						if(outParams.empty())
						{
							outParams = p.name;
						}
						else
						{
							outParams += ", ";
							outParams += p.name;
						}

						inParams += ", nil";
					}
				}
			}

			boost::format callFormat("%s = ERM.%s(x%s):%s(x%s)");

			callFormat % outParams;
			callFormat % name;
			callFormat % params;
			callFormat % trig.optionCode;
			callFormat % inParams;

			(*out) << callFormat.str() << std::endl;
		}
	};

	struct VR : public Converter
	{
		std::string var;

		VR(std::ostream * out_, std::string var_)
			: Converter(out_),
			var(var_)
		{}

		void operator()(TVRLogic const & trig) const
		{
			std::string rhs = boost::apply_visitor(LVL1Iexp(), trig.var);

			std::string opcode;

			switch (trig.opcode)
			{
			case '&':
				opcode = "bit.band";
				break;
			case '|':
				opcode = "bit.bor";
				break;
			case 'X':
				opcode = "bit.bxor";
				break;
			default:
				throw EInterpreterError("Wrong opcode in VR logic expression!");
				break;
			}

			boost::format fmt("%s = %s %s(%s, %s)");
			fmt % var % opcode % var % rhs;

			(*out) << fmt.str() << std::endl;
		}

		void operator()(TVRArithmetic const & trig) const
		{
			std::string rhs = boost::apply_visitor(LVL1Iexp(), trig.rhs);

			std::string opcode;

			switch (trig.opcode)
			{
			case '+':
			case '-':
			case '*':
			case '%':
				opcode = trig.opcode;
				break;
			case ':':
				opcode = "/";
				break;
			default:
				throw EInterpreterError("Wrong opcode in VR arithmetic!");
				break;
			}

			boost::format fmt("%s = %s %s %s");
			fmt % var %  var % opcode % rhs;
			(*out) << fmt.str() << std::endl;
		}

		void operator()(TNormalBodyOption const & trig) const
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
					if(trig.params.size() != 1)
						throw EScriptExecError("VR:S option takes exactly 1 parameter!");

					std::string opt = boost::apply_visitor(VR_S(), trig.params[0]);

					(*out) << var  << " = " << opt << std::endl;
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


	struct ERMExp : public Converter
	{
		ERMExp(std::ostream * out_)
			: Converter(out_)
		{}

		template <typename Visitor>
		void performBody(const boost::optional<ERM::Tbody> & body, const Visitor & visitor) const
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

		void convert(const std::string & name, boost::optional<Tidentifier> identifier, boost::optional<Tbody> body) const
		{
			if(name == "VR")
			{
				if(!identifier.is_initialized())
					throw EScriptExecError("VR receiver requires arguments");

				ERM::Tidentifier tid = identifier.get();
				if(tid.size() != 1)
					throw EScriptExecError("VR receiver takes exactly 1 argument");

				auto var = boost::apply_visitor(LVL1Iexp(), tid[0]);

				performBody(body, VR(out, var));

			}
			else if(name == "FU")
			{
				//TODO: FU receiver
				throw EScriptExecError("FU receiver is not implemented");
			}
			else if(name == "DO")
			{
				//TODO: use P body option
				//TODO: pass|return parameters
				if(!identifier.is_initialized())
					throw EScriptExecError("DO receiver requires arguments");

				ERM::Tidentifier tid = identifier.get();
				if(tid.size() != 4)
					throw EScriptExecError("DO receiver takes exactly 4 arguments");

				auto funNum = boost::apply_visitor(LVL1Iexp(), tid[0]);
				auto startVal = boost::apply_visitor(LVL1Iexp(), tid[1]);
				auto stopVal = boost::apply_visitor(LVL1Iexp(), tid[2]);
				auto increment = boost::apply_visitor(LVL1Iexp(), tid[3]);

				(*out) << "\t" << "for __iter = " << startVal <<", " << stopVal << "-1, " << increment << " do " << std::endl;
				(*out) << "\t\t" << "local x = x or {}" << std::endl;
				(*out) << "\t\t" << "x['16'] = __iter" << std::endl;
				(*out) << "\t\t" << "FU" << funNum << "(x)" << std::endl;
				(*out) << "\t\t" << "__iter = x['16']" << std::endl;
				(*out) << "\t" << "end" << std::endl;
			}
			else
			{
				std::vector<std::string> identifiers;

				if(identifier.is_initialized())
				{
					for(const auto & id : identifier.get())
						identifiers.push_back(boost::apply_visitor(LVL1Iexp(), id));
				}

				performBody(body, Receiver(out, name, identifiers));
			}
		}

		void convertConditionInner(Tcondition const & cond, char op) const
		{
			std::string lhs = boost::apply_visitor(Condition(), cond.cond);

			if(cond.ctype != '/')
				op = cond.ctype;

			switch (op)
			{
			case '&':
				(*out) << " and ";
				break;
			case '|':
				(*out) << " or ";
				break;
			default:
				throw EInterpreterProblem(std::string("Wrong condition connection (") + cond.ctype + ") !");
				break;
			}

			(*out) << lhs;

			if(cond.rhs.is_initialized())
			{
				switch (op)
				{
				case '&':
				case '|':
					break;
				default:
					throw EInterpreterProblem(std::string("Wrong condition connection (") + cond.ctype + ") !");
					break;
				}

				convertConditionInner(cond.rhs.get().get(), op);
			}
		}

		void convertConditionOuter(Tcondition const & cond) const
		{
			//&c1/c2/c3|c4/c5/c6 -> (c1  & c2  & c3)  | c4  |  c5  | c6
			std::string lhs = boost::apply_visitor(Condition(), cond.cond);

			(*out) << lhs;

			if(cond.rhs.is_initialized())
			{
				switch (cond.ctype)
				{
				case '&':
				case '|':
					break;
				default:
					throw EInterpreterProblem(std::string("Wrong condition connection (") + cond.ctype + ") !");
					break;
				}

				convertConditionInner(cond.rhs.get().get(), cond.ctype);
			}
		}

		void convertCondition(Tcondition const & cond) const
		{
			(*out) << " if ";
			convertConditionOuter(cond);
			(*out) << " then " << std::endl;
		}

		void operator()(Ttrigger const & trig) const
		{
			throw EInterpreterError("Triggers cannot be executed!");
		}

		void operator()(TPostTrigger const & trig) const
		{
			throw EInterpreterError("Post-triggers cannot be executed!");
		}

		void operator()(Tinstruction const & trig) const
		{
			if(trig.condition.is_initialized())
			{
				convertCondition(trig.condition.get());

				convert(trig.name, trig.identifier, boost::make_optional(trig.body));

				(*out) << "end" << std::endl;

			}
			else
			{
				convert(trig.name, trig.identifier, boost::make_optional(trig.body));
			}
		}

		void operator()(Treceiver const & trig) const
		{
			if(trig.name=="if")
			{
				if(trig.condition.is_initialized())
				{
					convertCondition(trig.condition.get());
				}
				else
				{
					(*out) << "if true then" << std::endl;
				}
			}
			else if(trig.name=="el")
			{
				(*out) << "else" << std::endl;
			}
			else if(trig.name=="en")
			{
				(*out) << "end" << std::endl;
			}
			else
			{
				if(trig.condition.is_initialized())
				{
					convertCondition(trig.condition.get());

					convert(trig.name, trig.identifier, trig.body);

					(*out) << "end" << std::endl;
				}
				else
				{
					convert(trig.name, trig.identifier, trig.body);
				}
			}


		}
	};

	struct Command : public Converter
	{
		Command(std::ostream * out_)
			: Converter(out_)
		{}

		void operator()(Tcommand const & cmd) const
		{
			boost::apply_visitor(ERMExp(out), cmd.cmd);
		}
		void operator()(std::string const & comment) const
		{
			(*out) << "-- " << comment << std::endl;
		}

		void operator()(spirit::unused_type const &) const
		{
		}
	};

	struct TLiteralEval : public boost::static_visitor<std::string>
	{

		std::string operator()(char const & val)
		{
			return std::to_string(val);
		}
		std::string operator()(double const & val)
		{
			return std::to_string(val);
		}
		std::string operator()(int const & val)
		{
			return std::to_string(val);
		}
		std::string operator()(std::string const & val)
		{
			return "[===[" + std::string(val) + "]===]";
		}
	};

	struct VOptionEval : public Converter
	{
		bool discardResult;

		VOptionEval(std::ostream * out_, bool discardResult_ = false)
			: Converter(out_),
			discardResult(discardResult_)
		{}

		void operator()(VNIL const & opt) const
		{
			if(!discardResult)
			{
				(*out) << "return ";
			}
			(*out) << "nil";
		}
		void operator()(VNode const & opt) const;

		void operator()(VSymbol const & opt) const
		{
			if(!discardResult)
			{
				(*out) << "return ";
			}
			(*out) << opt.text;
		}
		void operator()(TLiteral const & opt) const
		{
			if(!discardResult)
			{
				(*out) << "return ";
			}

			TLiteralEval tmp;

			(*out) << boost::apply_visitor(tmp, opt);
		}
		void operator()(ERM::Tcommand const & opt) const
		{
			//this is how FP works, evaluation == producing side effects
			//TODO: can we evaluate to smth more useful?
			boost::apply_visitor(ERMExp(out), opt.cmd);
		}
	};

	struct VOptionNodeEval : public Converter
	{
		bool discardResult;

		VNode & exp;

		VOptionNodeEval(std::ostream * out_, VNode & exp_, bool discardResult_ = false)
			: Converter(out_),
			discardResult(discardResult_),
			exp(exp_)
		{}

		void operator()(VNIL const & opt) const
		{
			throw EVermScriptExecError("Nil does not evaluate to a function");
		}

		void operator()(VNode const & opt) const
		{
			VNode tmpn(exp);

			(*out) << "local fu = ";

			VOptionEval tmp(out, true);
			tmp(opt);

			(*out) << std::endl;

			if(!discardResult)
				(*out) << "return ";

			(*out) << "fu(";

			VOptionList args = tmpn.children.cdr().getAsList();

			for(int g=0; g<args.size(); ++g)
			{
				if(g == 0)
				{
					boost::apply_visitor(VOptionEval(out, true), args[g]);
				}
				else
				{
					(*out) << ", ";
					boost::apply_visitor(VOptionEval(out, true), args[g]);
				}
			}

			(*out) << ")";
		}

		void operator()(VSymbol const & opt) const
		{
			std::map<std::string, std::string> symToOperator =
			{
				{"<", "<"},
				{"<=", "<="},
				{">", ">"},
				{">=", ">="},
				{"=", "=="},
				{"+", "+"},
				{"-", "-"},
				{"*", "*"},
				{"/", "/"},
				{"%", "%"}
			};

			if(false)
			{

			}
//			//check keywords
//			else if(opt.text == "quote")
//			{
//				if(exp.children.size() == 2)
//					return exp.children[1];
//				else
//					throw EVermScriptExecError("quote special form takes only one argument");
//			}
//			else if(opt.text == "backquote")
//			{
//				if(exp.children.size() == 2)
//					return boost::apply_visitor(_SbackquoteEval(interp), exp.children[1]);
//				else
//					throw EVermScriptExecError("backquote special form takes only one argument");
//
//			}
//			else if(opt.text == "car")
//			{
//				if(exp.children.size() != 2)
//					throw EVermScriptExecError("car special form takes only one argument");
//
//				auto & arg = exp.children[1];
//				VOption evaluated = interp->eval(arg);
//
//				return boost::apply_visitor(CarEval(interp), evaluated);
//			}
//			else if(opt.text == "cdr")
//			{
//				if(exp.children.size() != 2)
//					throw EVermScriptExecError("cdr special form takes only one argument");
//
//				auto & arg = exp.children[1];
//				VOption evaluated = interp->eval(arg);
//
//				return boost::apply_visitor(CdrEval(interp), evaluated);
//			}
			else if(opt.text == "if")
			{
				if(exp.children.size() > 4 || exp.children.size() < 3)
					throw EVermScriptExecError("if special form takes two or three arguments");

				(*out) << "if ";

				boost::apply_visitor(VOptionEval(out, true),  exp.children[1]);

				(*out) << " then" << std::endl;

				boost::apply_visitor(VOptionEval(out, discardResult),  exp.children[2]);

				if(exp.children.size() == 4)
				{
					(*out) << std::endl << "else" << std::endl;

					boost::apply_visitor(VOptionEval(out, discardResult),  exp.children[3]);
				}

				(*out)<< std::endl << "end" << std::endl;
			}
			else if(opt.text == "lambda")
			{
				if(exp.children.size() <= 2)
				{
					throw EVermScriptExecError("Too few arguments for lambda special form");
				}

				(*out) << " function(";

				VNode arglist = getAs<VNode>(exp.children[1]);

				for(int g=0; g<arglist.children.size(); ++g)
				{
					std::string argName = getAs<VSymbol>(arglist.children[g]).text;

					if(g == 0)
						(*out) << argName;
					else
						(*out) << ", " <<argName;
				}

				(*out) << ")" << std::endl;

				VOptionList body = exp.children.cdr().getAsCDR().getAsList();

				for(int g=0; g<body.size(); ++g)
				{
					if(g < body.size()-1)
						boost::apply_visitor(VOptionEval(out, true), body[g]);
					else
						boost::apply_visitor(VOptionEval(out, false), body[g]);
				}

				(*out)<< std::endl << "end";
			}
			else if(opt.text == "setq")
			{
				if(exp.children.size() != 3 && exp.children.size() != 4)
					throw EVermScriptExecError("setq special form takes 2 or 3 arguments");

				std::string name = getAs<VSymbol>(exp.children[1]).text;

				size_t valIndex = 2;

				if(exp.children.size() == 4)
				{
					TLiteral varIndexLit = getAs<TLiteral>(exp.children[2]);

					int varIndex = getAs<int>(varIndexLit);

					boost::format fmt("%s['%d']");
					fmt % name % varIndex;

					name = fmt.str();

					valIndex = 3;
				}

				(*out) << name << " = ";

				boost::apply_visitor(VOptionEval(out, true), exp.children[valIndex]);

				(*out) << std::endl;

				if(!discardResult)
					(*out) << "return " << name << std::endl;
			}
			else if(opt.text == ERMInterpreter::defunSymbol)
			{
				if(exp.children.size() < 4)
				{
					throw EVermScriptExecError("defun special form takes at least 3 arguments");
				}

				std::string name = getAs<VSymbol>(exp.children[1]).text;

				(*out) << std::endl << "local function " << name << " (";

				VNode arglist = getAs<VNode>(exp.children[2]);

				for(int g=0; g<arglist.children.size(); ++g)
				{
					std::string argName = getAs<VSymbol>(arglist.children[g]).text;

					if(g == 0)
						(*out) << argName;
					else
						(*out) << ", " <<argName;
				}

				(*out) << ")" << std::endl;

				VOptionList body = exp.children.cdr().getAsCDR().getAsCDR().getAsList();

				for(int g=0; g<body.size(); ++g)
				{
					if(g < body.size()-1)
						boost::apply_visitor(VOptionEval(out, true), body[g]);
					else
						boost::apply_visitor(VOptionEval(out, false), body[g]);
				}

				(*out)<< std::endl << "end";

				if(!discardResult)
				{
					(*out)  << std::endl << "return " << name;
				}

			}
			else if(opt.text == "defmacro")
			{
//				if(exp.children.size() < 4)
//				{
//					throw EVermScriptExecError("defmacro special form takes at least 3 arguments");
//				}
//				VFunc f(exp.children.cdr().getAsCDR().getAsCDR().getAsList(), true);
//				VNode arglist = getAs<VNode>(exp.children[2]);
//				for(int g=0; g<arglist.children.size(); ++g)
//				{
//					f.args.push_back(getAs<VSymbol>(arglist.children[g]));
//				}
//				env.localBind(getAs<VSymbol>(exp.children[1]).text, f);
//				return f;
			}
			else if(opt.text == "progn")
			{
				(*out)<< std::endl << "do" << std::endl;

				for(int g=1; g<exp.children.size(); ++g)
				{
					if(g < exp.children.size()-1)
						boost::apply_visitor(VOptionEval(out, true),  exp.children[g]);
					else
						boost::apply_visitor(VOptionEval(out, discardResult),  exp.children[g]);
				}
				(*out) << std::endl << "end" << std::endl;
			}
			else if(opt.text == "do") //evaluates second argument as long first evaluates to non-nil
			{
				if(exp.children.size() != 3)
				{
					throw EVermScriptExecError("do special form takes exactly 2 arguments");
				}

				(*out) << std::endl << "while ";

				boost::apply_visitor(VOptionEval(out, true), exp.children[1]);

				(*out) << " do" << std::endl;

				boost::apply_visitor(VOptionEval(out, true), exp.children[2]);

				(*out) << std::endl << "end" << std::endl;

			}
			//"apply" part of eval, a bit blurred in this implementation but this way it looks good too
			else if(symToOperator.find(opt.text) != symToOperator.end())
			{
				if(!discardResult)
					(*out) << "return ";

				std::string _operator = symToOperator[opt.text];

				VOptionList opts = exp.children.cdr().getAsList();

				for(int g=0; g<opts.size(); ++g)
				{
					if(g == 0)
					{
						boost::apply_visitor(VOptionEval(out, true), opts[g]);
					}
					else
					{
						(*out) << " " << _operator << " ";
						boost::apply_visitor(VOptionEval(out, true), opts[g]);
					}
				}
			}
			else
			{
				//assume callable
				if(!discardResult)
					(*out) << "return ";

				(*out) << opt.text << "(";

				VOptionList opts = exp.children.cdr().getAsList();

				for(int g=0; g<opts.size(); ++g)
				{
					if(g == 0)
					{
						boost::apply_visitor(VOptionEval(out, true), opts[g]);
					}
					else
					{
						(*out) << ", ";
						boost::apply_visitor(VOptionEval(out, true), opts[g]);
					}
				}

				(*out) << ")";
			}

		}
		void operator()(TLiteral const & opt) const
		{
			throw EVermScriptExecError("Literal does not evaluate to a function: "+boost::to_string(opt));
		}
		void operator()(ERM::Tcommand const & opt) const
		{
			throw EVermScriptExecError("ERM command does not evaluate to a function");
		}
	};

	void VOptionEval::operator()(VNode const& opt) const
	{
		if(!opt.children.empty())
		{
			VOption & car = const_cast<VNode&>(opt).children.car().getAsItem();

			boost::apply_visitor(VOptionNodeEval(out, const_cast<VNode&>(opt), discardResult), car);
		}
	}


	struct Line : public Converter
	{
		Line(std::ostream * out_)
			: Converter(out_)
		{}

		void operator()(TVExp const & cmd) const
		{
			//TODO:
			VNode line(cmd);

			VOptionEval eval(out, true);
			eval(line);

			(*out) << std::endl;
		}
		void operator()(TERMline const & cmd) const
		{
			boost::apply_visitor(Command(out), cmd);
		}
	};

	void convertInstructions(std::ostream & out, ERMInterpreter * owner)
	{
		out << "local function instructions()" << std::endl;

		Line lineConverter(&out);

		for(const LinePointer & lp : owner->instructions)
		{
			ERM::TLine & line = owner->retrieveLine(lp);

			boost::apply_visitor(lineConverter, line);
		}

		out << "end" << std::endl;
	}

	void convertFunctions(std::ostream & out, ERMInterpreter * owner, const std::vector<VERMInterpreter::Trigger> & triggers)
	{
		std::map<std::string, LinePointer> numToBody;

		Line lineConverter(&out);

		for(const VERMInterpreter::Trigger & trigger : triggers)
		{
			ERM::TLine & firstLine = owner->retrieveLine(trigger.line);

			const ERM::TTriggerBase & trig = ERMInterpreter::retrieveTrigger(firstLine);

			if(!trig.identifier.is_initialized())
				throw EInterpreterError("Function must have identifier");

			ERM::Tidentifier tid = trig.identifier.get();

			if(tid.size() == 0)
				throw EInterpreterError("Function must have identifier");

			std::string num = boost::apply_visitor(LVL1Iexp(), tid[0]);

			if(vstd::contains(numToBody, num))
				throw EInterpreterError("Function index duplicated: "+num);

			numToBody[num] = trigger.line;
		}

		for(const auto & p : numToBody)
		{
			std::string name = "FU"+p.first;

			out << name << " = function(x)" << std::endl;

			LinePointer lp = p.second;

			++lp;

			out << "local y = ERM.getY('" << name << "')" << std::endl;

			for(; lp.isValid(); ++lp)
			{
				ERM::TLine curLine = owner->retrieveLine(lp);
				if(owner->isATrigger(curLine))
					break;

				boost::apply_visitor(lineConverter, curLine);
			}

			out << "end" << std::endl;
		}
	}

	void convertTriggers(std::ostream & out, ERMInterpreter * owner, const VERMInterpreter::TriggerType & type, const std::vector<VERMInterpreter::Trigger> & triggers)
	{
		Line lineConverter(&out);

		for(const VERMInterpreter::Trigger & trigger : triggers)
		{
			ERM::TLine & firstLine = owner->retrieveLine(trigger.line);

			const ERM::TTriggerBase & trig = ERMInterpreter::retrieveTrigger(firstLine);

			//TODO: identifier
			//TODO: condition

			out << "ERM:addTrigger({" << std::endl;
			out << "\tname = '" << trig.name << "'," << std::endl;
			out << "\tfn = function ()" << std::endl;

			out << "local y = ERM.getY('" << trig.name  << "')" << std::endl;
			LinePointer lp = trigger.line;
			++lp;

			for(; lp.isValid(); ++lp)
			{
				ERM::TLine curLine = owner->retrieveLine(lp);
				if(owner->isATrigger(curLine))
					break;

				boost::apply_visitor(lineConverter, curLine);
			}

			out << "\tend," << std::endl;
			out << "})" << std::endl;
		}
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
			case 1: //instruction
				{
					interpreter->instructions.push_back(lp);
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
	for(auto p : scripts)
		boost::apply_visitor(ScriptScanner(this, p.first), p.second);
}

ERMInterpreter::ERMInterpreter(vstd::CLoggerBase * logger_)
	: logger(logger_)
{

}

ERMInterpreter::~ERMInterpreter()
{

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
	return false;
}

ERM::EVOtions ERMInterpreter::getExpType(const ERM::TVOption & opt)
{
	//MAINTENANCE: keep it correct!
	return static_cast<ERM::EVOtions>(opt.which());
}

bool ERMInterpreter::isCMDATrigger(const ERM::Tcommand & cmd)
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

ERM::TLine & ERMInterpreter::retrieveLine(const LinePointer & linePtr)
{
	return scripts.find(linePtr)->second;
}

ERM::TTriggerBase & ERMInterpreter::retrieveTrigger(ERM::TLine & line)
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

int ERMInterpreter::getRealLine(const LinePointer &lp)
{
	for(std::map<VERMInterpreter::LinePointer, ERM::TLine>::const_iterator i = scripts.begin(); i != scripts.end(); i++)
		if(i->first.lineNum == lp.lineNum && i->first.file->filename == lp.file->filename)
			return i->first.realLineNum;

	return -1;
}

const std::string ERMInterpreter::triggerSymbol = "trigger";
const std::string ERMInterpreter::postTriggerSymbol = "postTrigger";
const std::string ERMInterpreter::defunSymbol = "defun";

void ERMInterpreter::loadScript(const std::string & name, const std::string & source)
{
	ERMParser ep(source);
	FileInfo * finfo = new FileInfo();
	finfo->filename = name;

	std::vector<LineInfo> buf = ep.parseFile();
	finfo->length = buf.size();
	files.push_back(finfo);

	for(int g=0; g<buf.size(); ++g)
	{
		scripts[LinePointer(finfo, g, buf[g].realLineNum)] = buf[g].tl;
	}
}

std::string ERMInterpreter::convert()
{
	std::stringstream out;

	out << "local ERM = require(\"core:erm\")" << std::endl;

	out << "local v = ERM.v" << std::endl;
	out << "local z = ERM.z" << std::endl;
	out << "local flag = ERM.flag" << std::endl;
	out << "local quick = ERM.quick" << std::endl;

	ERMConverter::convertInstructions(out, this);

	for(const auto & p : triggers)
	{
		const VERMInterpreter::TriggerType & tt = p.first;

		if(tt.type == VERMInterpreter::TriggerType::FU)
		{
			ERMConverter::convertFunctions(out, this, p.second);
		}
		else
		{
			ERMConverter::convertTriggers(out, this, tt, p.second);
		}
	}

	for(const auto & p : postTriggers)
		;//TODO

	out << "ERM:callInstructions(instructions)" << std::endl;

	return out.str();
}

//struct _SbackquoteEval : boost::static_visitor<VOption>
//{
//	ERMInterpreter * interp;
//
//	_SbackquoteEval(ERMInterpreter * interp_)
//		: interp(interp_)
//	{}
//
//	VOption operator()(VNIL const& opt) const
//	{
//		return opt;
//	}
//	VOption operator()(VNode const& opt) const
//	{
//		VNode ret = opt;
//		if(opt.children.size() == 2)
//		{
//			VOption fo = opt.children[0];
//			if(isA<VSymbol>(fo))
//			{
//				if(getAs<VSymbol>(fo).text == "comma")
//				{
//					return interp->eval(opt.children[1]);
//				}
//			}
//		}
//		for(int g=0; g<opt.children.size(); ++g)
//		{
//			ret.children[g] = boost::apply_visitor(_SbackquoteEval(interp), ret.children[g]);
//		}
//		return ret;
//	}
//	VOption operator()(VSymbol const& opt) const
//	{
//		return opt;
//	}
//	VOption operator()(TLiteral const& opt) const
//	{
//		return opt;
//	}
//	VOption operator()(ERM::Tcommand const& opt) const
//	{
//		return opt;
//	}

//};


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
		processModifierList(exp.modifier, false);
	}

	VNode::VNode( const VOption & first, const VOptionList & rest ) /*merges given arguments into [a, rest] */
	{
		setVnode(first, rest);
	}

	VNode::VNode( const VOptionList & cdren ) : children(cdren)
	{}

	VNode::VNode( const ERM::TSymbol & sym )
	{
		children.car() = VSymbol(sym.sym);
		processModifierList(sym.symModifier, true);
	}

	void VNode::setVnode( const VOption & first, const VOptionList & rest )
	{
		children.car() = first;
		children.cdr() = rest;
	}

	void VNode::processModifierList( const std::vector<TVModifier> & modifierList, bool asSymbol )
	{
		for(int g=0; g<modifierList.size(); ++g)
		{
			if(asSymbol)
			{
				children.resize(children.size()+1);
				for(int i=children.size()-1; i >0; i--)
				{
					children[i] = children[i-1];
				}
			}
			else
			{
				children.cdr() = VNode(children);
			}

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
			if(parent->size() <= basePos)
				parent->push_back(opt);
			else
				(*parent)[basePos] = opt;
			break;
		case NORM:
			parent->resize(basePos+1);
			(*parent)[basePos] = opt;
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
		case NORM:
			parent->resize(basePos+1);
			parent->insert(parent->begin()+basePos, opt.begin(), opt.end());
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
			return (*parent)[basePos];
		else
			throw EInterpreterError("iterator is not in car state, cannot get as list");
	}
	VermTreeIterator VermTreeIterator::getAsCDR()
	{
		VermTreeIterator ret = *this;
		ret.basePos++;
		return ret;
	}
	VOption & VermTreeIterator::getIth( int i )
	{
		return (*parent)[basePos + i];
	}
	size_t VermTreeIterator::size() const
	{
		return parent->size() - basePos;
	}

	VERMInterpreter::VOptionList VermTreeIterator::getAsList()
	{
		VOptionList ret;
		for(int g = basePos; g<parent->size(); ++g)
		{
			ret.push_back((*parent)[g]);
		}
		return ret;
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
		ret.basePos = 1;
		return ret;
	}

	VermTreeIterator VOptionList::car()
	{
		VermTreeIterator ret(*this);
		ret.state = VermTreeIterator::CAR;
		return ret;
	}

}
