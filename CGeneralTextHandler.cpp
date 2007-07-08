#include "CGeneralTextHandler.h"
#include <fstream>

void CGeneralTextHandler::load()
{
	std::ifstream inp("H3bitmap.lod\\GENRLTXT.TXT", std::ios::in|std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	inp.close();
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i=0; //buf iterator
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}
	i+=2;
	loadToIt(cantAddManager, buf, i, 2);
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

