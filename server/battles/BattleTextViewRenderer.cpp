/*
 * BattleTextViewRenderer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleTextViewRenderer.h"

#include "../../lib/CStack.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/battle/CObstacleInstance.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace battleTextView
{

std::string abbreviateName(std::string_view name, size_t maxCodePoints)
{
	size_t byte = 0;
	for(size_t codePoint = 0; codePoint < maxCodePoints && byte < name.size(); ++codePoint)
	{
		++byte;
		while(byte < name.size() && (static_cast<unsigned char>(name[byte]) & 0xC0) == 0x80)
			++byte;
	}

	return std::string(name.substr(0, byte));
}

}

namespace
{

std::string stackCell(const CStack & stack, bool isActive, bool ansi)
{
	char side = stack.unitSide() == BattleSide::DEFENDER ? 'd' : 'a';
	if(isActive && !ansi)
		side = '*';

	std::string cell;
	cell += side;
	cell += battleTextView::abbreviateName(stack.unitType()->getNameSingularTranslated(), 3);
	if(cell.size() < 4)
		cell.append(4 - cell.size(), ' ');

	if(isActive && ansi)
		return "\x1b[7m" + cell + "\x1b[27m";
	return cell;
}

char gateMarker(EGateState state)
{
	switch(state)
	{
	case EGateState::OPENED:
		return 'g';
	case EGateState::DESTROYED:
		return 'X';
	default:
		return 'G';
	}
}

char wallMarker(EWallState state)
{
	switch(state)
	{
	case EWallState::DESTROYED:
		return 'X';
	case EWallState::DAMAGED:
		return 'x';
	case EWallState::INTACT:
		return '=';
	default:
		return 'H';
	}
}

}

namespace battleTextView
{

std::string renderBattleTextView(const BattleInfo & battle, const std::vector<std::string> & logTail, RenderOptions options)
{
	std::array<const CStack *, GameConstants::BFIELD_SIZE> stackAt{};
	std::array<char, GameConstants::BFIELD_SIZE> markerAt{};

	const CStack * activeStack = nullptr;
	int32_t activeStackID = battle.getActiveStackID();
	if(activeStackID >= 0)
	{
		for(const auto & stack : battle.stacks)
			if(stack->unitId() == static_cast<uint32_t>(activeStackID))
			{
				activeStack = stack.get();
				break;
			}
	}

	if(battle.getSideHero(BattleSide::ATTACKER))
		markerAt[BattleHex::HERO_ATTACKER] = 'A';
	if(battle.getSideHero(BattleSide::DEFENDER))
		markerAt[BattleHex::HERO_DEFENDER] = 'D';

	for(int part = static_cast<int>(EWallPart::KEEP); part <= static_cast<int>(EWallPart::GATE); ++part)
	{
		auto wallPart = static_cast<EWallPart>(part);
		EWallState wallState = battle.getWallState(wallPart);
		if(wallState == EWallState::NONE)
			continue;

		char marker;
		switch(wallPart)
		{
		case EWallPart::KEEP:
		case EWallPart::UPPER_TOWER:
		case EWallPart::BOTTOM_TOWER:
			// keep and towers degrade like any wall segment; only their standing marker is distinct
			marker = wallState == EWallState::DESTROYED ? 'X' : (wallPart == EWallPart::KEEP ? 'K' : 'T');
			break;
		case EWallPart::GATE:
			marker = gateMarker(battle.getGateState());
			break;
		default:
			marker = wallMarker(wallState);
			break;
		}

		BattleHex wallHex = battle.wallPartToBattleHex(wallPart);
		if(wallHex.isValid())
			markerAt[wallHex.toInt()] = marker;
	}

	// indestructible segments exist exactly in walled sieges, where the gate part carries a state
	if(battle.getWallState(EWallPart::GATE) != EWallState::NONE)
	{
		// hexes of the engine's wall-part table that no catapult can destroy
		constexpr si16 indestructibleHexes[] = {45, 62, 112, 147, 165, BattleHex::GATE_OUTER};
		for(const si16 hex : indestructibleHexes)
			markerAt[hex] = 'W';

		if(battle.getGateState() == EGateState::OPENED)
			markerAt[BattleHex::GATE_BRIDGE] = '=';
	}

	for(const auto & obstacle : battle.obstacles)
	{
		char marker = '#';
		if(obstacle->obstacleType == CObstacleInstance::SPELL_CREATED)
			marker = '%';
		else if(obstacle->obstacleType == CObstacleInstance::MOAT)
			marker = '~';

		// base-class getAffectedTiles() asserts for MOAT, so only virtual overrides are safe to ask
		// absolute obstacles carry no pos by design - their blocked tiles are absolute hexes rather than offsets
		const bool bareMoat = obstacle->obstacleType == CObstacleInstance::MOAT
			&& !dynamic_cast<const SpellCreatedObstacle *>(obstacle.get());
		const BattleHexArray tiles = bareMoat ? BattleHexArray() : obstacle->getAffectedTiles();

		if(!tiles.empty())
		{
			for(const BattleHex & hex : tiles)
				if(hex.isValid())
					markerAt[hex.toInt()] = marker;
		}
		// a non-empty footprint with no valid hex renders nothing: the pos fallback exists only for
		// obstacles without any footprint at all, not as a rescue for dirty footprints
		else if(obstacle->pos.isValid())
			markerAt[obstacle->pos.toInt()] = marker;
	}

	for(const auto & stack : battle.stacks)
	{
		if(!stack->alive())
			continue;

		BattleHex position = stack->getPosition();
		if(position.isValid())
			stackAt[position.toInt()] = stack.get();

		BattleHex secondHex = stack->occupiedHex();
		if(secondHex.isValid())
			stackAt[secondHex.toInt()] = stack.get();
	}

	std::string frame;
	if(options.ansi)
		frame += "\x1b[2J\x1b[H";

	frame += "VCMI battle mirror - battle #" + std::to_string(battle.getBattleID().getNum());
	frame += ", round " + std::to_string(battle.getRound());
	frame += ", active: " + (activeStack ? activeStack->getName() : "-") + "\r\n";

	frame += "\r\n";

	for(int y = 0; y < GameConstants::BFIELD_HEIGHT; ++y)
	{
		frame += static_cast<char>('0' + y / 10);
		frame += static_cast<char>('0' + y % 10);
		frame += '|';
		if(y % 2 == 1)
			frame += "  ";

		for(int x = 0; x < GameConstants::BFIELD_WIDTH; ++x)
		{
			int hexIndex = x + y * GameConstants::BFIELD_WIDTH;
			if(stackAt[hexIndex])
				frame += stackCell(*stackAt[hexIndex], stackAt[hexIndex] == activeStack, options.ansi);
			else if(markerAt[hexIndex] != 0)
			{
				frame += markerAt[hexIndex];
				frame += "   ";
			}
			else
				frame += "    ";
		}

		frame += "\r\n";
	}

	frame += "\r\n";

	std::vector<const CStack *> livingStacks;
	for(const auto & stack : battle.stacks)
		if(stack->alive())
			livingStacks.push_back(stack.get());

	std::sort(livingStacks.begin(), livingStacks.end(), [](const CStack * lhs, const CStack * rhs)
	{
		return lhs->unitId() < rhs->unitId();
	});

	for(const CStack * stack : livingStacks)
	{
		frame += stack->unitSide() == BattleSide::DEFENDER ? 'd' : 'a';
		frame += " " + stack->getName();
		frame += "  count " + std::to_string(stack->getCount());
		frame += "  HP " + std::to_string(stack->getFirstHPleft()) + "/" + std::to_string(stack->getMaxHealth());
		frame += "\r\n";
	}

	frame += "\r\n";

	for(const std::string & entry : logTail)
		frame += "| " + entry + "\r\n";

	return frame;
}

std::string renderBattleSummary(const BattleID & id, const PlayerColor & victor, RenderOptions options)
{
	std::string frame;
	if(options.ansi)
		frame += "\x1b[2J\x1b[H";

	frame += "VCMI battle mirror - battle #" + std::to_string(id.getNum()) + "\r\n";
	frame += "Battle ended, victor: player " + std::to_string(victor.getNum()) + "\r\n";
	return frame;
}

std::string renderBattleCancelled(const BattleID & id, RenderOptions options)
{
	std::string frame;
	if(options.ansi)
		frame += "\x1b[2J\x1b[H";

	frame += "VCMI battle mirror - battle #" + std::to_string(id.getNum()) + "\r\n";
	frame += "Battle cancelled.\r\n";
	return frame;
}

}
