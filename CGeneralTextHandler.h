#ifndef CGENERALTEXTHANDLER_H
#define CGENERALTEXTHANDLER_H

#include <string>

class CGeneralTextHandler
{
public:
	std::string cantAddManager, experienceLimitScenarioReached, heroExperienceInfo, perDay, or, somethingVanquished, lastTownLostInfo, heroesAbandonedYou, heroesAbandonedHim;
	std::string couldNotSaveGame, errorOpeningFile, newgameUppercase, sureToDismissArmy, playersTurn, errorReceivingDataKeepTrying, somethingTheSomething, recruit, noRoomInGarrision, numberOFAdventuringHeroes, heroWithoutCreatures;
	std::string videoQuality, itemCantBeTraded, sureDismissHero, selectSpellTarget, invalidTeleportDestination, teleportHere, castSomething, castSomethingOnSomething, sureRetreat, notEnoughGold, capturedEnemyArtifact, none;
	std::string surrenderProposal, highMoraleNextAttack, lowMoraleFreeze, makeRoomInArmyForSomething, attackSomethingSomethingDamage, shootSomethingOneShootLeftSomethingDamage;
	std::string campaignDescription, somethingIsActive, sessionName, userName, creature, creatures, badLuckOnSomething, goodLuckOnSomething, treasure, somethingSomethingsUnderAttack, town, hero, townScreen, cannotBuildBoat, requires, systemUppercase;

	void loadToIt(std::string & dest, std::string & src, int & iter, int mode = 0); //mode 0 - dump to tab, dest to tab, dump to eol //mode 1 - dump to tab, src to eol //mode 2 - copy to tab, dump to eol //mode 3 - copy to eol
	void load();
};


#endif //CGENERALTEXTHANDLER_H