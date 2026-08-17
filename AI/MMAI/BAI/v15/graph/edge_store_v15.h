/*
 * edge_store.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

namespace MMAI::BAI::V15::Graph
{

namespace detail
{
	struct by_ordinal_id;
	struct by_ptr_identity;
	struct by_src_dst_nodes;
	struct by_src_node;
	struct by_dst_node;

	template<typename EdgeType>
	struct SrcKey
	{
		using result_type = std::shared_ptr<const typename EdgeType::src_node_type>;
		result_type operator()(const std::shared_ptr<const EdgeType> & edge) const
		{
			return edge->srcNode;
		}
	};

	template<typename EdgeType>
	struct DstKey
	{
		using result_type = std::shared_ptr<const typename EdgeType::dst_node_type>;
		result_type operator()(const std::shared_ptr<const EdgeType> & edge) const
		{
			return edge->dstNode;
		}
	};

	// See comment in NodeStore::RawNodePtrKey
	template<typename T>
	struct RawEdgePtrKey
	{
		using result_type = const T *;
		const T * operator()(const std::shared_ptr<const T> & ptr) const noexcept
		{
			return ptr.get();
		}
	};

	template<typename T>
	using MultiIndexEdgeContainer = boost::multi_index::multi_index_container<
		std::shared_ptr<const T>,
		boost::multi_index::indexed_by<
			boost::multi_index::random_access<boost::multi_index::tag<by_ordinal_id>>,

			boost::multi_index::hashed_unique<boost::multi_index::tag<by_ptr_identity>, RawEdgePtrKey<T>>,

			boost::multi_index::
				hashed_unique<boost::multi_index::tag<by_src_dst_nodes>, boost::multi_index::composite_key<std::shared_ptr<const T>, SrcKey<T>, DstKey<T>>>,

			boost::multi_index::hashed_non_unique<boost::multi_index::tag<by_src_node>, SrcKey<T>>,

			boost::multi_index::hashed_non_unique<boost::multi_index::tag<by_dst_node>, DstKey<T>>>>;
}

template<typename EdgeType>
class EdgeStore
{
public:
	using edge_type = EdgeType;

	EdgeStore() = default;

	EdgeStore(const EdgeStore &) = delete;
	EdgeStore & operator=(const EdgeStore &) = delete;
	EdgeStore(EdgeStore &&) = delete;
	EdgeStore & operator=(EdgeStore &&) = delete;

	// XXX: pass-by-value + move is preferred, see comment in NodeStore::add
	void add(std::shared_ptr<const EdgeType> edge)
	{
		// Insertion fails when there is a duplicate in *any* unique index
		auto [_, inserted] = container.push_back(std::move(edge));
		if(!inserted)
			throwf("%1%: add: insertion failed. Duplicate index?", EdgeType::encoding_traits::name);
	}

	std::shared_ptr<const EdgeType> getById(std::size_t ind, bool strict) const
	{
		const auto & idx = container.template get<detail::by_ordinal_id>();
		if(ind >= idx.size())
		{
			if(strict)
				throwf("%1%: getById: not found: %2%", EdgeType::encoding_traits::name, ind);
			return nullptr;
		}
		return idx[ind];
	}

	std::shared_ptr<const EdgeType> getByIdentity(const std::shared_ptr<const EdgeType> & edge, bool strict) const
	{
		const auto & idx = container.template get<detail::by_ptr_identity>();
		auto it = idx.find(edge);
		if(it == idx.end())
		{
			if(strict)
				throwf("%1%: getByIdentity: not found: %2%", EdgeType::encoding_traits::name, edge->name());
			return nullptr;
		}
		return *it;
	}

	std::shared_ptr<const EdgeType> getBySrcDst(
		const std::shared_ptr<const typename EdgeType::src_node_type> & src,
		const std::shared_ptr<const typename EdgeType::dst_node_type> & dst,
		bool strict
	) const
	{
		const auto & idx = container.template get<detail::by_src_dst_nodes>();
		auto it = idx.find(boost::make_tuple(src, dst));
		if(it == idx.end())
		{
			if(strict)
				throwf("%1%: getBySrcDst: not found: [%2%->%3%]", EdgeType::encoding_traits::name, src->name(), dst->name());
			return nullptr;
		}
		return *it;
	}

	std::shared_ptr<const EdgeType> getOneBySrc(const std::shared_ptr<const typename EdgeType::src_node_type> & src, bool strict) const
	{
		const auto & idx = container.template get<detail::by_src_node>();
		auto it = idx.find(src);
		if(it == idx.end())
		{
			if(strict)
				throwf("%1%: getOneBySrc: none found: %2%", EdgeType::encoding_traits::name, src->name());
			return nullptr;
		}
		return *it;
	}

	std::shared_ptr<const typename EdgeType::dst_node_type>
	getOneDstBySrc(const std::shared_ptr<const typename EdgeType::src_node_type> & src, bool strict) const
	{
		if(auto res = getOneBySrc(src, strict))
			return res->dstNode;
		return nullptr;
	}

	auto getAllBySrc(const std::shared_ptr<const typename EdgeType::src_node_type> & src) const
	{
		const auto & idx = container.template get<detail::by_src_node>();
		auto [first, last] = idx.equal_range(src);
		return std::ranges::subrange(first, last);
	}

	auto getAllDstBySrc(const std::shared_ptr<const typename EdgeType::src_node_type> & src) const
	{
		return getAllBySrc(src)
			 | std::views::transform(
				   [](const auto & edge)
				   {
					   return edge->dstNode;
				   }
			 );
	}

	std::shared_ptr<const EdgeType> getOneByDst(const std::shared_ptr<const typename EdgeType::dst_node_type> & dst, bool strict) const
	{
		const auto & idx = container.template get<detail::by_dst_node>();
		auto it = idx.find(dst);
		if(it == idx.end())
		{
			if(strict)
				throwf("%1%: getOneByDst: none found: %2%", EdgeType::encoding_traits::name, dst->name());
			return nullptr;
		}
		return *it;
	}

	std::shared_ptr<const typename EdgeType::src_node_type>
	getOneSrcByDst(const std::shared_ptr<const typename EdgeType::dst_node_type> & dst, bool strict) const
	{
		if(auto res = getOneByDst(dst, strict))
			return res->srcNode;
		return nullptr;
	}

	auto getAllByDst(const std::shared_ptr<const typename EdgeType::dst_node_type> & dst) const
	{
		const auto & idx = container.template get<detail::by_dst_node>();
		auto [first, last] = idx.equal_range(dst);
		return std::ranges::subrange(first, last);
	}

	auto getAllSrcByDst(const std::shared_ptr<const typename EdgeType::dst_node_type> & dst) const
	{
		return getAllByDst(dst)
			 | std::views::transform(
				   [](const auto & edge)
				   {
					   return edge->srcNode;
				   }
			 );
	}

	const auto & entries() const
	{
		return container.template get<detail::by_ordinal_id>();
	}

	std::size_t size() const
	{
		return container.size();
	}

	int64_t getId(const EdgeType * edge) const
	{
		const auto & identity_idx = container.template get<detail::by_ptr_identity>();

		auto identity_it = identity_idx.find(edge);
		if(identity_it == identity_idx.end())
			throwf("%1%: getId: not found", EdgeType::encoding_traits::name);

		const auto & ordinal_idx = container.template get<detail::by_ordinal_id>();
		auto ordinal_it = container.template project<detail::by_ordinal_id>(identity_it);
		return std::distance(ordinal_idx.begin(), ordinal_it);
	}

	int64_t getId(const std::shared_ptr<const EdgeType> & edge) const
	{
		return getId(edge.get());
	}

private:
	detail::MultiIndexEdgeContainer<EdgeType> container;
};

}
