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

	static const std::map<std::string, std::string> CMP_OPERATION =
	{
		{"<", "<"},
		{">", ">"},
		{">=", ">="},
		{"=>", ">="},
		{"<=", "<="},
		{"=<", "<="},
		{"=", "=="},
		{"<>", "~="},
		{"><", "~="},
	};


	struct Variable
	{
		std::string name = "";
		std::string macro = "";
		int index = 0;

		Variable(const std::string & name_, int index_)
		{
			name = name_;
			index = index_;
		}

		Variable(const std::string & macro_)
		{
			macro = macro_;
		}

		bool isEmpty() const
		{
			return (name == "") && (macro == "");
		}

		bool isMacro() const
		{
			return (name == "") && (macro != "");
		}

		bool isSpecial() const
		{
			return (name.size() > 0) && (name[0] == 'd');
		}

		std::string str() const
		{
			if(isEmpty())
			{
				return std::to_string(index);
			}
			else if(isMacro())
			{
				return boost::to_string(boost::format("M['%s']") % macro);
			}
			else if(isSpecial() && (name.size() == 1))
			{
				boost::format fmt;
				fmt.parse("{'d', %d}");
				fmt % index;
				return fmt.str();
			}
			else if(isSpecial() && (name.size() != 1))
			{

				std::string ret;

				{
					boost::format fmt;

					if(index == 0)
					{
						fmt.parse("Q['%s']");
						fmt % name[name.size()-1];
					}
					else
					{
						fmt.parse("%s['%d']");
						fmt % name[name.size()-1] % index;
					}
					ret = fmt.str();
				}

				for(int i = ((int) name.size())-2; i > 0; i--)
				{
					boost::format fmt("%s[tostring(%s)]");

					fmt % name[i] % ret;

					ret = fmt.str();
				}

				{
					boost::format fmt;
					fmt.parse("{'d', %s}");
					fmt % ret;
					return fmt.str();
				}
			}
			else
			{
				std::string ret;
				{
					boost::format fmt;

					if(index == 0)
					{
						fmt.parse("Q['%s']");
						fmt % name[name.size()-1];
					}
					else
					{
						fmt.parse("%s['%d']");
						fmt % name[name.size()-1] % index;
					}
					ret = fmt.str();
				}

				for(int i = ((int) name.size())-2; i >= 0; i--)
				{
					boost::format fmt("%s[tostring(%s)]");

					fmt % name[i] % ret;

					ret = fmt.str();
				}

				return ret;
			}
		}
	};

	struct LVL2IexpToVar : boost::static_visitor<Variable>
	{
		LVL2IexpToVar() = default;

		Variable operator()(const TVarExpNotMacro & val) const
		{
			if(val.val.has_value())
				return Variable(val.varsym, val.val.get());
			else
				return Variable(val.varsym, 0);
		}

		Variable operator()(const TMacroUsage & val) const
		{
			return Variable(val.macro);
		}
	};

	struct LVL1IexpToVar : boost::static_visitor<Variable>
	{
		LVL1IexpToVar() = default;

		Variable operator()(int const & constant) const
		{
			return Variable("", constant);
		}

		Variable operator()(const TVarExp & var) const
		{
			return std::visit(LVL2IexpToVar(), var);
		}
	};

	struct Condition : public boost::static_visitor<std::string>
	{
		Condition()
		{}

		std::string operator()(const TComparison & cmp) const
		{
			Variable lhs = std::visit(LVL1IexpToVar(), cmp.lhs);
			Variable rhs = std::visit(LVL1IexpToVar(), cmp.rhs);

			auto sign = CMP_OPERATION.find(cmp.compSign);
			if(sign == std::end(CMP_OPERATION))
				throw EScriptExecError(std::string("Wrong comparison sign: ") + cmp.compSign);

			boost::format fmt("(%s %s %s)");
			fmt % lhs.str() % sign->second % rhs.str();
			return fmt.str();
		}
		std::string operator()(int const & flag) const
		{
			return boost::to_string(boost::format("F['%d']") % flag);
		}
	};

	struct ParamIO
	{
		ParamIO() = default;
		std::string name = "";
		bool isInput = false;
		bool semi = false;
		std::string semiCmpSign = "";
	};

	struct Converter : public boost::static_visitor<>
	{
		mutable std::ostream * out;
		Converter(std::ostream * out_)
			: out(out_)
		{}
	protected:

		void put(const std::string & text) const
		{
			(*out) << text;
		}

		void putLine(const std::string & line) const
		{
			(*out) << line << std::endl;
		}

		void endLine() const
		{
			(*out) << std::endl;
		}
	};

	struct GetBodyOption : public boost::static_visitor<std::string>
	{
		GetBodyOption()
		{}

		virtual std::string operator()(const TVarConcatString & cmp) const
		{
			throw EScriptExecError("String concatenation not allowed in this receiver");
		}
		virtual std::string operator()(const TStringConstant & cmp) const
		{
			throw EScriptExecError("String constant not allowed in this receiver");
		}
		virtual std::string operator()(const TCurriedString & cmp) const
		{
			throw EScriptExecError("Curried string not allowed in this receiver");
		}
		virtual std::string operator()(const TSemiCompare & cmp) const
		{
			throw EScriptExecError("Semi comparison not allowed in this receiver");
		}
		virtual std::string operator()(const TMacroDef & cmp) const
		{
			throw EScriptExecError("Macro definition not allowed in this receiver");
		}
		virtual std::string operator()(const TIexp & cmp) const
		{
			throw EScriptExecError("i-expression not allowed in this receiver");
		}
		virtual std::string operator()(const TVarpExp & cmp) const
		{
			throw EScriptExecError("Varp expression not allowed in this receiver");
		}
		virtual std::string operator()(const spirit::unused_type & cmp) const
		{
			throw EScriptExecError("\'Nothing\' not allowed in this receiver");
		}
	};

	struct BodyOption : public boost::static_visitor<ParamIO>
	{
		ParamIO operator()(const TVarConcatString & cmp) const
		{
			throw EScriptExecError(std::string("String concatenation not allowed in this receiver|")+cmp.string.str+"|");
		}

		ParamIO operator()(const TStringConstant & cmp) const
		{
			boost::format fmt("[===[%s]===]");
			fmt % cmp.str;

			ParamIO ret;
			ret.isInput = true;
			ret.name = fmt.str();
			return ret;
		}

		ParamIO operator()(const TCurriedString & cmp) const
		{
			throw EScriptExecError("Curried string not allowed in this receiver");
		}

		ParamIO operator()(const TSemiCompare & cmp) const
		{
			ParamIO ret;
			ret.isInput = false;
			ret.semi = true;
			ret.semiCmpSign = cmp.compSign;
			ret.name = (std::visit(LVL1IexpToVar(), cmp.rhs)).str();
			return ret;
		}

		ParamIO operator()(const TMacroDef & cmp) const
		{
			throw EScriptExecError("Macro definition not allowed in this receiver");
		}

		ParamIO operator()(const TIexp & cmp) const
		{
			ParamIO ret;
			ret.isInput = true;
			ret.name = (std::visit(LVL1IexpToVar(), cmp)).str();;
			return ret;
		}

		ParamIO operator()(const TVarpExp & cmp) const
		{
			ParamIO ret;
			ret.isInput = false;

			ret.name = (std::visit(LVL2IexpToVar(), cmp.var)).str();
			return ret;
		}

		ParamIO operator()(const spirit::unused_type & cmp) const
		{
			throw EScriptExecError("\'Nothing\' not allowed in this receiver");
		}
	};

	struct Receiver : public Converter
	{
		Receiver(std::ostream * out_)
			: Converter(out_)
		{}

		virtual void operator()(const TVRLogic & trig) const
		{
			throw EInterpreterError("VR logic is not allowed in this receiver!");
		}

		virtual void operator()(const TVRArithmetic & trig) const
		{
			throw EInterpreterError("VR arithmetic is not allowed in this receiver!");
		}

		virtual void operator()(const TNormalBodyOption & trig) const
		{
			throw EInterpreterError("Normal body is not allowed in this receiver!");
		}

	};

	struct GenericReceiver : public Receiver
	{
		std::string name;
		bool specialSemiCompare = false;

		GenericReceiver(std::ostream * out_, const std::string & name_, bool specialSemiCompare_)
			: Receiver(out_),
			name(name_),
			specialSemiCompare(specialSemiCompare_)
		{}

		using Receiver::operator();

		void operator()(const TNormalBodyOption & trig) const override
		{
			std::string outParams;
			std::string inParams;

			std::string semiCompareDecl;

			std::vector<std::string> compares;

			bool hasOutput = false;
			bool hasSemiCompare = false;

			std::vector<ParamIO> optionParams;

			if(trig.params.has_value())
			{
				for(auto & p : trig.params.get())
					optionParams.push_back(std::visit(BodyOption(), p));
			}

			int idx = 1;
			int fidx = 1;

			for(const ParamIO & p : optionParams)
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
				else if(p.semi)
				{
					hasOutput = true;
					hasSemiCompare = true;

					std::string tempVar = std::string("s")+std::to_string(idx);

					if(semiCompareDecl.empty())
					{
						semiCompareDecl = "local "+tempVar;
					}
					else
					{
						semiCompareDecl += ", ";
						semiCompareDecl += tempVar;
					}

					if(outParams.empty())
					{
						outParams = tempVar;
					}
					else
					{
						outParams += ", ";
						outParams += tempVar;
					}

					inParams += ", nil";


					auto sign = CMP_OPERATION.find(p.semiCmpSign);
					if(sign == std::end(CMP_OPERATION))
						throw EScriptExecError(std::string("Wrong comparison sign: ") + p.semiCmpSign);

					boost::format cmpFmt("F[%d] = (%s %s %s)");
					cmpFmt % fidx % p.name % sign->second % tempVar;
					compares.push_back(cmpFmt.str());

					fidx++;
				}
				else
				{
					hasOutput = true;

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

				idx++;
			}

			if(hasSemiCompare)
			{
				putLine(semiCompareDecl);
			}

			boost::format callFormat;

			if(hasOutput)
			{
				callFormat.parse("%s = %s:%s(x%s)");
				callFormat % outParams;
			}
			else
			{
				callFormat.parse("%s:%s(x%s)");
			}

			callFormat % name;
			callFormat % trig.optionCode;
			callFormat % inParams;

			putLine(callFormat.str());

			for(auto & str : compares)
				putLine(str);
		}
	};

	struct FU : public Receiver
	{
		Variable v;

		FU(std::ostream * out_, const ERM::TIexp & tid)
			: Receiver(out_),
			v(std::visit(LVL1IexpToVar(), tid))
		{
		}

		FU(std::ostream * out_)
			: Receiver(out_),
			v("", 0)
		{
		}

		using Receiver::operator();

		void operator()(const TNormalBodyOption & trig) const override
		{
			switch(trig.optionCode)
			{
			case 'E':
				{
					putLine("do return end");
				}
				break;
			default:
				throw EInterpreterError("Unknown opcode in FU receiver");
				break;
			}
		}
	};

	struct MC_S : public GetBodyOption
	{
		MC_S()
		{}

		using GetBodyOption::operator();

		std::string operator()(const TMacroDef & cmp) const override
		{
			return cmp.macro;
		}
	};

	struct MC : public Receiver
	{
		Variable v;

		MC(std::ostream * out_, const ERM::TIexp & tid)
			: Receiver(out_),
			v(std::visit(LVL1IexpToVar(), tid))
		{
		}

		MC(std::ostream * out_)
			: Receiver(out_),
			v("", 0)
		{
		}

		using Receiver::operator();

		void operator()(const TNormalBodyOption & option) const override
		{
			switch(option.optionCode)
			{
			case 'S':
				{
					if(option.params.has_value())
					{
						for(auto & p : option.params.get())
						{
							std::string macroName = std::visit(MC_S(), p);

							boost::format callFormat;

							if(v.isEmpty())
							{
								callFormat.parse("ERM:addMacro('%s', 'v', '%s')");
								callFormat % macroName % macroName;
							}
							else
							{
								callFormat.parse("ERM:addMacro('%s', '%s', '%d')");
								callFormat % macroName % v.name % v.index;
							}

							putLine(callFormat.str());
						}
					}
				}
				break;
			default:
				throw EInterpreterError("Unknown opcode in MC receiver");
				break;
			}
		}
	};

	struct VR_S : public GetBodyOption
	{
		VR_S()
		{}

		using GetBodyOption::operator();

		std::string operator()(const TIexp & cmp) const override
		{
			auto v = std::visit(LVL1IexpToVar(), cmp);
			return v.str();
		}
		std::string operator()(const TStringConstant & cmp) const override
		{
			boost::format fmt("[===[%s]===]");
			fmt % cmp.str;
			return fmt.str();
		}
	};

	struct VR_H : public GetBodyOption
	{
		VR_H()
		{}

		using GetBodyOption::operator();

		std::string operator()(const TIexp & cmp) const override
		{
			Variable p = std::visit(LVL1IexpToVar(), cmp);

			if(p.index <= 0)
				throw EScriptExecError("VR:H requires flag index");

			if(p.name != "")
				throw EScriptExecError("VR:H accept only flag index");


			boost::format fmt("'%d'");
			fmt % p.index;
			return fmt.str();
		}
	};

	struct VR_X : public GetBodyOption
	{
		VR_X()
		{
		}

		using GetBodyOption::operator();

		std::string operator()(const TIexp & cmp) const override
		{
			Variable p = std::visit(LVL1IexpToVar(), cmp);

			return p.str();
		}
	};

	struct VR : public Receiver
	{
		Variable v;

		VR(std::ostream * out_, const ERM::TIexp & tid)
			: Receiver(out_),
			v(std::visit(LVL1IexpToVar(), tid))
		{
		}

		using Receiver::operator();

		void operator()(const TVRLogic & trig) const override
		{
			Variable rhs = std::visit(LVL1IexpToVar(), trig.var);

			std::string opcode;

			switch (trig.opcode)
			{
			case '&':
				opcode = "bit.band";
				break;
			case '|':
				opcode = "bit.bor";
				break;
			default:
				throw EInterpreterError("Wrong opcode in VR logic expression!");
				break;
			}

			boost::format fmt("%s = %s(%s, %s)");
			fmt % v.str() % opcode % v.str() % rhs.str();
			putLine(fmt.str());
		}

		void operator()(const TVRArithmetic & trig) const override
		{
			Variable rhs = std::visit(LVL1IexpToVar(), trig.rhs);

			std::string opcode;

			switch (trig.opcode)
			{
			case '+':
				opcode = v.name[0] == 'z' ? ".." : "+";
				break;
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
			fmt % v.str() % v.str() % opcode % rhs.str();
			putLine(fmt.str());
		}

		void operator()(const TNormalBodyOption & trig) const override
		{
			switch(trig.optionCode)
			{
			case 'C': //setting/checking v vars
				{
					if(v.index <= 0)
						throw EScriptExecError("VR:C requires indexed variable");

					std::vector<ParamIO> optionParams;

					if(trig.params.has_value())
					{
						for(auto & p : trig.params.get())
							optionParams.push_back(std::visit(BodyOption(), p));
					}

					auto index = v.index;

					for(auto & p : optionParams)
					{
						boost::format fmt;
						if(p.isInput)
							fmt.parse("%s['%d'] = %s") % v.name % index % p.name;
						else
							fmt.parse("%s = %s['%d']") % p.name % v.name % index;
						putLine(fmt.str());
						index++;
					}
				}
				break;
			case 'H': //checking if string is empty
				{
					if(!trig.params.has_value() || trig.params.get().size() != 1)
						throw EScriptExecError("VR:H option takes exactly 1 parameter!");

					std::string opt = std::visit(VR_H(), trig.params.get()[0]);
					boost::format fmt("ERM.VR(%s):H(%s)");
					fmt % v.str() % opt;
					putLine(fmt.str());
				}
				break;
			case 'U':
				{
					if(!trig.params.has_value() || trig.params.get().size() != 1)
						throw EScriptExecError("VR:H/U need 1 parameter!");

					std::string opt = std::visit(VR_S(), trig.params.get()[0]);
					boost::format fmt("ERM.VR(%s):%c(%s)");
					fmt % v.str() % (trig.optionCode) % opt;
					putLine(fmt.str());
				}
				break;
			case 'M': //string operations
				{
					if(!trig.params.has_value() || trig.params.get().size() < 2)
						throw EScriptExecError("VR:M needs at least 2 parameters!");

					std::string opt = std::visit(VR_X(), trig.params.get()[0]);
					int paramIndex = 1;

					if(opt == "3")
					{
						boost::format fmt("%s = ERM.VR(%s):M3(");
						fmt % v.str() % v.str();
						put(fmt.str());
					}
					else
					{
						auto target = std::visit(VR_X(), trig.params.get()[paramIndex++]);

						boost::format fmt("%s = ERM.VR(%s):M%s(");
						fmt % target % v.str() % opt;
						put(fmt.str());
					}
					
					for(int i = paramIndex; i < trig.params.get().size(); i++)
					{
						opt = std::visit(VR_X(), trig.params.get()[i]);
						if(i > paramIndex) put(",");
						put(opt);
					}
					
					putLine(")");
				}
				break;
			case 'X': //bit xor
				{
					if(!trig.params.has_value() || trig.params.get().size() != 1)
						throw EScriptExecError("VR:X option takes exactly 1 parameter!");

					std::string opt = std::visit(VR_X(), trig.params.get()[0]);

					boost::format fmt("%s = bit.bxor(%s, %s)");
					fmt % v.str() % v.str() % opt;putLine(fmt.str());
				}
				break;
			case 'R': //random variables
				{
					//TODO
					putLine("--VR:R not implemented");
				}
				break;
			case 'S': //setting variable
				{
					if(!trig.params.has_value() || trig.params.get().size() != 1)
						throw EScriptExecError("VR:S option takes exactly 1 parameter!");

					std::string opt = std::visit(VR_S(), trig.params.get()[0]);
					put(v.str());
					put(" = ");
					put(opt);
					endLine();
				}
				break;
			case 'T': //random variables
				{
					//TODO
					putLine("--VR:T not implemented");
				}
				break;
			case 'V': //convert string to value
				{
					if(!trig.params.has_value() || trig.params.get().size() != 1)
						throw EScriptExecError("VR:V option takes exactly 1 parameter!");

					std::string opt = std::visit(VR_X(), trig.params.get()[0]);
					boost::format fmt("%s = tostring(%s)");
					fmt % v.str() % opt;
					putLine(fmt.str());
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
		void performBody(const std::optional<ERM::Tbody> & body, const Visitor & visitor) const
		{
			if(body.has_value())
			{
				const ERM::Tbody & bo = body.get();
				for(int g=0; g<bo.size(); ++g)
				{
					std::visit(visitor, bo[g]);
				}
			}
		}

		void convert(const std::string & name, const std::optional<Tidentifier> & identifier, const std::optional<Tbody> & body) const
		{
			if(name == "VR")
			{
				if(!identifier.has_value())
					throw EScriptExecError("VR receiver requires arguments");

				ERM::Tidentifier tid = identifier.value();
				if(tid.size() != 1)
					throw EScriptExecError("VR receiver takes exactly 1 argument");

				performBody(body, VR(out, tid[0]));
			}
			else if(name == "re")
			{
				if(!identifier.has_value())
					throw EScriptExecError("re receiver requires arguments");

				ERM::Tidentifier tid = identifier.value();

				auto argc = tid.size();

				if(argc > 0)
				{
					std::string loopCounter = (std::visit(LVL1IexpToVar(), tid.at(0))).str();

					std::string startVal = argc > 1 ? (std::visit(LVL1IexpToVar(), tid.at(1))).str() : loopCounter;
					std::string stopVal = argc > 2 ? (std::visit(LVL1IexpToVar(), tid.at(2))).str() : loopCounter;
					std::string increment = argc > 3 ? (std::visit(LVL1IexpToVar(), tid.at(3))).str() : "1";

					boost::format fmt("for __iter = %s, %s, %s do");


					fmt % startVal % stopVal % increment;
					putLine(fmt.str());
					fmt.parse("%s = __iter");
					fmt % loopCounter;
					putLine(fmt.str());
				}
				else
				{
					throw EScriptExecError("re receiver requires arguments");
				}
			}
			else if(name == "FU" && !identifier.has_value())
			{
				performBody(body, FU(out)); //assume FU:E
			}
			else if(name == "MC")
			{
				if(identifier.has_value())
				{
					ERM::Tidentifier tid = identifier.value();
					if(tid.size() != 1)
						throw EScriptExecError("MC receiver takes no more than 1 argument");

					performBody(body, MC(out, tid[0]));
				}
				else
				{
					performBody(body, MC(out));
				}
			}
			else
			{
				std::vector<std::string> identifiers;

				if(identifier.has_value())
				{
					for(const auto & id : identifier.value())
					{
						Variable v = std::visit(LVL1IexpToVar(), id);

						if(v.isSpecial())
							throw ELineProblem("Special variable syntax ('d') is not allowed in receiver identifier");
						identifiers.push_back(v.str());
					}
				}

				std::string params;

				for(auto iter = std::begin(identifiers); iter != std::end(identifiers); ++iter)
				{
					if(!params.empty())
						params += ", ";
					params += *iter;
				}

				if(body.has_value())
				{
					const ERM::Tbody & bo = body.get();
					if(bo.size() == 1)
					{
						boost::format fmt("ERM.%s(%s)");
						fmt % name;
						fmt % params;

						GenericReceiver gr(out, fmt.str(), (name == "DO"));
						bo[0].apply_visitor(gr);
					}
					else
					{
						putLine("do");
						boost::format fmt("local %s = ERM.%s(%s)");
						fmt % name;
						fmt % name;
						fmt % params;

						putLine(fmt.str());

						performBody(body, GenericReceiver(out, name, (name=="DO") ));

						putLine("end");
					}
				}
				else
				{
					//is it an error?
					logMod->warn("ERM receiver '%s %s' w/o body", name, params);
				}


			}
		}

		void convertConditionInner(const Tcondition & cond, char op) const
		{
			std::string lhs = std::visit(Condition(), cond.cond);

			if(cond.ctype != '/')
				op = cond.ctype;

			switch (op)
			{
			case '&':
				put(" and ");
				break;
			case '|':
				put(" or ");
				break;
			default:
				throw EInterpreterProblem(std::string("Wrong condition connection (") + cond.ctype + ") !");
				break;
			}

			put(lhs);

			if(cond.rhs.has_value())
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

		void convertCondition(const Tcondition & cond) const
		{
			//&c1/c2/c3|c4/c5/c6 -> (c1  & c2  & c3)  | c4  |  c5  | c6
			std::string lhs = std::visit(Condition(), cond.cond);
			put("if ");
			put(lhs);

			if(cond.rhs.has_value())
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

			putLine(" then ");
		}

		void convertReceiverOrInstruction(const std::optional<Tcondition> & condition,
			const std::string & name,
			const std::optional<Tidentifier> & identifier,
			const std::optional<Tbody> & body) const
		{
			if(name=="if")
			{
				if(condition.has_value())
					convertCondition(condition.get());
				else
					putLine("if true then");
			}
			else if(name=="el")
			{
				putLine("else");
			}
			else if(name=="en")
			{
				putLine("end");
			}
			else
			{
				if(condition.has_value())
				{
					convertCondition(condition.get());
					convert(name, identifier, body);
					putLine("end");
				}
				else
				{
					convert(name, identifier, body);
				}
			}
		}

		void operator()(const Ttrigger & trig) const
		{
			throw EInterpreterError("Triggers cannot be executed!");
		}

		void operator()(const TPostTrigger & trig) const
		{
			throw EInterpreterError("Post-triggers cannot be executed!");
		}

		void operator()(const Tinstruction & trig) const
		{
			convertReceiverOrInstruction(trig.condition, trig.name, trig.identifier, std::make_optional(trig.body));
		}

		void operator()(const Treceiver & trig) const
		{
			convertReceiverOrInstruction(trig.condition, trig.name, trig.identifier, trig.body);
		}
	};

	struct Command : public Converter
	{
		Command(std::ostream * out_)
			: Converter(out_)
		{}

		void operator()(const Tcommand & cmd) const
		{
			std::visit(ERMExp(out), cmd.cmd);
		}
		void operator()(const std::string & comment) const
		{
			(*out) << "-- " << comment;
			endLine();
		}

		void operator()(spirit::unused_type const &) const
		{
		}
	};

	struct TLiteralEval : public boost::static_visitor<std::string>
	{

		std::string operator()(char const & val)
		{
			return "{\"'\",'"+ std::to_string(val) +"'}";
		}
		std::string operator()(double const & val)
		{
			return std::to_string(val);
		}
		std::string operator()(int const & val)
		{
			return std::to_string(val);
		}
		std::string operator()(const std::string & val)
		{
			return "{\"'\",[===[" + val + "]===]}";
		}
	};

	struct VOptionEval : public Converter
	{
		VOptionEval(std::ostream * out_)
			: Converter(out_)
		{}

		void operator()(VNIL const & opt) const
		{
			(*out) << "{}";
		}
		void operator()(VNode const & opt) const;

		void operator()(VSymbol const & opt) const
		{
			(*out) << "\"" << opt.text << "\"";
		}
		void operator()(TLiteral const & opt) const
		{
			TLiteralEval tmp;
			(*out) << std::visit(tmp, opt);
		}
		void operator()(ERM::Tcommand const & opt) const
		{
			//this is how FP works, evaluation == producing side effects
			//TODO: can we evaluate to smth more useful?
			//???
			throw EVermScriptExecError("Using ERM options in VERM expression is not (yet) allowed");
//			std::visit(ERMExp(out), opt.cmd);
		}
	};

	void VOptionEval::operator()(VNode const& opt) const
	{
		VNode tmpn(opt);

		(*out) << "{";

		for(VOption & op : tmpn.children)
		{
			std::visit(VOptionEval(out), op);
			(*out) << ",";
		}
		(*out) << "}";
	}

	struct Line : public Converter
	{
		Line(std::ostream * out_)
			: Converter(out_)
		{}

		void operator()(TVExp const & cmd) const
		{
			put("VERM:E");

			VNode line(cmd);

			VOptionEval eval(out);
			eval(line);


			endLine();
		}
		void operator()(TERMline const & cmd) const
		{
			std::visit(Command(out), cmd);
		}
	};

	void convertInstructions(std::ostream & out, ERMInterpreter * owner)
	{
		out << "local function instructions()" << std::endl;
		out << "local e, x, y = {}, {}, {}" << std::endl;

		Line lineConverter(&out);

		for(const LinePointer & lp : owner->instructions)
		{
			ERM::TLine & line = owner->retrieveLine(lp);

			std::visit(lineConverter, line);
		}

		out << "end" << std::endl;
	}

	void convertFunctions(std::ostream & out, ERMInterpreter * owner, const std::vector<VERMInterpreter::Trigger> & triggers)
	{
		Line lineConverter(&out);

		for(const VERMInterpreter::Trigger & trigger : triggers)
		{
			ERM::TLine & firstLine = owner->retrieveLine(trigger.line);

			const ERM::TTriggerBase & trig = ERMInterpreter::retrieveTrigger(firstLine);

			//TODO: condition

			out << "ERM:addTrigger({" << std::endl;

			if(!trig.identifier.has_value())
				throw EInterpreterError("Function must have identifier");

			ERM::Tidentifier tid = trig.identifier.value();

			if(tid.empty())
				throw EInterpreterError("Function must have identifier");

			Variable v = std::visit(LVL1IexpToVar(), tid[0]);

			if(v.isSpecial())
				throw ELineProblem("Special variable syntax ('d') is not allowed in function definition");

			out << "id = {" << v.str() << "}," << std::endl;
			out << "name = 'FU'," << std::endl;
			out << "fn = function (e, y, x)" << std::endl;
			out << "local _" << std::endl;

			LinePointer lp = trigger.line;
			++lp;

			for(; lp.isValid(); ++lp)
			{
				ERM::TLine curLine = owner->retrieveLine(lp);
				if(owner->isATrigger(curLine))
					break;

				std::visit(lineConverter, curLine);
			}

			out << "end," << std::endl;
			out << "})" << std::endl;
		}
	}

	void convertTriggers(std::ostream & out, ERMInterpreter * owner, const VERMInterpreter::TriggerType & type, const std::vector<VERMInterpreter::Trigger> & triggers)
	{
		Line lineConverter(&out);

		for(const VERMInterpreter::Trigger & trigger : triggers)
		{
			ERM::TLine & firstLine = owner->retrieveLine(trigger.line);

			const ERM::TTriggerBase & trig = ERMInterpreter::retrieveTrigger(firstLine);

			//TODO: condition

			out << "ERM:addTrigger({" << std::endl;

			std::vector<std::string> identifiers;

			if(trig.identifier.has_value())
			{
				for(const auto & id : trig.identifier.value())
				{
					Variable v = std::visit(LVL1IexpToVar(), id);

					if(v.isSpecial())
						throw ELineProblem("Special variable syntax ('d') is not allowed in trigger definition");
					identifiers.push_back(v.str());
				}
			}

			out << "id = {";
			for(const auto & id : identifiers)
				out << "'" << id << "',";
			out << "}," << std::endl;

			out << "name = '" << trig.name << "'," << std::endl;
			out << "fn = function (e, y)" << std::endl;
			out << "local _" << std::endl;

			LinePointer lp = trigger.line;
			++lp;

			for(; lp.isValid(); ++lp)
			{
				ERM::TLine curLine = owner->retrieveLine(lp);
				if(owner->isATrigger(curLine))
					break;

				std::visit(lineConverter, curLine);
			}

			out << "end," << std::endl;
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
			Tcommand tcmd = std::get<Tcommand>(cmd);
			switch (tcmd.cmd.which())
			{
			case 0: //trigger
				{
					Trigger trig;
					trig.line = lp;
					interpreter->triggers[ TriggerType(std::get<ERM::Ttrigger>(tcmd.cmd).name) ].push_back(trig);
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
					interpreter->postTriggers[ TriggerType(std::get<ERM::TPostTrigger>(tcmd.cmd).name) ].push_back(trig);
				}
				break;
			default:
				break;
			}
		}

	}
};


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
			TVExp vexp = std::get<TVExp>(line);
			if(vexp.children.empty())
				return false;

			switch (getExpType(vexp.children[0]))
			{
			case SYMBOL:
				return false;
				break;
			case TCMD:
				return isCMDATrigger( std::get<ERM::Tcommand>(vexp.children[0]) );
				break;
			default:
				return false;
				break;
			}
		}
		break;
	case 1: //erm
		{
			TERMline ermline = std::get<TERMline>(line);
			switch(ermline.which())
			{
			case 0: //tcmd
				return isCMDATrigger( std::get<ERM::Tcommand>(ermline) );
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
		ERM::TERMline &tl = std::get<ERM::TERMline>(line);
		if(tl.which() == 0)
		{
			ERM::Tcommand &tcm = std::get<ERM::Tcommand>(tl);
			if(tcm.cmd.which() == 0)
			{
				return std::get<ERM::Ttrigger>(tcm.cmd);
			}
			else if(tcm.cmd.which() == 3)
			{
				return std::get<ERM::TPostTrigger>(tcm.cmd);
			}
			throw ELineProblem("Given line is not a trigger!");
		}
		throw ELineProblem("Given line is not a command!");
	}
	throw ELineProblem("Given line is not an ERM trigger!");
}

std::string ERMInterpreter::loadScript(const std::string & name, const std::string & source)
{
	CERMPreprocessor preproc(source);

	const bool isVERM = preproc.version == CERMPreprocessor::Version::VERM;

	ERMParser ep;

	std::vector<LineInfo> buf = ep.parseFile(preproc);

	for(int g=0; g<buf.size(); ++g)
		scripts[LinePointer(static_cast<int>(buf.size()), g, buf[g].realLineNum)] = buf[g].tl;

	for(auto p : scripts)
		std::visit(ScriptScanner(this, p.first), p.second);

	std::stringstream out;

	out << "local ERM = require(\"core:erm\")" << std::endl;

	if(isVERM)
	{
		out << "local VERM = require(\"core:verm\")" << std::endl;
	}

	out << "local _" << std::endl;
	out << "local v, w, z, F, M, Q = ERM.v, ERM.w, ERM.z, ERM.F, ERM.M, ERM.Q" << std::endl;

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
		;//TODO:postTriggers

	out << "ERM:callInstructions(instructions)" << std::endl;

	return out.str();
}

namespace VERMInterpreter
{
	VOption convertToVOption(const ERM::TVOption & tvo)
	{
		return std::visit(OptionConverterVisitor(), tvo);
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
				for(auto i=children.size()-1; i >0; i--)
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
				children.car() = VSymbol("`");
			}
			else if(modifierList[g] == ",!")
			{
				children.car() = VSymbol("comma-unlist");
			}
			else if(modifierList[g] == ",")
			{
				children.car() = VSymbol(",");
			}
			else if(modifierList[g] == "#'")
			{
				children.car() = VSymbol("get-func");
			}
			else if(modifierList[g] == "'")
			{
				children.car() = VSymbol("'");
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
		if(cmd.symModifier.empty())
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
