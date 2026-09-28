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

namespace
{

std::string stackCell(const CStack & stack, bool isActive, bool ansi)
{
	char side = stack.unitSide() == BattleSide::DEFENDER ? 'd' : 'a';
	if(isActive && !ansi)
		side = '*';

	std::string cell;
	cell += side;
	cell += stack.unitType()->getNameSingularTranslated().substr(0, 3);
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
			marker = 'K';
			break;
		case EWallPart::UPPER_TOWER:
		case EWallPart::BOTTOM_TOWER:
			marker = 'T';
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

	for(const auto & obstacle : battle.obstacles)
	{
		if(!obstacle->pos.isValid())
			continue;

		char marker = '#';
		if(obstacle->obstacleType == CObstacleInstance::SPELL_CREATED)
			marker = '%';
		else if(obstacle->obstacleType == CObstacleInstance::MOAT)
			marker = '~';

		markerAt[obstacle->pos.toInt()] = marker;
	}

	for(const auto & stack : battle.stacks)
	{
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
	frame += ", active: " + (activeStack ? activeStack->getName() : "-") + "\n";

	frame += "\n";

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

		frame += "\n";
	}

	frame += "\n";

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
		frame += "\n";
	}

	frame += "\n";

	for(const std::string & entry : logTail)
		frame += "| " + entry + "\n";

	return frame;
}

std::string renderBattleSummary(const BattleID & id, const PlayerColor & victor, RenderOptions options)
{
	std::string frame;
	if(options.ansi)
		frame += "\x1b[2J\x1b[H";

	frame += "VCMI battle mirror - battle #" + std::to_string(id.getNum()) + "\n";
	frame += "Battle ended, victor: player " + std::to_string(victor.getNum()) + "\n";
	return frame;
}

std::string renderBattleCancelled(const BattleID & id, RenderOptions options)
{
	std::string frame;
	if(options.ansi)
		frame += "\x1b[2J\x1b[H";

	frame += "VCMI battle mirror - battle #" + std::to_string(id.getNum()) + "\n";
	frame += "Battle cancelled.\n";
	return frame;
}

}
