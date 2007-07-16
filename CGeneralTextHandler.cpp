#include "stdafx.h"
#include "CGeneralTextHandler.h"
#include "CGameInfo.h"
#include <fstream>

void CGeneralTextHandler::load()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("GENRLTXT.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	i+=2;
	for(int jj=0; jj<764; ++jj)
	{
		std::string buflet;
		loadToIt(buflet, buf, i, 2);
		allTexts.push_back(buflet);
	}
	/*loadToIt(cantAddManager, buf, i, 2);
	loadToIt(experienceLimitScenarioReached, buf, i, 2);
	loadToIt(heroExperienceInfo, buf, i, 2);
	loadToIt(perDay, buf, i, 2);
	loadToIt(or, buf, i, 2);
	loadToIt(somethingVanquished, buf, i, 2);
	loadToIt(lastTownLostInfo, buf, i, 2);
	loadToIt(heroesAbandonedYou, buf, i, 2);
	loadToIt(heroesAbandonedHim, buf, i, 2);
	loadToIt(couldNotSaveGame, buf, i, 2);
	loadToIt(errorOpeningFile, buf, i, 2);
	loadToIt(newgameUppercase, buf, i, 2);
	loadToIt(sureToDismissArmy, buf, i, 2);
	loadToIt(playersTurn, buf, i, 2);
	loadToIt(errorReceivingDataKeepTrying, buf, i, 2);
	loadToIt(somethingTheSomething, buf, i, 2);
	loadToIt(recruit, buf, i, 2);
	loadToIt(noRoomInGarrision, buf, i, 2);
	loadToIt(numberOFAdventuringHeroes, buf, i, 2);
	loadToIt(heroWithoutCreatures, buf, i, 2);
	loadToIt(videoQuality, buf, i, 2);
	loadToIt(itemCantBeTraded, buf, i, 2);
	loadToIt(sureDismissHero, buf, i, 2);
	loadToIt(selectSpellTarget, buf, i, 2);
	loadToIt(invalidTeleportDestination, buf, i, 2);
	loadToIt(teleportHere, buf, i, 2);
	loadToIt(castSomething, buf, i, 2);
	loadToIt(castSomethingOnSomething, buf, i, 2);
	loadToIt(sureRetreat, buf, i, 2);
	loadToIt(notEnoughGold, buf, i, 2);
	loadToIt(capturedEnemyArtifact, buf, i, 2);
	loadToIt(none, buf, i, 2);
	loadToIt(surrenderProposal, buf, i, 2);
	loadToIt(highMoraleNextAttack, buf, i, 2);
	loadToIt(lowMoraleFreeze, buf, i, 2);
	loadToIt(makeRoomInArmyForSomething, buf, i, 2);
	loadToIt(attackSomethingSomethingDamage, buf, i, 2);
	loadToIt(shootSomethingOneShootLeftSomethingDamage, buf, i, 2);
	loadToIt(campaignDescription, buf, i, 2);
	loadToIt(somethingIsActive, buf, i, 2);
	loadToIt(sessionName, buf, i, 2);
	loadToIt(userName, buf, i, 2);
	loadToIt(creature, buf, i, 2);
	loadToIt(creatures, buf, i, 2);
	loadToIt(badLuckOnSomething, buf, i, 2);
	loadToIt(goodLuckOnSomething, buf, i, 2);
	loadToIt(treasure, buf, i, 2);
	loadToIt(somethingSomethingsUnderAttack, buf, i, 2);
	loadToIt(town, buf, i, 2);
	loadToIt(hero, buf, i, 2);
	loadToIt(townScreen, buf, i, 2);
	loadToIt(cannotBuildBoat, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(systemUppercase, buf, i, 2);
	loadToIt(worldMapHelp, buf, i, 2);
	loadToIt(sureEndTurnHeroMayMove, buf, i, 2);
	loadToIt(diggingArtifactWholeDay, buf, i, 2);
	loadToIt(noRoomForArtifact, buf, i, 2);
	loadToIt(heroUncoveredThe, buf, i, 2);
	loadToIt(diggingNothing, buf, i, 2);
	loadToIt(tryLookingOnLand, buf, i, 2);
	loadToIt(unchartedTerritory, buf, i, 2);
	loadToIt(month, buf, i, 2);
	loadToIt(week, buf, i, 2);
	loadToIt(day, buf, i, 2);
	loadToIt(enemyViewTip, buf, i, 2);
	loadToIt(pingInfo, buf, i, 2);
	loadToIt(sureToRestart, buf, i, 2);
	loadToIt(sureToStartNewGame, buf, i, 2);
	loadToIt(sureQuit, buf, i, 2);
	loadToIt(dimensionDoorFiled, buf, i, 2);
	loadToIt(chooseBonus, buf, i, 2);
	loadToIt(ping, buf, i, 2);
	loadToIt(pingSomething, buf, i, 2);
	loadToIt(pingEveryone, buf, i, 2);
	loadToIt(score, buf, i, 2);
	loadToIt(autosaveUppercase, buf, i, 2);
	loadToIt(startingHeroB, buf, i, 2);
	loadToIt(heroSpecialityB, buf, i, 2);
	loadToIt(associatedCreaturesB, buf, i, 2);
	loadToIt(townAlignmentB, buf, i, 2);
	loadToIt(errorSendingDataKeepTrying, buf, i, 2);
	loadToIt(tooFewPalyersForMultiGame, buf, i, 2);
	loadToIt(artifactBonusB, buf, i, 2);
	loadToIt(goldBonusB, buf, i, 2);
	loadToIt(resourceBonusB, buf, i, 2);
	loadToIt(randomBonusB, buf, i, 2);
	loadToIt(fiveHundredToOneThousand, buf, i, 2);
	loadToIt(armageddonDamages, buf, i, 2);
	loadToIt(woodOreBonus, buf, i, 2);
	loadToIt(randomlyChoosenArtifact, buf, i, 2);
	loadToIt(disruptingRayReducesDefence, buf, i, 2);
	loadToIt(goldStartingBonus, buf, i, 2);
	loadToIt(woodOreStartingBonus, buf, i, 2);
	loadToIt(randomStartingBonus, buf, i, 2);
	loadToIt(youEliminatedFormGame, buf, i, 2);
	loadToIt(scoreToHallOfFame, buf, i, 2);
	loadToIt(trySearchingOnClearGround, buf, i, 2);
	loadToIt(sendingData, buf, i, 2);
	loadToIt(receivingData, buf, i, 2);
	loadToIt(chaosMp2, buf, i, 2);
	loadToIt(randomHeroB, buf, i, 2);
	loadToIt(randomStartingHeroInfo, buf, i, 2);
	loadToIt(randomTownB, buf, i, 2);
	loadToIt(randomStartingTownInfo, buf, i, 2);
	loadToIt(somethingSurrendersInfo, buf, i, 2);
	loadToIt(heroesCDNotFound, buf, i, 2);
	loadToIt(autosaving, buf, i, 2);
	loadToIt(playerexitUppercase, buf, i, 2);
	loadToIt(statusWindowTip, buf, i, 2);
	loadToIt(border, buf, i, 2);
	loadToIt(somethingAbsorbsMana, buf, i, 2);
	loadToIt(somethingsAbsorbMana, buf, i, 2);
	loadToIt(unableChangeHeroesDirectory, buf, i, 2);
	loadToIt(unableFindHeoresDataFiles, buf, i, 2);
	loadToIt(victoryAchievementText, buf, i, 2);
	loadToIt(somethingsRiseFromDeath, buf, i, 2);
	loadToIt(somethingRisesFormDeath, buf, i, 2);
	loadToIt(somethingDiesUnderGaze, buf, i, 2);
	loadToIt(somethingsDieUnderGaze, buf, i, 2);
	loadToIt(somethingTakesDefensiveStance, buf, i, 2);
	loadToIt(somethingsTakeDefensiveStance, buf, i, 2);
	loadToIt(somethingExp, buf, i, 2);
	loadToIt(nearestTownOccupied, buf, i, 2);
	loadToIt(noAvailableTown, buf, i, 2);
	loadToIt(heroTooTiredSpell, buf, i, 2);
	loadToIt(townGateCannotBeUsed, buf, i, 2);
	loadToIt(youHaveNoHeroes, buf, i, 2);
	loadToIt(heroHasCastSpell, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);
	loadToIt(requires, buf, i, 2);*/
}


void CGeneralTextHandler::loadToIt(std::string &dest, std::string &src, int &iter, int mode)
{
	switch(mode)
	{
	case 0:
		{
			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					++hmcr;
				if(hmcr==1)
					break;
			}
			++iter;

			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					break;
			}
			dest = src.substr(befi, iter-befi);
			++iter;

			hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					++hmcr;
				if(hmcr==1)
					break;
			}
			iter+=2;
			break;
		}
	case 1:
		{
			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					++hmcr;
				if(hmcr==1)
					break;
			}
			++iter;

			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					break;
			}
			dest = src.substr(befi, iter-befi);
			iter+=2;
			break;
		}
	case 2:
		{
			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					break;
			}
			dest = src.substr(befi, iter-befi);
			++iter;

			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					++hmcr;
				if(hmcr==1)
					break;
			}
			iter+=2;
			break;
		}
	case 3:
		{
			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					break;
			}
			dest = src.substr(befi, iter-befi);
			iter+=2;
			break;
		}
	}
}

