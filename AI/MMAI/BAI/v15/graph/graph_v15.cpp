/*
 * graph.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/graph/graph_v15.h"

#include "BAI/v15/fastbfs_v15.h"
#include "schema/v15/graph.h"
#include <stdexcept>

namespace MMAI::BAI::V15::Graph
{

namespace
{
	std::vector<BattleHex>
	NearbyPositions(const BattleHex & defenderPos, BattleSide attackerSide, BattleSide defenderSide, bool isAttackerWide, bool isDefenderWide)
	{
		auto res = std::vector<BattleHex>{};

		auto checkAndPush = [&res](const BattleHex & bh)
		{
			if(bh.isAvailable())
				res.push_back(bh);
		};

		using EDir = BattleHex::EDir;
		for(auto edir : {EDir::TOP_RIGHT, EDir::BOTTOM_RIGHT, EDir::BOTTOM_LEFT, EDir::TOP_LEFT})
			checkAndPush(defenderPos.cloneInDirection(edir, false));

		auto x = defenderPos;
		auto l = x.cloneInDirection(EDir::LEFT, false);
		auto r = x.cloneInDirection(EDir::RIGHT, false);

		if(isDefenderWide && isAttackerWide)
		{
			if(defenderSide == attackerSide)
			{
				/*
				 *     Defender=WL "~x"   <OR>   Defender=WR "x~"
				 *     Attacker=WL "~o"          Attacker=WR "o~"
				 *  . . . . . . . . . . . .
				 * . . . . o * * o . . . . .  Legend:
				 *  . . . o . x . o . . . .    x: defender pos
				 * . . . . o * * o . . . . .   *: hex already added
				 *  . . . . . . . . . . . .    o: hex to add now
				 */
				checkAndPush(r.cloneInDirection(EDir::TOP_RIGHT, false));
				checkAndPush(r.cloneInDirection(EDir::RIGHT, false));
				checkAndPush(r.cloneInDirection(EDir::BOTTOM_RIGHT, false));
				checkAndPush(l.cloneInDirection(EDir::TOP_LEFT, false));
				checkAndPush(l.cloneInDirection(EDir::LEFT, false));
				checkAndPush(l.cloneInDirection(EDir::BOTTOM_LEFT, false));
			}
			else if(defenderSide == BattleSide::LEFT_SIDE)
			{
				/*
				 *  . . . . . . . . . . . .
				 * . . . o o * * . . . . . .   Defender=WL "~x"
				 *  . . o . . x o . . . . .    Attacker=WR "o~"
				 * . . . o o * * . . . . . .
				 *  . . . . . . . . . . . .
				 */
				checkAndPush(r);
				checkAndPush(l.cloneInDirection(EDir::TOP_LEFT, false));
				checkAndPush(l.cloneInDirection(EDir::BOTTOM_LEFT, false));
				auto ll = l.cloneInDirection(EDir::LEFT, false);
				checkAndPush(ll.cloneInDirection(EDir::TOP_LEFT, false));
				checkAndPush(ll.cloneInDirection(EDir::LEFT, false));
				checkAndPush(ll.cloneInDirection(EDir::BOTTOM_LEFT, false));
			}
			else
			{
				/*
				 *  . . . . . . . . . . . .
				 * . . . * * o o . . . . . .   Defender=WR "x~"
				 *  . . o x . . o . . . . .    Attacker=WL "~o"
				 * . . . * * o o . . . . . .
				 *  . . . . . . . . . . . .
				 */
				checkAndPush(l);
				checkAndPush(r.cloneInDirection(EDir::TOP_RIGHT, false));
				checkAndPush(r.cloneInDirection(EDir::BOTTOM_RIGHT, false));
				auto rr = r.cloneInDirection(EDir::RIGHT, false);
				checkAndPush(rr.cloneInDirection(EDir::TOP_RIGHT, false));
				checkAndPush(rr.cloneInDirection(EDir::RIGHT, false));
				checkAndPush(rr.cloneInDirection(EDir::BOTTOM_RIGHT, false));
			}
		}
		else if(isDefenderWide)
		{
			if(defenderSide == BattleSide::LEFT_SIDE)
			{
				/*
				 *  . . . . . . . . . . . .
				 * . . . . o * * . . . . . .  Defender=WL     "~x"
				 *  . . . o . x o . . . . .   Attacker=(any)  "o"
				 * . . . . o * * . . . . . .
				 *  . . . . . . . . . . . .
				 */
				checkAndPush(r);
				checkAndPush(l.cloneInDirection(EDir::TOP_LEFT, false));
				checkAndPush(l.cloneInDirection(EDir::LEFT, false));
				checkAndPush(l.cloneInDirection(EDir::BOTTOM_LEFT, false));
			}
			else
			{
				/*
				 *  . . . . . . . . . . . .
				 * . . . . . * * o . . . . .   Defender=WR    "x~"
				 *  . . . . o x . o . . . .    Attacker=(any) "o"
				 * . . . . . * * o . . . . .
				 *  . . . . . . . . . . . .
				 */
				checkAndPush(l);
				checkAndPush(r.cloneInDirection(EDir::TOP_RIGHT, false));
				checkAndPush(r.cloneInDirection(EDir::RIGHT, false));
				checkAndPush(r.cloneInDirection(EDir::BOTTOM_RIGHT, false));
			}
		}
		else if(isAttackerWide)
		{
			if(attackerSide == BattleSide::LEFT_SIDE)
			{
				/*
				 *  . . . . . . . . . . . .
				 * . . . . . * * o . . . . .  Defender=(any)  "x"
				 *  . . . . o x . o . . . .   Attacker=WL     "~o"
				 * . . . . . * * o . . . . .
				 *  . . . . . . . . . . . .
				 */
				checkAndPush(l);
				checkAndPush(r.cloneInDirection(EDir::TOP_RIGHT, false));
				checkAndPush(r.cloneInDirection(EDir::RIGHT, false));
				checkAndPush(r.cloneInDirection(EDir::BOTTOM_RIGHT, false));
			}
			else
			{
				/*
				 *  . . . . . . . . . . . .
				 * . . . . o * * . . . . . .  Defender=(any)  "x"
				 *  . . . o . x o . . . . .   Attacker=WR     "o~"
				 * . . . . o * * . . . . . .
				 *  . . . . . . . . . . . .
				 */
				checkAndPush(r);
				checkAndPush(l.cloneInDirection(EDir::TOP_LEFT, false));
				checkAndPush(l.cloneInDirection(EDir::LEFT, false));
				checkAndPush(l.cloneInDirection(EDir::BOTTOM_LEFT, false));
			}
		}
		else
		{
			/*
			 *  . . . . . . . . . . . .
			 * . . . . . * * . . . . . .  Defender=(any)  "x"
			 *  . . . . o x o . . . . .   Attacker=(any)  "o"
			 * . . . . . * * . . . . . .
			 *  . . . . . . . . . . . .
			 */
			checkAndPush(l);
			checkAndPush(r);
		}

		for(const auto & bh : res)
			if(!bh.isAvailable())
				throw std::runtime_error("Unavailable hex while precalculating nearby positions");

		return res;
	}

	detail::PrecalculatedNearbyPositions PrecalculateNearbyPositions()
	{
		auto res = detail::PrecalculatedNearbyPositions{};
		static_assert(static_cast<int>(BattleSide::LEFT_SIDE) == 0);
		static_assert(static_cast<int>(BattleSide::RIGHT_SIDE) == 1);
		for(int attackerSide : {0, 1})
		{
			for(int defenderSide : {0, 1})
			{
				for(int isAttackerWide : {0, 1})
				{
					for(int isDefenderWide : {0, 1})
					{
						for(int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
						{
							const auto bhex = BattleHex(i);
							if(!bhex.isAvailable())
								continue;

							res.ary[attackerSide][defenderSide][isAttackerWide][isDefenderWide][bhex.toInt()] = NearbyPositions(
								bhex, static_cast<BattleSide>(attackerSide), static_cast<BattleSide>(defenderSide), isAttackerWide, isDefenderWide
							);
						}
					}
				}
			}
		}

		return res;
	}
}

