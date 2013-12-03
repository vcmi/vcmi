#pragma once

//FIXME: move some of code into .cpp to avoid this include?
#include "JsonNode.h"

namespace LogicalExpressionDetail
{
	/// class that defines required types for logical expressions
	template<typename ContainedClass>
	class ExpressionBase
	{
		/// Possible logical operations, mostly needed to create different types for boost::variant
		enum EOperations
		{
			ANY_OF,
			ALL_OF,
			NONE_OF
		};
	public:
		template<EOperations tag> class Element;

		typedef Element<ANY_OF> OperatorAny;
		typedef Element<ALL_OF> OperatorAll;
		typedef Element<NONE_OF> OperatorNone;

		typedef ContainedClass Value;

		/// Variant that contains all possible elements from logical expression
		typedef boost::variant<
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

			template <typename Handler>
			void serialize(Handler & h, const int version)
			{
				h & expressions;
			}
		};
	};

	/// Visitor to test result (true/false) of the expression
	template <typename ContainedClass>
	class TestVisitor : public boost::static_visitor<bool>
	{
		typedef ExpressionBase<ContainedClass> Base;

		std::function<bool(const typename Base::Value &)> classTest;

		size_t countPassed(const std::vector<typename Base::Variant> & element) const
		{
			return boost::range::count_if(element, [&](const typename Base::Variant & expr)
			{
				return boost::apply_visitor(*this, expr);
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

	/// visitor that is trying to generates candidates that must be fulfilled
	/// to complete this expression
	template <typename ContainedClass>
	class CandidatesVisitor : public boost::static_visitor<std::vector<ContainedClass> >
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
					boost::range::copy(boost::apply_visitor(*this, elem), std::back_inserter(ret));
			}
			return ret;
		}

		TValueList operator()(const typename Base::OperatorAll & element) const
		{
			TValueList ret;
			if (!classTest(element))
			{
				for (auto & elem : element.expressions)
					boost::range::copy(boost::apply_visitor(*this, elem), std::back_inserter(ret));
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

	std::string getTextForOperator(std::string operation);

	/// Prints expression in human-readable format
	template <typename ContainedClass>
	class Printer : public boost::static_visitor<std::string>
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
				ret += prefix + boost::apply_visitor(*this, expr) + "\n";
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

	/// calculates if expression evaluates to "true".
	/// Note: empty expressions always return true
	bool test(std::function<bool(const Value &)> toBool) const
	{
		LogicalExpressionDetail::TestVisitor<Value> testVisitor(toBool);
		return boost::apply_visitor(testVisitor, data);
	}

	/// generates list of candidates that can be fulfilled by caller (like AI)
	std::vector<Value> getFulfillmentCandidates(std::function<bool(const Value &)> toBool) const
	{
		LogicalExpressionDetail::CandidatesVisitor<Value> candidateVisitor(toBool);
		return boost::apply_visitor(candidateVisitor, data);
	}

	/// Converts expression in human-readable form
	/// Second version will try to do some pretty printing using H3 text formatting "{}"
	/// to indicate fulfilled components of an expression
	std::string toString(std::function<std::string(const Value &)> toStr) const
	{
		LogicalExpressionDetail::Printer<Value> printVisitor(toStr);
		return boost::apply_visitor(printVisitor, data);
	}
	std::string toString(std::function<std::string(const Value &)> toStr, std::function<bool(const Value &)> toBool) const
	{
		LogicalExpressionDetail::Printer<Value> printVisitor(toStr, toBool);
		return boost::apply_visitor(printVisitor, data);
	}

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & data;
	}
};
