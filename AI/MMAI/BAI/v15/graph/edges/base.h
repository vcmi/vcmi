#pragma once

#include "common.h" // IWYU pragma: keep

#include "BAI/v15/graph/element.h"
#include "schema/v15/graph.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

template<typename SrcNode, typename DstNode, typename EncTraits>
class Base : public Element<S15::Graph::IEdge, EncTraits>
{
	using Base_Base = Element<S15::Graph::IEdge, EncTraits>;

public:
	using src_node_type = SrcNode;
	using dst_node_type = DstNode;

	// bring names into scope
	// (needed due to dependent name lookup rules in C++ templates)
	using Base_Base::attrs;
	using Base_Base::setattr;
	using A = typename EncTraits::A;

	Base(const std::shared_ptr<const SrcNode> & srcNode, const std::shared_ptr<const DstNode> & dstNode) : Base_Base(), srcNode(srcNode), dstNode(dstNode) {}

	Base(const Base &) = delete;
	Base & operator=(const Base &) = delete;
	Base(Base &&) = delete;
	Base & operator=(Base &&) = delete;

	std::string name() const override
	{
		std::stringstream ss;
		// ss << Base_Base::name() << "[" << srcNode->name() << "/" << srcNode.get() << "->" << dstNode->name() << "/" <<dstNode.get() <<"]";
		ss << Base_Base::name() << "[" << srcNode->name() << "->" << dstNode->name() << "]";
		return ss.str();
	}

	Schema::V15::Graph::Endpoints endpoints() const override
	{
		return {srcNode.get(), dstNode.get()};
	}

	const std::shared_ptr<const SrcNode> srcNode;
	const std::shared_ptr<const DstNode> dstNode;
};
}
