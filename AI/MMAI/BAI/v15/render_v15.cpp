/*
 * render.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/render_v15.h"

#include "BAI/v15/graph/edges/generic_v15.h"
#include "BAI/v15/graph/edges/unit_melee_dmg_unit_v15.h"
#include "BAI/v15/graph/nodes/hex_v15.h"
#include "BAI/v15/state_v15.h"

#include "schema/v15/graph.h"
#include "schema/v15/types.h"

namespace MMAI::BAI::V15
{
namespace S15 = Schema::V15;
namespace N = Graph::Nodes;
namespace E = Graph::Edges;

using GA = N::Global::Attribute;
using PA = N::Player::Attribute;
using UA = N::Unit::Attribute;
using HA = N::Hex::Attribute;
using AA = N::Action::Attribute;

namespace EA = S15::Graph::EdgeAttributes;

using PlayerPtr = std::shared_ptr<const N::Player>;
using UnitPtr = std::shared_ptr<const N::Unit>;
using HexPtr = std::shared_ptr<const N::Hex>;
using ActionPtr = std::shared_ptr<const N::Action>;

template<typename E>
using EdgePtr = std::shared_ptr<const E>;

namespace
{
	std::string PadLeft(const std::string & input, int desiredLength, char paddingChar = ' ')
	{
		std::ostringstream ss;
		ss << std::right << std::setfill(paddingChar) << std::setw(desiredLength) << input;
		return ss.str();
	}

	std::map<UnitPtr, int> BuildQueue(const Graph::Graph * G)
	{
		struct OutEdge
		{
			UnitPtr dst;
			int weight;
		};

		std::map<UnitPtr, std::vector<OutEdge>> graph;
		std::map<UnitPtr, int> indegree;
		std::map<UnitPtr, int> position;

		// Build graph
		for(const auto & edge : G->getAll<E::Unit_ActsBefore_Unit>())
		{
			// auto [src, dst] = edge->endpoints();
			const auto & src = edge->srcNode;
			const auto & dst = edge->dstNode;
			const int w = edge->attr(EA::Unit_ActsBefore_Unit::TIMES);

			if(w < 0)
				throw std::runtime_error("ActsBefore::times() cannot be negative");

			graph[src].push_back(OutEdge{.dst = dst, .weight = w});

			indegree.try_emplace(src, 0);
			indegree.try_emplace(dst, 0);
			position.try_emplace(src, 0);
			position.try_emplace(dst, 0);

			++indegree[dst];
		}

		// Start with nodes that have no incoming constraints
		std::queue<UnitPtr> q;

		for(const auto & [unit, deg] : indegree)
		{
			if(deg == 0)
			{
				q.push(unit);
			}
		}

		std::size_t visited = 0;

		while(!q.empty())
		{
			UnitPtr src = q.front();
			q.pop();

			++visited;

			for(const auto & e : graph[src])
			{
				position[e.dst] = std::max(position[e.dst], position[src] + e.weight);

				--indegree[e.dst];

				if(indegree[e.dst] == 0)
				{
					q.push(e.dst);
				}
			}
		}

		if(visited != indegree.size())
			throw std::runtime_error("ActsBefore graph contains a cycle");

		return position;
	}
}

// This intentionally uses the IState interface to ensure that
// the schema is properly exposing all needed informaton
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::string Render(const State * state, const ActionPtr & action)
{
	const auto & alogs = state->attackLogs;
	const auto * G = state->G.get();
	const auto & gnode = G->getAll<N::Global>().at(0);
	const auto & pnodes = G->getAll<N::Player>();

	PlayerPtr lplayer;
	PlayerPtr rplayer;

	for(const auto & player : pnodes)
	{
		switch(player->attr(PA::BATTLE_SIDE))
		{
			case EU(BattleSide::LEFT_SIDE):
				lplayer = player;
				break;
			case EU(BattleSide::RIGHT_SIDE):
				rplayer = player;
				break;
			default:
				throw std::runtime_error("unexpected player side");
		}
	}

	ASSERT(lplayer->attr(PA::BATTLE_SIDE) == EU(BattleSide::LEFT_SIDE), "invalid player side");
	ASSERT(lplayer && rplayer, "players not found");

	const auto & myplayer = lplayer->attr(PA::IS_ACTIVE) ? lplayer : rplayer;

	ASSERT(myplayer->attr(PA::IS_ACTIVE), "active player not found");

	UnitPtr aunit = nullptr;

	for(const auto & unit : G->getAll<N::Unit>())
	{
		if(unit->attr(UA::IS_ACTIVE))
		{
			aunit = unit;
			break;
		}
	}

	auto ended = gnode->attr(GA::BATTLE_WINNER) != EU(S15::CombatResult::NONE);

	if(!aunit && !ended)
		logAi->error("could not find an active stack (battle has not ended).");

	// {hex -> [activeAction, ...]}
	auto hexActiveActions = std::unordered_map<HexPtr, std::unordered_set<ActionPtr>>{};

	for(const auto & e : G->getAll<E::Action_EndsAt_Hex>())
	{
		const auto & action = e->srcNode;
		const auto & hex = e->dstNode;
		if(!action->isActive)
			continue;
		auto [_, inserted] = hexActiveActions[hex].insert(action);
		ASSERT(inserted, "duplicate active action on hex");
	}

	std::string nocol = "\033[0m";
	std::string redcol = "\033[31m"; // red
	std::string bluecol = "\033[34m"; // blue
	std::string allycol = lplayer->attr(PA::IS_ACTIVE) ? redcol : bluecol;
	std::string enemycol = allycol == redcol ? bluecol : redcol;
	std::string darkcol = "\033[90m";
	std::string activemod = "\033[107m\033[7m"; // bold+reversed
	// std::string ukncol = "\033[7m"; // white

	std::vector<std::stringstream> lines;

	//
	// 1. Add logs table:
	//
	// #1 attacks #5 for 16 dmg (1 killed)
	// #5 attacks #1 for 4 dmg (0 killed)
	// ...
	//
	for(const auto & alog : alogs)
	{
		auto row = std::stringstream();
		std::string attcol;
		std::string attalias = "?";
		std::string defcol;
		std::string defalias = "?";

		if(alog.attacker)
		{
			attcol = alog.attacker->cstack.unitSide() == BattleSide::LEFT_SIDE ? "red" : "blue";
			attalias = alog.attacker->alias;
		}

		if(alog.defender)
		{
			defcol = alog.defender->cstack.unitSide() == BattleSide::LEFT_SIDE ? "red" : "blue";
			defalias = alog.defender->alias;
		}

		row << attcol << "#" << attalias << nocol;
		row << " attacks ";
		row << defcol << "#" << defalias << nocol;
		row << " for " << alog.getDamageDealt() << " dmg";
		row << " (kills: " << alog.getUnitsKilled() << ", value: " << alog.getValueKilled() << " / " << alog.getValueKilledPermille() << "‰)";

		lines.push_back(std::move(row));
	}

	//
	// 2. Build ASCII table
	//    (+populate aliveStacks var)
	//    NOTE: the contents below look mis-aligned in some editors.
	//          In (my) terminal, it all looks correct though.
	//
	//   ▕₁▕₂▕₃▕₄▕₅▕₆▕₇▕₈▕₉▕₀▕₁▕₂▕₃▕₄▕₅▕
	//  ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃
	// ¹┨  1 ◌ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ 1 ┠¹
	// ²┨ ◌ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌  ┠²
	// ³┨  ◌ ○ ○ ○ ○ ○ ○ ◌ ▦ ▦ ◌ ◌ ◌ ◌ ◌ ┠³
	// ⁴┨ ◌ ○ ○ ○ ○ ○ ○ ○ ▦ ▦ ▦ ◌ ◌ ◌ ◌  ┠⁴
	// ⁵┨  2 ◌ ○ ○ ▦ ▦ ◌ ○ ◌ ◌ ◌ ◌ ◌ ◌ 2 ┠⁵
	// ⁶┨ ◌ ○ ○ ○ ▦ ▦ ◌ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌  ┠⁶
	// ⁷┨  3 3 ○ ○ ○ ▦ ◌ ○ ○ ◌ ◌ ▦ ◌ ◌ 3 ┠⁷
	// ⁸┨ ◌ ○ ○ ○ ○ ○ ○ ○ ○ ◌ ◌ ▦ ▦ ◌ ◌  ┠⁸
	// ⁹┨  ◌ ○ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ┠⁹
	// ⁰┨ ◌ ○ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌  ┠⁰
	// ¹┨  4 ◌ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ 4 ┠¹
	//  ┃▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁┃
	//   ▕¹▕²▕³▕⁴▕⁵▕⁶▕⁷▕⁸▕⁹▕⁰▕¹▕²▕³▕⁴▕⁵▕
	//

	// s=special; can be any number, slot is always 7 (SPECIAL), visualized A,B,C...

	auto tablestartrow = lines.size();

	lines.emplace_back() << "    ₀▏₁▏₂▏₃▏₄▏₅▏₆▏₇▏₈▏₉▏₀▏₁▏₂▏₃▏₄";
	lines.emplace_back() << " ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃ ";

	static const std::array<std::string, 10> nummap{"₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉"};

	bool addspace = true;
	bool divlines = true;

	// y even "▏"
	// y odd "▕"

	// {unit -> hex}
	auto seenunits = std::unordered_map<UnitPtr, HexPtr>{};

	ASSERT(G->size<N::Hex>() == 165, "165 hexes expected");
	for(int i = 0; i < 165; ++i)
	{
		const auto & hex = G->getById<N::Hex>(i);
		const auto & unit = G->getOneEdgeSrcByDst<E::Unit_Occupies_Hex>(hex, false);
		auto sym = std::string("?");

		int y = i / 15;
		int x = i % 15;

		const char * spacer = (y % 2 == 0) ? " " : "";
		auto & row = (x == 0) ? (lines.emplace_back() << nummap.at(y % 10) << "┨" << spacer) : lines.back();

		if(addspace)
		{
			if(divlines && (x != 0))
			{
				row << darkcol << (y % 2 == 0 ? "▏" : "▕") << nocol;
			}
			else
			{
				row << " ";
			}
		}

		addspace = true;

		using HexStateMask = std::bitset<4>;

		auto smask = HexStateMask();
		if(hex->attr(N::Hex::A::IS_PASSABLE))
			smask.set(0);
		if(hex->attr(N::Hex::A::IS_STOPPING))
			smask.set(1);
		if(hex->attr(N::Hex::A::IS_DAMAGING_L))
			smask.set(2);
		if(hex->attr(N::Hex::A::IS_DAMAGING_R))
			smask.set(3);

		auto col = nocol;

		// First put symbols based on hex state.
		// If there's a stack on this hex, symbol will be overriden.
		HexStateMask mpass = 1 << 0;
		HexStateMask mstop = 1 << 1;
		HexStateMask mdmgl = 1 << 2;
		HexStateMask mdmgr = 1 << 3;
		HexStateMask mdefault = 0;

		std::vector<std::tuple<std::string, std::string, HexStateMask>> symbols{
			{"⨻", bluecol, mpass | mstop | mdmgl},
			{"⨻", redcol,  mpass | mstop | mdmgr},
			{"✶", bluecol, mpass | mdmgl        },
			{"✶", redcol,  mpass | mdmgr        },
			{"△", nocol,   mpass | mstop        },
			{"○", nocol,   mpass                }, // changed to "◌" if unreachable
			{"◼", nocol,   mdefault             }
		};

		for(const auto & tuple : symbols)
		{
			const auto & [s, c, m] = tuple;
			if((smask & m) == m)
			{
				sym = s;
				col = c;
				break;
			}
		}

		bool hasMoveAction = std::ranges::find_if(
								 hexActiveActions[hex],
								 [](const ActionPtr & act)
								 {
									 return act->actionType == S15::ActionType::MOVE;
								 }
							 )
						  != hexActiveActions[hex].end();

		if(col == nocol && !hasMoveAction)
		{ // || supdata->getIsBattleEnded()
			col = darkcol;
			sym = sym == "○" ? "◌" : sym;
		}

		if(unit)
		{
			auto seen = seenunits.contains(unit);

			sym = unit->alias;
			col = unit->attr(UA::IS_ENEMY) ? enemycol : allycol;

			if(unit == aunit)
				col += activemod;

			if(unit->cstack.doubleWide() && !seen)
			{
				if(unit->cstack.unitSide() == BattleSide::LEFT_SIDE)
				{
					sym += "↠";
					addspace = false;
				}
				else if(unit->cstack.unitSide() == BattleSide::RIGHT_SIDE && hex->attr(HA::X_COORD) < 14)
				{
					sym += "↞";
					addspace = false;
				}
			}

			if(!seen)
				seenunits.try_emplace(unit, hex);
		}

		row << col << sym << nocol;

		if(x == 15 - 1)
		{
			row << (y % 2 == 0 ? " " : "  ") << "┠" << nummap.at(y % 10);
		}
	}

	lines.emplace_back() << " ┃▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁┃";
	lines.emplace_back() << "   ⁰▕¹▕²▕³▕⁴▕⁵▕⁶▕⁷▕⁸▕⁹▕⁰▕¹▕²▕³▕⁴";

	//
	// 3. Add side table stuff
	//
	//   ▕₁▕₂▕₃▕₄▕₅▕₆▕₇▕₈▕₉▕₀▕₁▕₂▕₃▕₄▕₅▕
	//  ┃▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔┃         Player: RED
	// ₁┨  ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ┠₁    Last action:
	// ₂┨ ○ ○ ○ ○ ○ ○ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌ ◌  ┠₂      DMG dealt: 0
	// ₃┨  1 ○ ○ ○ ○ ○ ◌ ◌ ▦ ▦ ◌ ◌ ◌ ◌ 1 ┠₃   Units killed: 0
	// ...

	for(int i = 0; i <= lines.size(); i++)
	{
		std::string name;
		std::string value;

		switch(i)
		{
			case 1:
				name = "Player";
				if(ended)
					value = "";
				else
					value = myplayer->side == BattleSide::LEFT_SIDE ? (redcol + "RED").append(nocol) : (bluecol + "BLUE").append(nocol);
				break;
			case 2:
				name = "Round";
				value = std::to_string(gnode->attr(GA::BATTLE_ROUND));
				break;
			case 3:
				name = "Last action";
				value = action ? action->humanName(state->battle.battleGetMySide()) : "";
				break;
			case 4:
			{
				// XXX: if there's a draw, this text will be incorrect
				auto restext = gnode->attr(GA::BATTLE_WINNER) ? (bluecol + "BLUE WINS") : (redcol + "RED WINS");

				name = "Battle result";
				value = ended ? (restext + nocol) : "";
			}
			break;
			default:
				continue;
		}

		lines.at(tablestartrow + i) << PadLeft(name, 17, ' ') << ": " << value;
	}

	lines.emplace_back() << "";

	//
	// 5. Add stacks table:
	//
	//          Stack # |   0   1   2   3   4   5   6   A   B   C   0   1   2   3   4   5   6   A   B   C
	// -----------------+--------------------------------------------------------------------------------
	//              Qty |   0  34   0   0   0   0   0   0   0   0   0  17   0   0   0   0   0   0   0   0
	//           Attack |   0   8   0   0   0   0   0   0   0   0   0   6   0   0   0   0   0   0   0   0
	//    ...10 more... | ...
	// -----------------+--------------------------------------------------------------------------------
	//
	// table with 24 columns (1 header, 3 dividers, 10 stacks per side)
	// Each row represents a separate attribute

	// max to show
	constexpr int max_stacks_per_side = 10;

	// All cell text is aligned right
	auto colwidths = std::array<int, 4 + (2 * max_stacks_per_side)>{};
	colwidths.fill(5); // default col width
	colwidths.at(0) = 16; // header col

	// Divider column indexes
	auto divcolids = {1, max_stacks_per_side + 2, (2 * max_stacks_per_side) + 3};

	for(int i : divcolids)
		colwidths.at(i) = 2; // divider col

	enum class Col : uint8_t
	{
		ALIAS,
		QUANTITY,
		ATTACK,
		DEFENSE,
		SHOTS,
		DMG_MIN,
		DMG_MAX,
		HP,
		HP_LEFT,
		SPEED,
		QUEUE,
		VALUE_REL,
		STATE,
		DIV,
		_count
	};

	// {Attribute, name, colwidth}
	const auto rowdefs = std::vector<std::pair<Col, std::string>>{
		{Col::ALIAS,     "Stack #"         },
		{Col::DIV,       ""                },
		{Col::QUANTITY,  "Qty"             },
		{Col::ATTACK,    "Attack"          },
		{Col::DEFENSE,   "Defense"         },
		{Col::SHOTS,     "Shots"           },
		{Col::DMG_MIN,   "Dmg (min)"       },
		{Col::DMG_MAX,   "Dmg (max)"       },
		{Col::HP,        "HP"              },
		{Col::HP_LEFT,   "HP left"         },
		{Col::SPEED,     "Speed"           },
		{Col::QUEUE,     "Queue"           },
		{Col::VALUE_REL, "       Value (‰)"}, // manually pad to 16 (unicode length issue)
		{Col::STATE,     "State"           }, // "WAR" = CAN_WAIT, WILL_ACT, CAN_RETAL
		{Col::DIV,       ""                },
	};

	// Table with nrows and ncells, each cell a 3-element tuple
	// cell: color, width, txt
	using TableCell = std::tuple<std::string, int, std::string>;
	using TableRow = std::array<TableCell, colwidths.size()>;

	auto table = std::vector<TableRow>{};

	auto divrow = TableRow{};
	for(int i = 0; i < colwidths.size(); i++)
		divrow[i] = {nocol, colwidths.at(i), std::string(colwidths.at(i), '-')};

	for(int i : divcolids)
		divrow.at(i) = {nocol, colwidths.at(i), std::string(colwidths.at(i) - 1, '-') + "+"};

	auto queue = BuildQueue(G);

	auto sortedunits = std::array<std::array<std::pair<UnitPtr, HexPtr>, max_stacks_per_side>, 2>{};

	// Attribute rows
	for(const auto & [column, name] : rowdefs)
	{
		if(column == Col::DIV)
		{ // divider row
			table.push_back(divrow);
			continue;
		}

		auto row = TableRow{};

		// Header col
		row.at(0) = {nocol, colwidths.at(0), name};

		// Div cols
		for(int i : {1, 2 + max_stacks_per_side, static_cast<int>(colwidths.size() - 1)})
			row.at(i) = {nocol, colwidths.at(i), "|"};

		// Stack cols
		for(auto side : {BattleSide::LEFT_SIDE, BattleSide::RIGHT_SIDE})
		{
			auto & sideunits = sortedunits.at(EU(side));
			auto extracounter = 0;
			for(const auto & [unit, hex] : seenunits)
			{
				const auto & cstack = unit->cstack;
				if(cstack.unitSide() == side)
				{
					int slot;
					if(unit->alias == "0" || unit->alias == "1" || unit->alias == "2" || unit->alias == "3" || unit->alias == "4" || unit->alias == "5"
					   || unit->alias == "6")
					{
						slot = std::stoi(unit->alias);
					}
					else
					{
						slot = 7 + extracounter;
						extracounter += 1;
					}

					// XXX: this needs to be reworked....
					if(slot < max_stacks_per_side)
						sideunits.at(slot) = {unit, hex};
				}
			}

			for(int i = 0; i < sideunits.size(); ++i)
			{
				const auto & [unit, hex] = sideunits.at(i);
				auto colid = 2 + i + EU(side) + (max_stacks_per_side * EU(side));

				if(!unit)
				{
					row.at(colid) = {nocol, colwidths.at(colid), ""};
					continue;
				}

				std::string value;

				auto color = unit->cstack.unitSide() == BattleSide::LEFT_SIDE ? redcol : bluecol;
				const auto & cstack = unit->cstack;

				switch(column)
				{
					case Col::ALIAS:
						value = unit->alias;
						break;
					case Col::QUANTITY:
						value = std::to_string(cstack.getCount());
						break;
					case Col::ATTACK:
						value = std::to_string(cstack.getAttack(false));
						break;
					case Col::DEFENSE:
						value = std::to_string(cstack.getDefense(false));
						break;
					case Col::SHOTS:
						value = std::to_string(unit->attr(UA::SHOTS));
						break;
					case Col::DMG_MIN:
						value = std::to_string(cstack.getMinDamage(false));
						break;
					case Col::DMG_MAX:
						value = std::to_string(cstack.getMaxDamage(false));
						break;
					case Col::HP:
						value = std::to_string(cstack.unitType()->getMaxHealth());
						break;
					case Col::HP_LEFT:
						value = std::to_string(cstack.getFirstHPleft());
						break;
					case Col::SPEED:
						value = std::to_string(cstack.getMovementRange());
						break;
					case Col::QUEUE:
						if(ended || !queue.contains(unit))
							value = "-";
						else
							value = std::to_string(queue.at(unit));
						break;
					case Col::VALUE_REL:
						value = std::to_string(unit->attr(UA::VALUE_REL));
						break;
					case Col::STATE:
						value = cstack.waitedThisTurn ? "" : "W";
						value += cstack.willMove() ? "A" : "";
						value += cstack.ableToRetaliate() ? "R" : "";
						break;
					case Col::DIV:
						break;
					default:
						throw std::runtime_error("unexpected column: " + std::to_string(EU(column)));
				}

				if(unit == aunit && !ended)
					color += activemod;

				row.at(colid) = {color, colwidths.at(colid), value};
			}
		}

		table.push_back(row);
	}

	for(const auto & r : table)
	{
		auto line = std::stringstream();
		for(const auto & [color, width, txt] : r)
			line << color << PadLeft(txt, width, ' ') << nocol;

		lines.push_back(std::move(line));
	}

	// Exchange simulation table (my attacking units only)
	//
	//  Sim |      0      1      2    ...
	// -----+----------------------------------------
	//    0 |    250    -51     74    ...
	//    1 |     11  -1000      3    ...
	//  ... |    ...    ...    ...    ...
	// -----+-----------------------------------------
	//
	int w = 7; // width

	const auto & redunits = sortedunits.at(0);
	const auto & blueunits = sortedunits.at(1);

	std::vector<int> redidxs;
	std::vector<int> blueidxs;

	for(int i = 0; i < static_cast<int>(redunits.size()); ++i)
	{
		const auto & [unit, _hex] = redunits.at(i);
		if(unit)
			redidxs.push_back(i);
	}

	for(int i = 0; i < static_cast<int>(blueunits.size()); ++i)
	{
		const auto & [unit, _hex] = blueunits.at(i);
		if(unit)
			blueidxs.push_back(i);
	}

	{
		std::stringstream line;
		line << " Sim |" << bluecol;

		for(int idx : blueidxs)
			line << PadLeft(std::to_string(idx), w);

		line << nocol;
		lines.push_back(std::move(line));
	}

	const int table_width = w * static_cast<int>(blueidxs.size());

	std::stringstream divline;
	divline << "-----+" << PadLeft("", table_width, '-');
	const std::string divlinestr = divline.str();

	lines.emplace_back(divlinestr);

	for(int redidx : redidxs)
	{
		const auto & [redunit, _1] = redunits.at(redidx);
		std::stringstream line;
		line << redcol;

		if(redunit->isActive)
			line << activemod;
		else
			line << nocol;

		line << PadLeft(std::to_string(redidx), 4) << " |";

		if(redunit->isActive)
			line << activemod;
		else
			line << nocol;

		for(int blueidx : blueidxs)
		{
			const auto & [blueunit, _2] = blueunits.at(blueidx);
			const auto & edge = G->getEdgeBySrcDst<E::Unit_MeleeDmg_Unit>(redunit, blueunit, false);

			if(!edge)
			{
				line << PadLeft("", w);
				continue;
			}

			auto net = edge->attr(E::Unit_MeleeDmg_Unit::A::ESTIMATED_NET_VALUE_REL_BF);

			line << PadLeft(std::to_string(net), w);
		}

		line << nocol;
		lines.push_back(std::move(line));
	}

	lines.emplace_back(divlinestr);

	//
	// 7. Join rows into a single string
	//
	std::string res = lines[0].str();
	for(int i = 1; i < lines.size(); i++)
		res += "\n" + lines[i].str();

	return res;
}
}
