#ifndef CGENERALTEXTHANDLER_H
#define CGENERALTEXTHANDLER_H

#include <string>
#include <vector>

class CGeneralTextHandler //Handles general texts
{
public:
	std::vector<std::string> allTexts;
	/*std::string cantAddManager, experienceLimitScenarioReached, heroExperienceInfo, perDay, or, somethingVanquished, lastTownLostInfo, heroesAbandonedYou, heroesAbandonedHim;
	std::string couldNotSaveGame, errorOpeningFile, newgameUppercase, sureToDismissArmy, playersTurn, errorReceivingDataKeepTrying, somethingTheSomething, recruit, noRoomInGarrision, numberOFAdventuringHeroes, heroWithoutCreatures;
	std::string videoQuality, itemCantBeTraded, sureDismissHero, selectSpellTarget, invalidTeleportDestination, teleportHere, castSomething, castSomethingOnSomething, sureRetreat, notEnoughGold, capturedEnemyArtifact, none;
	std::string surrenderProposal, highMoraleNextAttack, lowMoraleFreeze, makeRoomInArmyForSomething, attackSomethingSomethingDamage, shootSomethingOneShootLeftSomethingDamage;
	std::string campaignDescription, somethingIsActive, sessionName, userName, creature, creatures, badLuckOnSomething, goodLuckOnSomething, treasure, somethingSomethingsUnderAttack, town, hero, townScreen, cannotBuildBoat, requires, systemUppercase;
	std::string worldMapHelp, sureEndTurnHeroMayMove, diggingArtifactWholeDay, noRoomForArtifact, heroUncoveredThe, diggingNothing, tryLookingOnLand, unchartedTerritory;
	std::string month, week, day, enemyViewTip, pingInfo, sureToRestart, sureToStartNewGame, sureQuit, dimensionDoorFiled, chooseBonus, ping, pingSomething, pingEveryone, score, autosaveUppercase;
	std::string startingHeroB, heroSpecialityB, associatedCreaturesB, townAlignmentB, errorSendingDataKeepTrying, tooFewPalyersForMultiGame, artifactBonusB, goldBonusB, resourceBonusB, randomBonusB;
	std::string fiveHundredToOneThousand, armageddonDamages, woodOreBonus, randomlyChoosenArtifact, disruptingRayReducesDefence, goldStartingBonus, woodOreStartingBonus, randomStartingBonus;
	std::string youEliminatedFormGame, scoreToHallOfFame, trySearchingOnClearGround, sendingData, receivingData, chaosMp2, randomHeroB, randomStartingHeroInfo, randomTownB, randomStartingTownInfo, somethingSurrendersInfo;
	std::string heroesCDNotFound, autosaving, playerexitUppercase, statusWindowTip, border, somethingAbsorbsMana, somethingsAbsorbMana, unableChangeHeroesDirectory, unableFindHeoresDataFiles, victoryAchievementText;
	std::string somethingsRiseFromDeath, somethingRisesFormDeath, somethingDiesUnderGaze, somethingsDieUnderGaze, somethingTakesDefensiveStance, somethingsTakeDefensiveStance, somethingExp, nearestTownOccupied, noAvailableTown, heroTooTiredSpell, townGateCannotBeUsed, youHaveNoHeroes, heroHasCastSpell;
	std::string doYouWishToSaveSomethingsArmy, problemsWithInputDevices, problemsWithSound, problemsWithMouse, problemsWithWindows, abandonedShipyard, spellFailed, somethingPauses, somethingsPause, somethingLevelSomething, somethingStudiedMagic, learnsSpeced, andSpaced, fromSomethingSpaced;*/

	static void loadToIt(std::string & dest, std::string & src, int & iter, int mode = 0); //mode 0 - dump to tab, dest to tab, dump to eol //mode 1 - dump to tab, src to eol //mode 2 - copy to tab, dump to eol //mode 3 - copy to eol //mode 4 - copy to tab
	void load();
};


#endif //CGENERALTEXTHANDLER_H