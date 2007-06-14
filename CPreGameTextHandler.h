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
	std::string getTitle(std::string text);
	std::string getDescr(std::string text);
	void loadTexts();
	void loadToIt(std::string & dest, std::string & src, int & iter, int mode = 0); //mode 0 - dump to tab, destto tab, dump to eol //mode 1 - dump to tab, src to eol
};


#endif //CPREGAMETEXTHANDLER_H