Graph::Graph(const CPlayerBattleCallback & battle)
	: accessibility(battle.getAccessibility()), fastbfs(FastBFS(battle, accessibility)), nearbyPositions(PrecalculateNearbyPositions()) {};

const S15::Graph::INode * Graph::getNode(Schema::V15::Graph::ElementType t, std::size_t ind) const
{
	return withNodeStore(
		t,
		[ind](const auto & store)
		{
			const S15::Graph::INode * node = store.getById(ind, true).get();
			return node;
		}
	);
};

std::vector<const S15::Graph::INode *> Graph::getNodes(Schema::V15::Graph::ElementType t) const
{
	return withNodeStore(
		t,
		[](const auto & store)
		{
			std::vector<const S15::Graph::INode *> res;
			res.reserve(store.size());
			for(const auto & e : store.entries())
				res.push_back(e.get());
			return res;
		}
	);
}

std::vector<const S15::Graph::IEdge *> Graph::getEdges(Schema::V15::Graph::ElementType t) const
{
	return withEdgeStore(
		t,
		[](const auto & store)
		{
			std::vector<const S15::Graph::IEdge *> res;
			res.reserve(store.size());
			for(const auto & e : store.entries())
				res.push_back(e.get());
			return res;
		}
	);
}

int64_t Graph::getNodeIndex(const S15::Graph::INode * inode) const
{
	return withNodeStore(
		inode->getType(),
		[&inode](const auto & store)
		{
			using Store = std::decay_t<decltype(store)>;
			using Node = typename Store::node_type;

			// Downcast INode* to the real node, e.g. Nodes::Player*
			const auto * node = dynamic_cast<const Node *>(inode);
			if(!node)
				throwf("Node type does not match element type: %1%", EI(inode->getType())); // NOSONAR: inode is never null

			return store.getId(node);
		}
	);
}

