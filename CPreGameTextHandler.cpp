#include "stdafx.h"
#include "CPreGameTextHandler.h"
#include "CGameInfo.h"
#include "CLodHandler.h"
std::string CPreGameTextHandler::getTitle(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='{'));
	while ((text[i]!='}') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
std::string CPreGameTextHandler::getDescr(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='}'));
	i+=2;
	while ((text[i]!='"') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
void CPreGameTextHandler::loadTexts()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("ZELP.TXT");
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==3)
			break;
	}
	i+=3;

	int befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainNewGame = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainLoadGame = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainHighScores = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainCredits = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainQuit = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==3)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngSingleScenario = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngCampain = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngMultiplayer = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngTutorial = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngBack = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==4)
			break;
	}
	i+=2;
	loadToIt(singleChooseScenario, buf, i);
	loadToIt(singleSetAdvOptions, buf, i);
	loadToIt(singleRandomMap, buf, i);
	loadToIt(singleScenarioName, buf, i);
	loadToIt(singleDescriptionTitle, buf, i);
	loadToIt(singleDescriptionText, buf, i);
	loadToIt(singleEasy, buf, i);
	loadToIt(singleNormal, buf, i);
	loadToIt(singleHard, buf, i);
	loadToIt(singleExpert, buf, i);
	loadToIt(singleImpossible, buf, i);
	loadToIt(singleAllyFlag[0], buf, i);
	loadToIt(singleAllyFlag[1], buf, i);
	loadToIt(singleAllyFlag[2], buf, i);
	loadToIt(singleAllyFlag[3], buf, i, 1);
	loadToIt(singleAllyFlag[4], buf, i, 1);
	loadToIt(singleAllyFlag[5], buf, i, 1);
	loadToIt(singleAllyFlag[6], buf, i, 1);
	loadToIt(singleAllyFlag[7], buf, i, 1);
	loadToIt(singleEnemyFlag[0], buf, i, 1);
	loadToIt(singleEnemyFlag[1], buf, i, 1);
	loadToIt(singleEnemyFlag[2], buf, i, 1);
	loadToIt(singleEnemyFlag[3], buf, i, 1);
	loadToIt(singleEnemyFlag[4], buf, i, 1);
	loadToIt(singleEnemyFlag[5], buf, i, 1);
	loadToIt(singleEnemyFlag[6], buf, i, 1);
	loadToIt(singleEnemyFlag[7], buf, i, 1);
	loadToIt(singleViewHideScenarioList, buf, i, 1);
	loadToIt(singleViewHideAdvOptions, buf, i, 1);
	loadToIt(singlePlayRandom, buf, i, 1);
	loadToIt(singleChatDesc, buf, i, 1);
	loadToIt(singleMapDifficulty, buf, i, 1);
	loadToIt(singleRating, buf, i, 1);
	loadToIt(singleMapPossibleDifficulties, buf, i, 1);
	loadToIt(singleVicCon, buf, i, 1);
	loadToIt(singleLossCon, buf, i, 1);
	loadToIt(singleSFilter, buf, i, 1);
	loadToIt(singleMFilter, buf, i, 1);
	loadToIt(singleLFilter, buf, i, 1);
	loadToIt(singleXLFilter, buf, i, 1);
	loadToIt(singleAllFilter, buf, i, 1);
	for(int vv=0; vv<18; ++vv)
	{
		loadToIt(singleScenarioNameNr[vv], buf, i, 1);
	}
	for(int vv=0; vv<18; ++vv)
	{
		loadToIt(singleEntryScenarioNameNr[vv], buf, i, 1);
	}
	std::string ff = singleEntryScenarioNameNr[4];
	loadToIt(singleTurnDuration, buf, i, 1);
	loadToIt(singleChatText, buf, i, 0);
	loadToIt(singleChatEntry, buf, i, 0);
	loadToIt(singleChatPlug, buf, i, 0);
	loadToIt(singleChatPlayer, buf, i, 0);
	loadToIt(singleChatPlayerSlider, buf, i, 0);
	loadToIt(singleRollover, buf, i, 0);
	loadToIt(singleNext, buf, i, 0);
	loadToIt(singleBegin, buf, i, 0);
	loadToIt(singleBack, buf, i, 0);
	loadToIt(singleSSExit, buf, i, 0);
	loadToIt(singleWhichMap, buf, i, 0);
	loadToIt(singleSortNumber, buf, i, 0);
	loadToIt(singleSortSize, buf, i, 0);
	loadToIt(singleSortVersion, buf, i, 0);
	loadToIt(singleSortAlpha, buf, i, 0);
	loadToIt(singleSortVictory, buf, i, 0);
	loadToIt(singleSortLoss, buf, i, 1);
	loadToIt(singleBriefing, buf, i, 1);
	loadToIt(singleSSHero, buf, i, 1);
	loadToIt(singleGoldpic, buf, i, 1);
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleHumanCPU[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleHandicap[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleTownLeft[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleTownRite[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleHeroLeft[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleHeroRite[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleResLeft[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleResRite[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleHeroSetting[vv], buf, i, 1);
	}
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleTownSetting[vv], buf, i, 1);
	}
	loadToIt(singleConstCreateMap, buf, i, 1);
	loadToIt(singleConstMapSizeLabel, buf, i, 1);
	loadToIt(singleConstSmallMap, buf, i, 1);
	loadToIt(singleConstMediumMap, buf, i, 1);
	loadToIt(singleConstLargeMap, buf, i, 1);
	loadToIt(singleConstHugeMap, buf, i, 1);
	loadToIt(singleConstMapLevels, buf, i, 1);
	loadToIt(singleConstHumanPositionsLabel, buf, i, 1);
	for(int vv=0; vv<8; ++vv)
	{
		loadToIt(singleConstNHumans[vv], buf, i, 1);
	}
	loadToIt(singleConstRandomHumans, buf, i, 1);
	loadToIt(singleConstHumanTeamsLabel, buf, i, 1);
	loadToIt(singleConstNoHumanTeams, buf, i, 1);
	for(int vv=0; vv<7; ++vv)
	{
		loadToIt(singleConstNHumanTeams[vv], buf, i, 1);
	}
	loadToIt(singleConstRandomHumanTeams, buf, i, 1);
	loadToIt(singleConstComputerPositionsLabel, buf, i, 1);
	loadToIt(singleConstNoComputers, buf, i, 1);
	for(int vv=0; vv<7; ++vv)
	{
		loadToIt(singleConstNComputers[vv], buf, i, 1);
	}
	loadToIt(singleConstRandomComputers, buf, i, 1);
	loadToIt(singleConstComputerTeamsLabel, buf, i, 1);
	loadToIt(singleConstNoComputerTeams, buf, i, 1);
	for(int vv=0; vv<6; ++vv)
	{
		loadToIt(singleConstNComputerTeams[vv], buf, i, 1);
	}
	loadToIt(singleConstRandomComputerTeams, buf, i, 1);
	loadToIt(singleConstWaterLabel, buf, i, 1);
	loadToIt(singleConstNoWater, buf, i, 1);
	loadToIt(singleConstNormalWater, buf, i, 1);
	loadToIt(singleConstIslands, buf, i, 1);
	loadToIt(singleConstRandomWater, buf, i, 1);
	loadToIt(singleConstMonsterStrengthLabel, buf, i, 1);
	loadToIt(singleConstWeakMonsters, buf, i, 1);
	loadToIt(singleConstNormalMonsters, buf, i, 1);
	loadToIt(singleConstStrongMonsters, buf, i, 1);
	loadToIt(singleConstRandomMonsters, buf, i, 1);
	loadToIt(singleConstShowSavedRandomMaps, buf, i, 1);
	loadToIt(singleSliderChatWindow, buf, i, 1);
	loadToIt(singleSliderFileMenu, buf, i, 1);
	loadToIt(singleSliderDuration, buf, i, 1);

	loadToIt(singlePlayerHandicapHeaderID, buf, i, 0);
	loadToIt(singleTurnDurationHeaderID, buf, i, 0);
	loadToIt(singleStartingTownHeaderID, buf, i, 0);
	loadToIt(singleStartingTownHeaderWConfluxID, buf, i, 0);
	loadToIt(singleStartingHeroHeaderID, buf, i, 0);
	loadToIt(singleStartingBonusHeaderID, buf, i, 0);

	hmcr = 0;
	for(i; i<buf.length(); ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==3)
			break;
	}
	i+=2;

	loadToIt(multiOnlineService, buf, i, 0);
	loadToIt(multiHotSeat, buf, i, 0);
	loadToIt(multiIPX, buf, i, 0);
	loadToIt(multiTCPIP, buf, i, 0);
	loadToIt(multiModem, buf, i, 0);
	loadToIt(multiDirectConnection, buf, i, 0);
	loadToIt(multiHostGame, buf, i, 0);
	loadToIt(multiJoinGame, buf, i, 1);
	loadToIt(multiSearchGame, buf, i, 1);
	for(int vv=0; vv<12; ++vv)
	{
		loadToIt(multiGameNo[vv], buf, i, 1);
	}
	loadToIt(multiScrollGames, buf, i, 1);
	std::string dump;
	loadToIt(dump, buf, i, 1);
	loadToIt(multiCancel, buf, i, 0);
	loadToIt(dump, buf, i, 0);
	loadToIt(dump, buf, i, 4);
	loadToIt(dump, buf, i, 2);

	loadToIt(advWorldMap.first, buf, i, 4);
	loadToIt(advWorldMap.second, buf, i, 2);
	loadToIt(advStatusWindow1.first, buf, i, 4);
	loadToIt(advStatusWindow1.second, buf, i, 2);
	loadToIt(advKingdomOverview.first, buf, i, 4);
	loadToIt(advKingdomOverview.second, buf, i, 2);
	loadToIt(advSurfaceSwitch.first, buf, i, 4);
	loadToIt(advSurfaceSwitch.second, buf, i, 2);
	loadToIt(advQuestlog.first, buf, i, 4);
	loadToIt(advQuestlog.second, buf, i, 2);
	loadToIt(advSleepWake.first, buf, i, 4);
	loadToIt(advSleepWake.second, buf, i, 2);
	loadToIt(advMoveHero.first, buf, i, 4);
	loadToIt(advMoveHero.second, buf, i, 2);
	loadToIt(advCastSpell.first, buf, i, 4);
	loadToIt(advCastSpell.second, buf, i, 2);
	loadToIt(advAdvOptions.first, buf, i, 4);
	loadToIt(advAdvOptions.second, buf, i, 2);
	loadToIt(advSystemOptions.first, buf, i, 4);
	loadToIt(advSystemOptions.second, buf, i, 2);
	loadToIt(advNextHero.first, buf, i, 4);
	loadToIt(advNextHero.second, buf, i, 2);
	loadToIt(advEndTurn.first, buf, i, 4);
	loadToIt(advEndTurn.second, buf, i, 2);

	loadLossConditions();
	loadVictoryConditions();
}

void CPreGameTextHandler::loadToIt(std::string &dest, std::string &src, int &iter, int mode)
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
	case 4:
		{
			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					break;
			}
			dest = src.substr(befi, iter-befi);
			iter++;
			break;
		}
	}
	//loadLossConditions();
	//loadVictoryConditions(); //moved to loadTexts
}

