/*
 * node_store.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

namespace detail
{
// Tags for boost multi_index
// This allows to access them by name
// (only random_access index is accessed by index: 0)
struct by_ordinal_id;
struct by_ptr_identity;
struct by_extra_index;

// Helper template with partial specialization
template<typename T>
struct MultiIndexContainerHelper;

// The elements are stored as std::shared_ptr<T>, but we need
// to be able to look them up by raw T* address to (nedeed by IGraph)
// => define an extractor which converts stored shared_ptr<T> to T*
template<typename T>
struct RawNodePtrKey
{
	using result_type = const T *;
	const T * operator()(const std::shared_ptr<const T> & ptr) const noexcept
	{
		return ptr.get();
	}
};

// Specialization without extra index (e.g. Global nodes)
template<typename T>
requires std::is_same_v<typename T::extra_index_type, void>
struct MultiIndexContainerHelper<T>
{
	using type = boost::multi_index::multi_index_container<
		std::shared_ptr<const T>,
		boost::multi_index::indexed_by<
			boost::multi_index::random_access<boost::multi_index::tag<by_ordinal_id>>,
			boost::multi_index::hashed_unique<boost::multi_index::tag<by_ptr_identity>, RawNodePtrKey<T>>>>;
};

// Specialization with extra index
// To define an extra index, declare a block in the class's public section:
//
//     struct extra_index_type {
//         using result_type = int16_t;
//         result_type operator()(const std::shared_ptr<Hex> & hex) const {
//             return hex->bhex.toInt();
//         }
//     };
template<typename T>
requires(!std::is_same_v<typename T::extra_index_type, void>)
struct MultiIndexContainerHelper<T>
{
	using type = boost::multi_index::multi_index_container<
		std::shared_ptr<const T>,
		boost::multi_index::indexed_by<
			boost::multi_index::random_access<boost::multi_index::tag<by_ordinal_id>>,

			boost::multi_index::hashed_unique<boost::multi_index::tag<by_ptr_identity>, RawNodePtrKey<T>>,

			boost::multi_index::hashed_unique<boost::multi_index::tag<by_extra_index>, typename T::extra_index_type>>>;
};

// Then expose the alias
template<typename T>
using MultiIndexNodeContainer = typename MultiIndexContainerHelper<T>::type;
}

template<typename NodeType>
class NodeStore
{
public:
	using node_type = NodeType;

	NodeStore() = default;

	NodeStore(const NodeStore &) = delete;
	NodeStore & operator=(const NodeStore &) = delete;
	NodeStore(NodeStore &&) = delete;
	NodeStore & operator=(NodeStore &&) = delete;

	// XXX: pass-by-value + move is preferred to pass-by-reference
	// avoids the copying and incrementing the counter when the shared_ptr is rvalue.
	void add(std::shared_ptr<const NodeType> node)
	{
		// Insertion fails when there is a duplicate in *any* unique index
		auto [_, inserted] = container.push_back(std::move(node));
		if(!inserted)
			throw std::runtime_error(std::string(NodeType::encoding_traits::name) + ": insertion failed. Duplicate index?");
	}

	std::shared_ptr<const NodeType> getById(std::size_t ind, bool strict) const
	{
		const auto & idx = container.template get<detail::by_ordinal_id>();
		if(ind >= idx.size())
		{
			if(strict)
				throw std::runtime_error(std::string(NodeType::encoding_traits::name) + ": getById: not found: " + std::to_string(ind));
			return nullptr;
		}
		return idx[ind];
	}

	std::shared_ptr<const NodeType> getByIdentity(const std::shared_ptr<const NodeType> & node, bool strict) const
	{
		const auto & idx = container.template get<detail::by_ptr_identity>();
		auto it = idx.find(node.get());
		if(it == idx.end())
		{
			if(strict)
				throw std::runtime_error(std::string(NodeType::encoding_traits::name) + ": getByIdentity: not found: " + node->name());
			return nullptr;
		}
		return *it;
	}

	// This notation (with explicit typename Key) is preferrable as it allows
	// to pass "convertible" (or "compatible") types.
	// E.g. if the index type is std::string, then passing "foo" here is OK.
	// Without it, the argument would have to be exactly std::string("foo").
	template<typename Key>
	requires(!std::is_same_v<typename NodeType::extra_index_type, void>)
	std::shared_ptr<const NodeType> getByExtraIndex(const Key & key, bool strict) const
	{
		const auto & idx = container.template get<detail::by_extra_index>();
		auto it = idx.find(key);
		if(it == idx.end())
		{
			if(strict)
				throw std::runtime_error(std::string(NodeType::encoding_traits::name) + ": getByExtraIndex: not found");
			return nullptr;
		}
		return *it;
	}

	const auto & entries() const
	{
		return container.template get<detail::by_ordinal_id>();
	}

	std::size_t size() const
	{
		return container.size();
	}

	int64_t getId(const NodeType * node) const
	{
		const auto & identity_idx = container.template get<detail::by_ptr_identity>();

		auto identity_it = identity_idx.find(node);
		if(identity_it == identity_idx.end())
		{
			// std::cout << "ERROR - NODE NOT FOUND: " << node->name() << " " << node << ". ALL NODES:\n";
			// for (const auto & n : entries())
			//     std::cout << n->name() << " " << n.get() << "\n";
			throw std::runtime_error("getId: node not found: " + node->name());
		}

		const auto & ordinal_idx = container.template get<detail::by_ordinal_id>();
		auto ordinal_it = container.template project<detail::by_ordinal_id>(identity_it);
		return std::distance(ordinal_idx.begin(), ordinal_it);
	}

	int64_t getId(const std::shared_ptr<const NodeType> & node) const
	{
		return getId(node.get());
	}

private:
	detail::MultiIndexNodeContainer<NodeType> container;
};
