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
enum Ecolor {RED, BLUE, TAN, GREEN, ORANGE, PURPLE, TEAL, PINK};
enum EterrainType {dirt, sand, grass, snow, swamp, rough, subterranean, lava, water, rock};
enum Eriver {noRiver=0, clearRiver, icyRiver, muddyRiver, lavaRiver};
enum Eroad {dirtRoad=1, grazvelRoad, cobblestoneRoad};
enum Eformat { WoG=0x33, AB=0x15, RoE=0x0e,  SoD=0x1c};
enum EvictoryConditions {artifact, gatherTroop, gatherResource, buildCity, buildGrail, beatHero, 
	captureCity, beatMonster, takeDwellings, takeMines, transportItem, winStandard=255};
enum ElossCon {lossCastle, lossHero, timeExpires, lossStandard=255};
enum EHeroClasses {HERO_KNIGHT, HERO_CLERIC, HERO_RANGER, HERO_DRUID, HREO_ALCHEMIST, HERO_WIZARD, 
	HERO_DEMONIAC, HERO_HERETIC, HERO_DEATHKNIGHT, HERO_NECROMANCER, HERO_WARLOCK, HERO_OVERLORD, 
	HERO_BARBARIAN, HERO_BATTLEMAGE, HERO_BEASTMASTER, HERO_WITCH, HERO_PLANESWALKER, HERO_ELEMENTALIST};
#define CURPLINT (((CPlayerInterface*)(CGI->playerint[CGI->state->currentPlayer]))) //interface of current player (only human)
const int F_NUMBER = 9; //factions quantity
const int PLAYER_LIMIT = 8; //player limit per map
const int HEROES_PER_TYPE=8; //amount of heroes of each type
const int SKILL_QUANTITY=28;
const int ARTIFACTS_QUANTITY=171;
const int HEROES_QUANTITY=156;



#define DEFBYPASS
#endif //GLOBAL_H