int64_t Graph::getEdgeIndex(const S15::Graph::IEdge * iedge) const
{
	return withEdgeStore(
		iedge->getType(),
		[&iedge](const auto & store)
		{
			using Store = std::decay_t<decltype(store)>;
			using Edge = typename Store::edge_type;

			// Downcast INode* to the real node, e.g. Nodes::Player*
			const auto * edge = dynamic_cast<const Edge *>(iedge);
			if(!edge)
				throwf("Node type does not match element type: %1%", EI(iedge->getType())); // NOSONAR: iedge is never null

			return store.getId(edge);
		}
	);
}

std::vector<int64_t> Graph::getActiveActionIds() const
{
	auto res = std::vector<int64_t>{};
	int i = 0;
	for(const auto & action : getAll<Nodes::Action>())
	{
		if(action->isActive)
			res.push_back(i);
		++i;
	}

	return res;
}

void Graph::verify() const
{
	for(int i = 0; i < EU(ET::_count); ++i)
	{
		auto et = ET(i);
		switch(et)
		{
			case ET::NODE_GLOBAL:
			case ET::NODE_PLAYER:
			case ET::NODE_UNIT:
			case ET::NODE_HEX:
			case ET::NODE_ACTION:
				withNodeStore(
					et,
					[](const auto & store)
					{
						for(const auto & node : store.entries())
							node->verify();
					}
				);
				break;
			// All other types are edges
			default:
				withEdgeStore(
					et,
					[](const auto & store)
					{
						for(const auto & edge : store.entries())
							edge->verify();
					}
				);
				break;
		}
	}
}

EnumFlags<S15::Graph::ElementType> Graph::getFlags() const
{
	return flags;
}

void Graph::setFlag(S15::Graph::ElementType et)
{
	flags.set(et);
}

const AccessibilityInfo & Graph::getAccessibility() const
{
	return accessibility;
}

const FastBFS & Graph::getFastBFS() const
{
	return fastbfs;
}

const detail::PrecalculatedNearbyPositions & Graph::getNearbyPositions() const
{
	return nearbyPositions;
}

} // namespace
