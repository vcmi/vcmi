#ifndef CPREGAMETEXTHANDLER_H
#define CPREGAMETEXTHANDLER_H

#include <string>

class CPreGameTextHandler //handles pre - game texts
{
public:
	std::string mainNewGame, mainLoadGame, mainHighScores, mainCredits, mainQuit; //right - click texts in main menu
	std::string ngSingleScenario, ngCampain, ngMultiplayer, ngTutorial, ngBack; //right - click texts in new game menu
	std::string getTitle(std::string text);
	std::string getDescr(std::string text);
	void loadTexts();
};


#endif //CPREGAMETEXTHANDLER_H