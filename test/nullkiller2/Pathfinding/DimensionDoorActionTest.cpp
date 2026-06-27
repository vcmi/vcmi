/*
 * DimensionDoorActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Pathfinding/Actions/DimensionDoorAction.h"
#include "AI/Nullkiller2/Pathfinding/AINodeStorage.h"

namespace
{
using namespace NK2AI;
using namespace NK2AI::AIPathfinding;

DimensionDoorActionParameters makeActionParameters()
{
	DimensionDoorActionParameters parameters;
	parameters.usedSpell = SpellID(8);
	parameters.destination = int3(7, 8, 0);
	parameters.manaCost = 7;
	parameters.movementPointsRequired = 100;
	parameters.movementPointsTaken = 600;
	parameters.plannedSourceTurn = 0;
	parameters.plannedSourceMoveRemains = 500;
	parameters.plannedSourceMoveLimit = 1000;
	parameters.plannedDimensionDoorCasts = 2;
	parameters.guardedLandingDanger = 1234;
	parameters.guardedLandingArmyLoss = 56;
	return parameters;
}
}

TEST(Nullkiller2_Pathfinding_DimensionDoorAction, appliesCostAndResourceAccountingToDestination)
{
	AIPathNode sourceNode;
	sourceNode.setCost(1.25f);
	sourceNode.manaCost = 5;
	sourceNode.armyLoss = 10;

	AIPathNode destinationNode;
	PathNodeInfo source;
	source.node = &sourceNode;
	CDestinationNodeInfo destination;

	DimensionDoorAction action(makeActionParameters());
	action.applyOnDestination(nullptr, destination, source, &destinationNode, &sourceNode);

	EXPECT_EQ(destinationNode.manaCost, 12);
	EXPECT_EQ(destinationNode.dimensionDoorCasts, 3);
	EXPECT_EQ(destinationNode.theNodeBefore, &sourceNode);
	EXPECT_EQ(destinationNode.moveRemains, 0);
	EXPECT_FLOAT_EQ(destinationNode.getCost(), 1.75f);
	EXPECT_EQ(destinationNode.armyLoss, 56);
	EXPECT_EQ(destinationNode.danger, 1234);

	EXPECT_FLOAT_EQ(destination.cost, destinationNode.getCost());
	EXPECT_EQ(destination.movementLeft, destinationNode.moveRemains);
}

TEST(Nullkiller2_Pathfinding_DimensionDoorAction, keepsRunwayAfterCastForSameDayContinuation)
{
	auto parameters = makeActionParameters();
	parameters.plannedSourceMoveRemains = 1000;
	parameters.plannedSourceMoveLimit = 1000;
	parameters.movementPointsTaken = 300;
	parameters.guardedLandingDanger = 0;
	parameters.guardedLandingArmyLoss = 0;

	AIPathNode sourceNode;
	sourceNode.setCost(2.0f);

	AIPathNode destinationNode;
	PathNodeInfo source;
	source.node = &sourceNode;
	CDestinationNodeInfo destination;

	DimensionDoorAction action(parameters);
	action.applyOnDestination(nullptr, destination, source, &destinationNode, &sourceNode);

	EXPECT_EQ(destinationNode.moveRemains, 700);
	EXPECT_FLOAT_EQ(destinationNode.getCost(), 2.3f);
	EXPECT_FLOAT_EQ(destination.cost, destinationNode.getCost());
	EXPECT_EQ(destination.movementLeft, 700);
}
