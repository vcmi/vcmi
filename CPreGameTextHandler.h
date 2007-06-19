#ifndef CPREGAMETEXTHANDLER_H
#define CPREGAMETEXTHANDLER_H

#include <string>

class CPreGameTextHandler //handles pre - game texts
{
public:
	std::string mainNewGame, mainLoadGame, mainHighScores, mainCredits, mainQuit; //right - click texts in main menu
	std::string ngSingleScenario, ngCampain, ngMultiplayer, ngTutorial, ngBack; //right - click texts in new game menu
	std::string singleChooseScenario, singleSetAdvOptions, singleRandomMap, singleScenarioName, singleDescriptionTitle, singleDescriptionText, singleEasy, singleNormal, singleHard, singleExpert, singleImpossible; //main single scenario texts
	std::string singleAllyFlag[8], singleEnemyFlag[8];
	std::string singleViewHideScenarioList, singleViewHideAdvOptions, singlePlayRandom, singleChatDesc, singleMapDifficulty, singleRating, singleMapPossibleDifficulties, singleVicCon, singleLossCon;
	std::string singleSFilter, singleMFilter, singleLFilter, singleXLFilter, singleAllFilter;
	std::string singleScenarioNameNr[18], singleEntryScenarioNameNr[18];
	std::string singleTurnDuration, singleChatText, singleChatEntry, singleChatPlug, singleChatPlayer, singleChatPlayerSlider, singleRollover, singleNext, singleBegin, singleBack, singleSSExit, singleWhichMap, singleSortNumber, singleSortSize, singleSortVersion, singleSortAlpha, singleSortVictory, singleSortLoss, singleBriefing, singleSSHero, singleGoldpic;
	std::string singleHumanCPU[8], singleHandicap[8], singleTownLeft[8], singleTownRite[8], singleHeroLeft[8], singleHeroRite[8], singleResLeft[8], singleResRite[8], singleHeroSetting[8], singleTownSetting[8];
	std::string singleConstCreateMap, singleConstMapSizeLabel, singleConstSmallMap, singleConstMediumMap, singleConstLargeMap, singleConstHugeMap, singleConstMapLevels, singleConstHumanPositionsLabel;
	std::string singleConstNHumans[8];
	std::string singleConstRandomHumans, singleConstHumanTeamsLabel, singleConstNoHumanTeams;
	std::string singleConstNHumanTeams[7];
	std::string singleConstRandomHumanTeams, singleConstComputerPositionsLabel, singleConstNoComputers;
	std::string singleConstNComputers[7];
	std::string singleConstRandomComputers, singleConstComputerTeamsLabel, singleConstNoComputerTeams;
	std::string singleConstNComputerTeams[6];
	std::string singleConstRandomComputerTeams, singleConstWaterLabel, singleConstNoWater, singleConstNormalWater, singleConstIslands, singleConstRandomWater, singleConstMonsterStrengthLabel, singleConstWeakMonsters, singleConstNormalMonsters, singleConstStrongMonsters, singleConstRandomMonsters, singleConstShowSavedRandomMaps, singleSliderChatWindow, singleSliderFileMenu, singleSliderDuration;
	std::string singlePlayerHandicapHeaderID, singleTurnDurationHeaderID, singleStartingTownHeaderID, singleStartingTownHeaderWConfluxID, singleStartingHeroHeaderID, singleStartingBonusHeaderID;
	std::string getTitle(std::string text);
	std::string getDescr(std::string text);
	void loadTexts();
	void loadToIt(std::string & dest, std::string & src, int & iter, int mode = 0); //mode 0 - dump to tab, dest to tab, dump to eol //mode 1 - dump to tab, src to eol
};


#endif //CPREGAMETEXTHANDLER_H