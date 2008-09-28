#pragma once
struct BattleAction
{
	ui8 side; //who made this action: false - left, true - right player
	ui32 stackNumber;//stack ID, -1 left hero, -2 right hero,
	ui8 actionType; //    0 = Cancel BattleAction   1 = Hero cast a spell   2 = Walk   3 = Defend   4 = Retreat from the battle   5 = Surrender   6 = Walk and Attack   7 = Shoot    8 = Wait   9 = Catapult 10 = Monster casts a spell (i.e. Faerie Dragons)
	ui16 destinationTile;
	si32 additionalInfo; // e.g. spell number if type is 1 || 10; tile to attack if type is 6
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side & stackNumber & actionType & destinationTile & additionalInfo;
	}
};