void CPreGameTextHandler::loadVictoryConditions()
{
	//std::ifstream inp("H3bitmap.lod\\VCDESC.TXT", std::ios::in|std::ios::binary);
	//inp.seekg(0,std::ios::end); // na koniec
	//int andame = inp.tellg();  // read length
	//inp.seekg(0,std::ios::beg); // wracamy na poczatek
	//char * bufor = new char[andame]; // allocate memory 
	//inp.read((char*)bufor, andame); // read map file to buffer
	//inp.close();
	//std::string buf = std::string(bufor);
	//delete [andame] bufor;
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("VCDESC.TXT");
	int andame = buf.size();
	int i=0; //buf iterator

	for(int gg=0; gg<14; ++gg)
	{
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		victoryConditions[gg] = buf.substr(befi, i-befi);
		i+=2;
	}
}

void CPreGameTextHandler::loadLossConditions()
{
	//std::ifstream inp("H3bitmap.lod\\LCDESC.TXT", std::ios::in|std::ios::binary);
	//inp.seekg(0,std::ios::end); // na koniec
	//int andame = inp.tellg();  // read length
	//inp.seekg(0,std::ios::beg); // wracamy na poczatek
	//char * bufor = new char[andame]; // allocate memory 
	//inp.read((char*)bufor, andame); // read map file to buffer
	//inp.close();
	//std::string buf = std::string(bufor);
	//delete [andame] bufor;
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("LCDESC.TXT");
	int andame = buf.size();
	int i=0; //buf iterator

	for(int gg=0; gg<4; ++gg)
	{
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		lossCondtions[gg] = buf.substr(befi, i-befi);
		i+=2;
	}
}
