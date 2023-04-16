/*
 * LogicalExpression.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

//FIXME: move some of code into .cpp to avoid this include?
#include "JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace LogicalExpressionDetail
{
	/// class that defines required types for logical expressions
	template<typename ContainedClass>
	class ExpressionBase
	{
	public:
		/// Possible logical operations, mostly needed to create different types for std::variant
		enum EOperations
		{
			ANY_OF,
			ALL_OF,
			NONE_OF
		};
		template<EOperations tag> class Element;

		typedef Element<ANY_OF> OperatorAny;
		typedef Element<ALL_OF> OperatorAll;
		typedef Element<NONE_OF> OperatorNone;

		typedef ContainedClass Value;

		/// Variant that contains all possible elements from logical expression
		typedef std::variant<
			OperatorAll,
			OperatorAny,
			OperatorNone,
			Value
			> Variant;

		/// Variant element, contains list of expressions to which operation "tag" should be applied
		template<EOperations tag>
		class Element
		{
		public:
			Element() {}
			Element(std::vector<Variant> expressions):
				expressions(expressions)
			{}

			std::vector<Variant> expressions;

			bool operator == (const Element & other) const
			{
				return expressions == other.expressions;
			}

			template <typename Handler>
			void serialize(Handler & h, const int version)
			{
				h & expressions;
			}
		};
	};

	/// Visitor to test result (true/false) of the expression
	template<typename ContainedClass>
	class TestVisitor
	{
		typedef ExpressionBase<ContainedClass> Base;

		std::function<bool(const typename Base::Value &)> classTest;

		size_t countPassed(const std::vector<typename Base::Variant> & element) const
		{
			return boost::range::count_if(element, [&](const typename Base::Variant & expr)
			{
				return std::visit(*this, expr);
			});
		}
	public:
		TestVisitor(std::function<bool (const typename Base::Value &)> classTest):
			classTest(classTest)
		{}

		bool operator()(const typename Base::OperatorAny & element) const
		{
			return countPassed(element.expressions) != 0;
		}

		bool operator()(const typename Base::OperatorAll & element) const
		{
			return countPassed(element.expressions) == element.expressions.size();
		}

		bool operator()(const typename Base::OperatorNone & element) const
		{
			return countPassed(element.expressions) == 0;
		}

		bool operator()(const typename Base::Value & element) const
		{
			return classTest(element);
		}
	};

	template <typename ContainedClass>
	class SatisfiabilityVisitor;

	template <typename ContainedClass>
	class FalsifiabilityVisitor;

	template<typename ContainedClass>
	class PossibilityVisitor
	{
		typedef ExpressionBase<ContainedClass> Base;

	protected:
		std::function<bool(const typename Base::Value &)> satisfiabilityTest;
		std::function<bool(const typename Base::Value &)> falsifiabilityTest;
		SatisfiabilityVisitor<ContainedClass> *satisfiabilityVisitor;
		FalsifiabilityVisitor<ContainedClass> *falsifiabilityVisitor;

		size_t countSatisfiable(const std::vector<typename Base::Variant> & element) const
		{
			return boost::range::count_if(element, [&](const typename Base::Variant & expr)
			{
				return std::visit(*satisfiabilityVisitor, expr);
			});
		}

		size_t countFalsifiable(const std::vector<typename Base::Variant> & element) const
		{
			return boost::range::count_if(element, [&](const typename Base::Variant & expr)
			{
				return std::visit(*falsifiabilityVisitor, expr);
			});
		}

	public:
		PossibilityVisitor(std::function<bool (const typename Base::Value &)> satisfiabilityTest,
		                   std::function<bool (const typename Base::Value &)> falsifiabilityTest):
			satisfiabilityTest(satisfiabilityTest),
			falsifiabilityTest(falsifiabilityTest),
			satisfiabilityVisitor(nullptr),
			falsifiabilityVisitor(nullptr)
		{}

		void setSatisfiabilityVisitor(SatisfiabilityVisitor<ContainedClass> *satisfiabilityVisitor)
		{
			this->satisfiabilityVisitor = satisfiabilityVisitor;
		}

		void setFalsifiabilityVisitor(FalsifiabilityVisitor<ContainedClass> *falsifiabilityVisitor)
		{
			this->falsifiabilityVisitor = falsifiabilityVisitor;
		}
	};

	/// Visitor to test whether expression's value can be true
	template <typename ContainedClass>
	class SatisfiabilityVisitor : public PossibilityVisitor<ContainedClass>
	{
		typedef ExpressionBase<ContainedClass> Base;

	public:
		SatisfiabilityVisitor(std::function<bool (const typename Base::Value &)> satisfiabilityTest,
		                      std::function<bool (const typename Base::Value &)> falsifiabilityTest):
			PossibilityVisitor<ContainedClass>(satisfiabilityTest, falsifiabilityTest)
		{
			this->setSatisfiabilityVisitor(this);
		}

		bool operator()(const typename Base::OperatorAny & element) const
		{
			return this->countSatisfiable(element.expressions) != 0;
		}

		bool operator()(const typename Base::OperatorAll & element) const
		{
			return this->countSatisfiable(element.expressions) == element.expressions.size();
		}

		bool operator()(const typename Base::OperatorNone & element) const
		{
			return this->countFalsifiable(element.expressions) == element.expressions.size();
		}

		bool operator()(const typename Base::Value & element) const
		{
			return this->satisfiabilityTest(element);
		}
	};

	/// Visitor to test whether expression's value can be false
	template <typename ContainedClass>
	class FalsifiabilityVisitor : public PossibilityVisitor<ContainedClass>
	{
		typedef ExpressionBase<ContainedClass> Base;

	public:
		FalsifiabilityVisitor(std::function<bool (const typename Base::Value &)> satisfiabilityTest,
		                      std::function<bool (const typename Base::Value &)> falsifiabilityTest):
			PossibilityVisitor<ContainedClass>(satisfiabilityTest, falsifiabilityTest)
		{
			this->setFalsifiabilityVisitor(this);
		}

		bool operator()(const typename Base::OperatorAny & element) const
		{
			return this->countFalsifiable(element.expressions) == element.expressions.size();
		}

		bool operator()(const typename Base::OperatorAll & element) const
		{
			return this->countFalsifiable(element.expressions) != 0;
		}

		bool operator()(const typename Base::OperatorNone & element) const
		{
			return this->countSatisfiable(element.expressions) != 0;
		}

		bool operator()(const typename Base::Value & element) const
		{
			return this->falsifiabilityTest(element);
		}
	};

	/// visitor that is trying to generates candidates that must be fulfilled
	/// to complete this expression
	template<typename ContainedClass>
	class CandidatesVisitor
	{
		typedef ExpressionBase<ContainedClass> Base;
		typedef std::vector<typename Base::Value> TValueList;

		TestVisitor<ContainedClass> classTest;

	public:
		CandidatesVisitor(std::function<bool(const typename Base::Value &)> classTest):
			classTest(classTest)
		{}

		TValueList operator()(const typename Base::OperatorAny & element) const
		{
			TValueList ret;
			if (!classTest(element))
			{
				for (auto & elem : element.expressions)
					boost::range::copy(std::visit(*this, elem), std::back_inserter(ret));
			}
			return ret;
		}

		TValueList operator()(const typename Base::OperatorAll & element) const
		{
			TValueList ret;
			if (!classTest(element))
			{
				for (auto & elem : element.expressions)
					boost::range::copy(std::visit(*this, elem), std::back_inserter(ret));
			}
			return ret;
		}

		TValueList operator()(const typename Base::OperatorNone & element) const
		{
			return TValueList(); //TODO. Implementing this one is not straightforward, if ever possible
		}

		TValueList operator()(const typename Base::Value & element) const
		{
			if (classTest(element))
				return TValueList();
			else
				return TValueList(1, element);
		}
	};

	/// Simple foreach visitor
	template<typename ContainedClass>
	class ForEachVisitor
	{
		typedef ExpressionBase<ContainedClass> Base;

		std::function<typename Base::Variant(const typename Base::Value &)> visitor;

	public:
		ForEachVisitor(std::function<typename Base::Variant(const typename Base::Value &)> visitor):
			visitor(visitor)
		{}

		typename Base::Variant operator()(const typename Base::Value & element) const
		{
			return visitor(element);
		}

		template <typename Type>
		typename Base::Variant operator()(Type element) const
		{
			for (auto & entry : element.expressions)
				entry = std::visit(*this, entry);
			return element;
		}
	};

	/// Minimizing visitor that removes all redundant elements from variant (e.g. AllOf inside another AllOf can be merged safely)
	template<typename ContainedClass>
	class MinimizingVisitor
	{
		typedef ExpressionBase<ContainedClass> Base;

	public:
		typename Base::Variant operator()(const typename Base::Value & element) const
		{
			return element;
		}

		template <typename Type>
		typename Base::Variant operator()(const Type & element) const
		{
			Type ret;

			for (auto & entryRO : element.expressions)
			{
				auto entry = std::visit(*this, entryRO);

				try
				{
					// copy entries from child of this type
					auto sublist = std::get<Type>(entry).expressions;
					std::move(sublist.begin(), sublist.end(), std::back_inserter(ret.expressions));
				}
				catch (std::bad_variant_access &)
				{
					// different type (e.g. allOf vs oneOf) just copy
					ret.expressions.push_back(entry);
				}
			}

			for ( auto it = ret.expressions.begin(); it != ret.expressions.end();)
			{
				if (std::find(ret.expressions.begin(), it, *it) != it)
					it = ret.expressions.erase(it); // erase duplicate
				else
					it++; // goto next
			}
			return ret;
		}
	};

	/// Json parser for expressions
	template <typename ContainedClass>
	class Reader
	{
		typedef ExpressionBase<ContainedClass> Base;

		std::function<typename Base::Value(const JsonNode &)> classParser;

		typename Base::Variant readExpression(const JsonNode & node)
		{
			assert(!node.Vector().empty());

			std::string type = node.Vector()[0].String();
			if (type == "anyOf")
				return typename Base::OperatorAny(readVector(node));
			if (type == "allOf")
				return typename Base::OperatorAll(readVector(node));
			if (type == "noneOf")
				return typename Base::OperatorNone(readVector(node));
			return classParser(node);
		}

		std::vector<typename Base::Variant> readVector(const JsonNode & node)
		{
			std::vector<typename Base::Variant> ret;
			ret.reserve(node.Vector().size()-1);
			for (size_t i=1; i < node.Vector().size(); i++)
				ret.push_back(readExpression(node.Vector()[i]));
			return ret;
		}
	public:
		Reader(std::function<typename Base::Value(const JsonNode &)> classParser):
			classParser(classParser)
		{}
		typename Base::Variant operator ()(const JsonNode & node)
		{
			return readExpression(node);
		}
	};

	/// Serializes expression in JSON format. Part of map format.
	template<typename ContainedClass>
	class Writer
	{
		typedef ExpressionBase<ContainedClass> Base;

		std::function<JsonNode(const typename Base::Value &)> classPrinter;

		JsonNode printExpressionList(std::string name, const std::vector<typename Base::Variant> & element) const
		{
			JsonNode ret;
			ret.Vector().resize(1);
			ret.Vector().back().String() = name;
			for (auto & expr : element)
				ret.Vector().push_back(std::visit(*this, expr));
			return ret;
		}
	public:
		Writer(std::function<JsonNode(const typename Base::Value &)> classPrinter):
			classPrinter(classPrinter)
		{}

		JsonNode operator()(const typename Base::OperatorAny & element) const
		{
			return printExpressionList("anyOf", element.expressions);
		}

		JsonNode operator()(const typename Base::OperatorAll & element) const
		{
			return printExpressionList("allOf", element.expressions);
		}

		JsonNode operator()(const typename Base::OperatorNone & element) const
		{
			return printExpressionList("noneOf", element.expressions);
		}

		JsonNode operator()(const typename Base::Value & element) const
		{
			return classPrinter(element);
		}
	};

	std::string DLL_LINKAGE getTextForOperator(const std::string & operation);

	/// Prints expression in human-readable format
	template<typename ContainedClass>
	class Printer
	{
		typedef ExpressionBase<ContainedClass> Base;

		std::function<std::string(const typename Base::Value &)> classPrinter;
		std::unique_ptr<TestVisitor<ContainedClass>> statusTest;
		mutable std::string prefix;

		template<typename Operator>
		std::string formatString(std::string toFormat, const Operator & expr) const
		{
			// highlight not fulfilled expressions, if pretty formatting is on
			if (statusTest && !(*statusTest)(expr))
				return "{" + toFormat + "}";
			return toFormat;
		}

		std::string printExpressionList(const std::vector<typename Base::Variant> & element) const
		{
			std::string ret;
			prefix.push_back('\t');
			for (auto & expr : element)
				ret += prefix + std::visit(*this, expr) + "\n";
			prefix.pop_back();
			return ret;
		}
	public:
		Printer(std::function<std::string(const typename Base::Value &)> classPrinter):
			classPrinter(classPrinter)
		{}

		Printer(std::function<std::string(const typename Base::Value &)> classPrinter, std::function<bool(const typename Base::Value &)> toBool):
			classPrinter(classPrinter),
			statusTest(new TestVisitor<ContainedClass>(toBool))
		{}

		std::string operator()(const typename Base::OperatorAny & element) const
		{
			return formatString(getTextForOperator("anyOf"), element) + "\n"
					+ printExpressionList(element.expressions);
		}

		std::string operator()(const typename Base::OperatorAll & element) const
		{
			return formatString(getTextForOperator("allOf"), element) + "\n"
					+ printExpressionList(element.expressions);
		}

		std::string operator()(const typename Base::OperatorNone & element) const
		{
			return formatString(getTextForOperator("noneOf"), element) + "\n"
					+ printExpressionList(element.expressions);
		}

		std::string operator()(const typename Base::Value & element) const
		{
			return formatString(classPrinter(element), element);
		}
	};
}

///
/// Class for evaluation of logical expressions generated in runtime
///
template<typename ContainedClass>
class LogicalExpression
{
	typedef LogicalExpressionDetail::ExpressionBase<ContainedClass> Base;
public:
	/// Type of values used in expressions, same as ContainedClass
	typedef typename Base::Value Value;
	/// Operators for use in expressions, all include vectors with operands
	typedef typename Base::OperatorAny OperatorAny;
	typedef typename Base::OperatorAll OperatorAll;
	typedef typename Base::OperatorNone OperatorNone;
	/// one expression entry
	typedef typename Base::Variant Variant;

private:
	Variant data;

public:
	/// Base constructor
	LogicalExpression()
	{}

	/// Constructor from variant or (implicitly) from Operator* types
	LogicalExpression(const Variant & data):
		data(data)
	{
	}

	/// Constructor that receives JsonNode as input and function that can parse Value instances
	LogicalExpression(const JsonNode & input, std::function<Value(const JsonNode &)> parser)
	{
		LogicalExpressionDetail::Reader<Value> reader(parser);
		LogicalExpression expr(reader(input));
		std::swap(data, expr.data);
	}

	Variant get() const
	{
		return data;
	}

	/// Simple visitor that visits all entries in expression
	Variant morph(std::function<Variant(const Value &)> morpher) const
	{
		LogicalExpressionDetail::ForEachVisitor<Value> visitor(morpher);
		return std::visit(visitor, data);
	}

	/// Minimizes expression, removing any redundant elements
	void minimize()
	{
		LogicalExpressionDetail::MinimizingVisitor<Value> visitor;
		data = std::visit(visitor, data);
	}

	/// calculates if expression evaluates to "true".
	/// Note: empty expressions always return true
	bool test(std::function<bool(const Value &)> toBool) const
	{
		LogicalExpressionDetail::TestVisitor<Value> testVisitor(toBool);
		return std::visit(testVisitor, data);
	}

	/// calculates if expression can evaluate to "true".
	bool satisfiable(std::function<bool(const Value &)> satisfiabilityTest, std::function<bool(const Value &)> falsifiabilityTest) const
	{
		LogicalExpressionDetail::SatisfiabilityVisitor<Value> satisfiabilityVisitor(satisfiabilityTest, falsifiabilityTest);
		LogicalExpressionDetail::FalsifiabilityVisitor<Value> falsifiabilityVisitor(satisfiabilityTest, falsifiabilityTest);

		satisfiabilityVisitor.setFalsifiabilityVisitor(&falsifiabilityVisitor);
		falsifiabilityVisitor.setSatisfiabilityVisitor(&satisfiabilityVisitor);

		return std::visit(satisfiabilityVisitor, data);
	}

	/// calculates if expression can evaluate to "false".
	bool falsifiable(std::function<bool(const Value &)> satisfiabilityTest, std::function<bool(const Value &)> falsifiabilityTest) const
	{
		LogicalExpressionDetail::SatisfiabilityVisitor<Value> satisfiabilityVisitor(satisfiabilityTest);
		LogicalExpressionDetail::FalsifiabilityVisitor<Value> falsifiabilityVisitor(falsifiabilityTest);

		satisfiabilityVisitor.setFalsifiabilityVisitor(&falsifiabilityVisitor);
		falsifiabilityVisitor.setFalsifiabilityVisitor(&satisfiabilityVisitor);

		return std::visit(falsifiabilityVisitor, data);
	}

	/// generates list of candidates that can be fulfilled by caller (like AI)
	std::vector<Value> getFulfillmentCandidates(std::function<bool(const Value &)> toBool) const
	{
		LogicalExpressionDetail::CandidatesVisitor<Value> candidateVisitor(toBool);
		return std::visit(candidateVisitor, data);
	}

	/// Converts expression in human-readable form
	/// Second version will try to do some pretty printing using H3 text formatting "{}"
	/// to indicate fulfilled components of an expression
	std::string toString(std::function<std::string(const Value &)> toStr) const
	{
		LogicalExpressionDetail::Printer<Value> printVisitor(toStr);
		return std::visit(printVisitor, data);
	}
	std::string toString(std::function<std::string(const Value &)> toStr, std::function<bool(const Value &)> toBool) const
	{
		LogicalExpressionDetail::Printer<Value> printVisitor(toStr, toBool);
		return std::visit(printVisitor, data);
	}

	JsonNode toJson(std::function<JsonNode(const Value &)> toJson) const
	{
		LogicalExpressionDetail::Writer<Value> writeVisitor(toJson);
		return std::visit(writeVisitor, data);
	}

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & data;
	}
};

VCMI_LIB_NAMESPACE_END
