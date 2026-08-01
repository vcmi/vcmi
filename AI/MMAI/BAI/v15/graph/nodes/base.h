/*
 * base.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "common.h" // IWYU pragma: keep

#include "BAI/v15/graph/element.h"
#include "schema/v15/graph.h"

namespace MMAI::BAI::V15::Graph::Nodes
{
namespace S15 = Schema::V15;

template<typename EncTraits>
class Base : public Element<S15::Graph::INode, EncTraits>
{
public:
	// bring names into scope
	// (needed due to dependent name lookup rules in C++ templates)
	using Element<S15::Graph::INode, EncTraits>::attrs;
	using Element<S15::Graph::INode, EncTraits>::setattr;
	using A = typename EncTraits::A;
	using extra_index_type = void;

	Base() = default;

	Base(const Base &) = delete;
	Base & operator=(const Base &) = delete;
	Base(Base &&) = delete;
	Base & operator=(Base &&) = delete;
};
}
