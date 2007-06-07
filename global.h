#ifndef GLOBAL_H
#define GLOBAL_H
#define CHECKTIME 1
#if CHECKTIME
#include "timeHandler.h"
#include <iostream>
#define THC
#else 
#define THC //
#endif
enum EterrainType {dirt, sand, grass, snow, swamp, rough, subterranean, lava, water, rock};
enum Eriver {clearRiver=1, icyRiver, muddyRiver, lavaRiver};
enum Eroad {dirtRoad=1, gravelRoad, cobblestoneRoad};
enum Eformat { WoG=0x33, AB=0x15, RoE=0x0e,  SoD=0x1c};
enum EvictoryConditions {artifact, gatherTroop, gatherResource, buildCity, buildGrail, beatHero, 
captureCity, beatMonster, takeDwellings, takeMines, transportItem, winStandard=255};
enum ElossCon {lossCastle, lossHero, timeExpires, lossStandard=255};

#define DEFBYPASS
#endif //GLOBAL_H